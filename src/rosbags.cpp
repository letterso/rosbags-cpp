#include "internal.hpp"

#include <filesystem>

namespace rosbags {

void RosbagsError::add_context(const ErrorContext& context) {
#define ROSBAGS_CONTEXT_FIELD(name) if (!context_.name) context_.name = context.name
  ROSBAGS_CONTEXT_FIELD(source_path);
  ROSBAGS_CONTEXT_FIELD(connection_id);
  ROSBAGS_CONTEXT_FIELD(topic);
  ROSBAGS_CONTEXT_FIELD(type);
  ROSBAGS_CONTEXT_FIELD(serialization_format);
  ROSBAGS_CONTEXT_FIELD(timestamp);
  ROSBAGS_CONTEXT_FIELD(byte_offset);
#undef ROSBAGS_CONTEXT_FIELD
  diagnostic_ = reason();
  if (context_.source_path) diagnostic_ += " [file=" + *context_.source_path + "]";
  if (context_.connection_id) diagnostic_ += " [connection=" + std::to_string(*context_.connection_id) + "]";
  if (context_.topic) diagnostic_ += " [topic=" + *context_.topic + "]";
  if (context_.type) diagnostic_ += " [type=" + *context_.type + "]";
  if (context_.serialization_format) diagnostic_ += " [format=" + *context_.serialization_format + "]";
  if (context_.timestamp) diagnostic_ += " [timestamp_ns=" + std::to_string(*context_.timestamp) + "]";
  if (context_.byte_offset) diagnostic_ += " [byte_offset=" + std::to_string(*context_.byte_offset) + "]";
}

const char* RosbagsError::what() const noexcept {
  return diagnostic_.empty() ? reason() : diagnostic_.c_str();
}

struct MessageCursor::Impl {
  virtual ~Impl() = default;
  virtual bool next(Message& output) = 0;
};

namespace {

class BackendCursorImpl final : public MessageCursor::Impl {
 public:
  BackendCursorImpl(std::shared_ptr<void> owner, std::unique_ptr<internal::BackendCursor> cursor,
                    std::string path)
      : owner_(std::move(owner)), cursor_(std::move(cursor)), path_(std::move(path)) {}
  bool next(Message& output) override {
    try {
      return cursor_->next(output);
    } catch (RosbagsError& error) {
      ErrorContext context;
      context.source_path = path_;
      error.add_context(context);
      throw;
    }
  }

 private:
  // Keep the backend alive until after its cursor has been destroyed.
  std::shared_ptr<void> owner_;
  std::unique_ptr<internal::BackendCursor> cursor_;
  std::string path_;
};

}  // namespace

struct AnyReader::Impl {
  std::vector<std::unique_ptr<Reader>> readers;
  std::vector<Connection> connections;
  ReaderMetadata metadata;
  bool open = false;
};

class AnyReaderCursorImpl final : public MessageCursor::Impl {
 public:
  AnyReaderCursorImpl(std::shared_ptr<AnyReader::Impl> owner, const ReadFilter& filter)
      : owner_(std::move(owner)) {
    std::size_t global_index = 0;
    for (const auto& reader : owner_->readers) {
      Source source;
      const auto& local_connections = reader->connections();
      source.global_index = global_index;
      source.local_positions.reserve(local_connections.size());
      for (std::size_t index = 0; index < local_connections.size(); ++index)
        source.local_positions.emplace(&local_connections[index], index);

      ReadFilter local = filter;
      local.connection_ids.clear();
      if (!filter.connection_ids.empty()) {
        for (std::size_t index = 0; index < local_connections.size(); ++index) {
          const auto global_id = owner_->connections[global_index + index].id;
          if (std::find(filter.connection_ids.begin(), filter.connection_ids.end(), global_id) !=
              filter.connection_ids.end())
            local.connection_ids.push_back(local_connections[index].id);
        }
      }
      if (filter.connection_ids.empty() || !local.connection_ids.empty()) {
        source.cursor.emplace(reader->messages(local));
        sources_.push_back(std::move(source));
        pull(sources_.size() - 1);
      }
      global_index += local_connections.size();
    }
  }

  bool next(Message& output) override {
    if (!owner_->open) throw RosbagsError("reader is not open");
    if (pending_source_) {
      pull(*pending_source_);
      pending_source_.reset();
    }
    if (heap_.empty()) return false;
    const auto item = heap_.top();
    heap_.pop();
    auto& source = sources_[item.source];
    output = std::move(*source.head);
    source.head.reset();
    pending_source_ = item.source;
    return true;
  }

 private:
  struct Source {
    std::optional<MessageCursor> cursor;
    std::size_t global_index = 0;
    std::unordered_map<const Connection*, std::size_t> local_positions;
    std::optional<Message> head;
  };
  struct HeapItem {
    std::uint64_t timestamp = 0;
    std::size_t source = 0;
  };
  struct HeapCompare {
    bool operator()(const HeapItem& left, const HeapItem& right) const {
      if (left.timestamp != right.timestamp) return left.timestamp > right.timestamp;
      return left.source > right.source;
    }
  };

  void pull(std::size_t source_index) {
    auto& source = sources_[source_index];
    Message message;
    if (!source.cursor || !source.cursor->next(message)) return;
    const auto position = source.local_positions.find(message.connection);
    if (position == source.local_positions.end()) throw RosbagsError("backend returned unknown connection");
    message.connection = &owner_->connections[source.global_index + position->second];
    source.head = std::move(message);
    heap_.push({source.head->timestamp, source_index});
  }

  std::shared_ptr<AnyReader::Impl> owner_;
  std::vector<Source> sources_;
  std::priority_queue<HeapItem, std::vector<HeapItem>, HeapCompare> heap_;
  std::optional<std::size_t> pending_source_;
};

MessageCursor::MessageCursor(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
MessageCursor::~MessageCursor() = default;
MessageCursor::MessageCursor(MessageCursor&&) noexcept = default;
MessageCursor& MessageCursor::operator=(MessageCursor&&) noexcept = default;
bool MessageCursor::next(Message& output) {
  if (!impl_) throw RosbagsError("message cursor is not initialized");
  return impl_->next(output);
}

struct Reader::Impl {
  explicit Impl(std::string value) : path(std::move(value)) {}
  ~Impl() {
    if (backend) backend->close();
  }
  std::string path;
  std::unique_ptr<internal::Backend> backend;
};

Reader::Reader(std::string path, ReaderOptions options) : impl_(std::make_shared<Impl>(std::move(path))) {
  const std::filesystem::path input(impl_->path);
  if (!std::filesystem::exists(input)) throw RosbagsError("path does not exist: " + impl_->path);
  if (std::filesystem::is_directory(input)) {
    impl_->backend = internal::make_directory_backend(impl_->path);
  } else if (input.extension() == ".bag") {
    impl_->backend = internal::make_rosbag1_backend(impl_->path, options);
  } else if (input.extension() == ".mcap") {
    impl_->backend = internal::make_mcap_backend(impl_->path);
  } else {
    // ROS2 files are SQLite databases even when an application uses a custom suffix.
    impl_->backend = internal::make_sqlite_backend(impl_->path);
  }
}

Reader::~Reader() = default;
Reader::Reader(Reader&&) noexcept = default;
Reader& Reader::operator=(Reader&&) noexcept = default;

void Reader::open() {
  if (is_open()) throw RosbagsError("reader is already open");
  try {
    impl_->backend->open();
  } catch (RosbagsError& error) {
    ErrorContext context;
    context.source_path = impl_->path;
    error.add_context(context);
    throw;
  }
}

void Reader::close() noexcept {
  if (impl_ && impl_->backend) impl_->backend->close();
}

bool Reader::is_open() const noexcept {
  return impl_ && impl_->backend && impl_->backend->is_open();
}

StorageKind Reader::storage_kind() const {
  if (!impl_->backend) throw RosbagsError("reader has no backend");
  return impl_->backend->kind();
}
const std::string& Reader::path() const noexcept { return impl_->path; }
const ReaderMetadata& Reader::metadata() const { return impl_->backend->metadata(); }
const std::vector<Connection>& Reader::connections() const { return impl_->backend->connections(); }
MessageCursor Reader::messages(const ReadFilter& filter) const {
  if (!is_open()) throw RosbagsError("reader is not open");
  try {
    (void)topics_match({}, {}, filter.topic_match, filter.topic_namespace);
    auto cursor = impl_->backend->make_cursor(filter);
    return MessageCursor(std::make_unique<BackendCursorImpl>(impl_, std::move(cursor), impl_->path));
  } catch (RosbagsError& error) {
    ErrorContext context;
    context.source_path = impl_->path;
    error.add_context(context);
    throw;
  }
}
void Reader::read_raw(const ReadFilter& filter, const MessageCallback& callback) const {
  if (!callback) throw RosbagsError("message callback is empty");
  auto cursor = messages(filter);
  Message message;
  while (cursor.next(message)) callback(message);
}
void Reader::read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                          const DecodedCallback& callback, UnknownTypePolicy policy,
                          const WarningCallback& warning) const {
  if (!callback) throw RosbagsError("decoded message callback is empty");
  read_raw(filter, [&](const Message& message) {
    if (const auto value = decode(message, registry, profile, policy, warning)) callback(message, *value);
  });
}

AnyReader::AnyReader(std::vector<std::string> paths, ReaderOptions options) : impl_(std::make_shared<Impl>()) {
  if (paths.empty()) throw RosbagsError("at least one input path is required");
  for (auto& path : paths) impl_->readers.push_back(std::make_unique<Reader>(std::move(path), options));
}
AnyReader::~AnyReader() = default;
AnyReader::AnyReader(AnyReader&&) noexcept = default;
AnyReader& AnyReader::operator=(AnyReader&&) noexcept = default;

void AnyReader::open() {
  if (!impl_) throw RosbagsError("reader is not initialized");
  if (impl_->open) throw RosbagsError("reader is already open");
  try {
    for (const auto& reader : impl_->readers) reader->open();
    const auto first_kind = impl_->readers.front()->storage_kind();
    const bool first_is_ros1 = first_kind == StorageKind::Rosbag1;
    for (const auto& reader : impl_->readers) {
      if ((reader->storage_kind() == StorageKind::Rosbag1) != first_is_ros1)
        throw RosbagsError("ROS1 and ROS2 inputs cannot be mixed");
    }

    impl_->connections.clear();
    std::uint32_t next_id = 1;
    for (const auto& reader : impl_->readers) {
      for (const auto& connection : reader->connections()) {
        auto copy = connection;
        copy.id = next_id++;
        impl_->connections.push_back(std::move(copy));
      }
    }

    impl_->metadata = ReaderMetadata{};
    impl_->metadata.storage = first_kind;
    impl_->metadata.start_time = std::numeric_limits<std::uint64_t>::max();
    bool have_messages = false;
    for (const auto& reader : impl_->readers) {
      const auto& metadata = reader->metadata();
      if (metadata.message_count) {
        have_messages = true;
        impl_->metadata.start_time = std::min(impl_->metadata.start_time, metadata.start_time);
        impl_->metadata.end_time = std::max(impl_->metadata.end_time, metadata.end_time);
      }
      impl_->metadata.message_count += metadata.message_count;
      impl_->metadata.files.insert(impl_->metadata.files.end(), metadata.files.begin(), metadata.files.end());
    }
    if (!have_messages) impl_->metadata.start_time = 0;
    impl_->metadata.duration = impl_->metadata.end_time >= impl_->metadata.start_time
                                   ? impl_->metadata.end_time - impl_->metadata.start_time
                                   : 0;
    impl_->open = true;
  } catch (...) {
    close();
    throw;
  }
}

void AnyReader::close() noexcept {
  if (!impl_) return;
  for (const auto& reader : impl_->readers) reader->close();
  impl_->open = false;
  impl_->connections.clear();
}
bool AnyReader::is_open() const noexcept { return impl_ && impl_->open; }
const ReaderMetadata& AnyReader::metadata() const {
  if (!is_open()) throw RosbagsError("reader is not open");
  return impl_->metadata;
}
const std::vector<Connection>& AnyReader::connections() const {
  if (!is_open()) throw RosbagsError("reader is not open");
  return impl_->connections;
}

MessageCursor AnyReader::messages(const ReadFilter& filter) const {
  if (!is_open()) throw RosbagsError("reader is not open");
  return MessageCursor(std::make_unique<AnyReaderCursorImpl>(impl_, filter));
}
void AnyReader::read_raw(const ReadFilter& filter, const MessageCallback& callback) const {
  if (!callback) throw RosbagsError("message callback is empty");
  auto cursor = messages(filter);
  Message message;
  while (cursor.next(message)) callback(message);
}
void AnyReader::read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                             const DecodedCallback& callback, UnknownTypePolicy policy,
                             const WarningCallback& warning) const {
  if (!callback) throw RosbagsError("decoded message callback is empty");
  read_raw(filter, [&](const Message& message) {
    if (const auto value = decode(message, registry, profile, policy, warning)) callback(message, *value);
  });
}

std::string TypeRegistry::key(std::string_view profile, std::string_view type) {
  return std::string(profile) + '\n' + normalize_type(type);
}
void TypeRegistry::register_type(std::shared_ptr<const TypeSupportBase> support) {
  if (!support || support->type_name.empty() || support->profile.empty())
    throw RosbagsError("invalid type support");
  supports_[key(support->profile, support->type_name)] = std::move(support);
}
const TypeSupportBase* TypeRegistry::find(std::string_view profile, std::string_view type) const {
  const auto it = supports_.find(key(profile, type));
  return it == supports_.end() ? nullptr : it->second.get();
}
std::vector<std::string> TypeRegistry::types() const {
  std::vector<std::string> result;
  result.reserve(supports_.size());
  for (const auto& item : supports_) {
    const auto separator = item.first.find('\n');
    result.push_back(separator == std::string::npos ? item.first : item.first.substr(separator + 1));
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::optional<DecodedMessage> decode(const Message& message, const TypeRegistry& registry,
                                     std::string_view profile, UnknownTypePolicy policy,
                                     const WarningCallback& warning) {
  const auto message_context = [&]() {
    ErrorContext context;
    if (!message.source_path.empty()) context.source_path = message.source_path;
    context.timestamp = message.timestamp;
    if (message.connection) {
      context.connection_id = message.connection->id;
      context.topic = message.connection->original_topic.empty()
                          ? message.connection->topic : message.connection->original_topic;
      context.type = normalize_type(message.connection->type);
      context.serialization_format = message.connection->serialization_format;
    }
    return context;
  };
  if (!message.connection || !message.bytes) {
    DecodeError error("message has no connection/data");
    error.add_context(message_context());
    throw error;
  }
  const auto* support = registry.find(profile, message.connection->type);
  if (!support) {
    const auto text = "unregistered message type: " + message.connection->type + " on " +
                      message.connection->topic;
    if (warning) warning(text);
    if (policy == UnknownTypePolicy::Error) {
      DecodeError error(text);
      error.add_context(message_context());
      throw error;
    }
    if (policy == UnknownTypePolicy::WarnAndRaw) return DecodedMessage{nullptr, nullptr, &message};
    return std::nullopt;
  }
  try {
    std::shared_ptr<void> object;
    if (message.connection->serialization_format == "cdr") {
      object = support->deserialize_cdr(ByteView{message.bytes->data(), message.bytes->size()});
    } else if (message.connection->serialization_format == "ros1" ||
               message.connection->serialization_format.empty()) {
      object = support->deserialize_ros1(ByteView{message.bytes->data(), message.bytes->size()});
    } else {
      throw UnsupportedFeature("unsupported message serialization format: " +
                               message.connection->serialization_format);
    }
    return DecodedMessage{std::move(object), support, &message};
  } catch (RosbagsError& error) {
    error.add_context(message_context());
    throw;
  } catch (const std::exception& error) {
    DecodeError wrapped(error.what());
    wrapped.add_context(message_context());
    std::throw_with_nested(wrapped);
  }
}

std::string storage_kind_name(StorageKind kind) {
  switch (kind) {
    case StorageKind::Rosbag1: return "rosbag1";
    case StorageKind::Sqlite3: return "sqlite3";
    case StorageKind::Mcap: return "mcap";
  }
  return "unknown";
}
std::string definition_format_name(DefinitionFormat format) {
  switch (format) {
    case DefinitionFormat::None: return "none";
    case DefinitionFormat::Msg: return "msg";
    case DefinitionFormat::Idl: return "idl";
  }
  return "unknown";
}

std::string normalize_topic(std::string_view topic) {
  std::string result;
  if (!topic.empty() && topic.front() == '/') result.push_back('/');
  bool slash = false;
  for (const char value : topic) {
    if (value == '/') {
      if (!slash && !result.empty() && result.back() != '/') result.push_back('/');
      slash = true;
    } else {
      result.push_back(value);
      slash = false;
    }
  }
  while (result.size() > 1 && result.back() == '/') result.pop_back();
  return result;
}

bool topics_match(std::string_view left, std::string_view right,
                  TopicMatchPolicy policy, std::string_view topic_namespace) {
  auto name_space = normalize_topic(topic_namespace);
  if (policy == TopicMatchPolicy::ResolveNamespace &&
      (name_space.empty() || name_space.front() != '/'))
    throw RosbagsError("topic namespace must be absolute");
  const auto key = [&](std::string_view name) {
    auto value = normalize_topic(name);
    if (policy == TopicMatchPolicy::IgnoreLeadingSlash) {
      if (!value.empty() && value.front() == '/') value.erase(0, 1);
    } else if (policy == TopicMatchPolicy::ResolveNamespace &&
               !value.empty() && value.front() != '/') {
      value = normalize_topic(name_space + "/" + value);
    }
    return value;
  };
  return key(left) == key(right);
}

std::uint64_t timestamp_nanoseconds(std::int64_t seconds, std::uint32_t nanoseconds) {
  constexpr std::uint64_t scale = 1000000000;
  if (seconds < 0 || nanoseconds >= scale ||
      static_cast<std::uint64_t>(seconds) >
          (std::numeric_limits<std::uint64_t>::max() - nanoseconds) / scale)
    throw RosbagsError("timestamp is outside the unsigned nanosecond range");
  return static_cast<std::uint64_t>(seconds) * scale + nanoseconds;
}

std::uint64_t nanoseconds_to_microseconds(std::uint64_t nanoseconds) noexcept {
  return nanoseconds / 1000;
}

std::uint64_t microseconds_to_nanoseconds(std::uint64_t microseconds) {
  if (microseconds > std::numeric_limits<std::uint64_t>::max() / 1000)
    throw RosbagsError("microsecond timestamp overflows nanoseconds");
  return microseconds * 1000;
}

std::string normalize_type(std::string_view type) {
  std::string value(type);
  const auto slash = value.find_last_of('/');
  if (slash == std::string::npos) return value;
  const auto parent = value.substr(0, slash);
  const auto has_suffix = [&parent](std::string_view suffix) {
    return parent.size() >= suffix.size() &&
           parent.compare(parent.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  if (has_suffix("/msg") || has_suffix("/action")) return value;
  return parent + "/msg/" + value.substr(slash + 1);
}

}  // namespace rosbags
