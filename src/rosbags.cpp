#include "internal.hpp"

#include <filesystem>

namespace rosbags {

struct Reader::Impl {
  explicit Impl(std::string value) : path(std::move(value)) {}
  std::string path;
  std::unique_ptr<internal::Backend> backend;
};

Reader::Reader(std::string path) : impl_(std::make_unique<Impl>(std::move(path))) {
  const std::filesystem::path input(impl_->path);
  if (!std::filesystem::exists(input)) throw RosbagsError("path does not exist: " + impl_->path);
  if (std::filesystem::is_directory(input)) {
    impl_->backend = internal::make_directory_backend(impl_->path);
  } else if (input.extension() == ".bag") {
    impl_->backend = internal::make_rosbag1_backend(impl_->path);
  } else if (input.extension() == ".mcap") {
    impl_->backend = internal::make_mcap_backend(impl_->path);
  } else {
    // ROS2 files are SQLite databases even when an application uses a custom suffix.
    impl_->backend = internal::make_sqlite_backend(impl_->path);
  }
}

Reader::~Reader() { close(); }
Reader::Reader(Reader&&) noexcept = default;
Reader& Reader::operator=(Reader&&) noexcept = default;

void Reader::open() {
  if (is_open()) throw RosbagsError("reader is already open");
  impl_->backend->open();
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
void Reader::read_raw(const ReadFilter& filter, const MessageCallback& callback) const {
  if (!callback) throw RosbagsError("message callback is empty");
  impl_->backend->read_raw(filter, callback);
}
void Reader::read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                          const DecodedCallback& callback, UnknownTypePolicy policy,
                          const WarningCallback& warning) const {
  if (!callback) throw RosbagsError("decoded message callback is empty");
  read_raw(filter, [&](const Message& message) {
    if (const auto value = decode(message, registry, profile, policy, warning)) callback(message, *value);
  });
}

AnyReader::AnyReader(std::vector<std::string> paths) : paths_(std::move(paths)) {
  if (paths_.empty()) throw RosbagsError("at least one input path is required");
  for (const auto& path : paths_) readers_.push_back(std::make_unique<Reader>(path));
}

void AnyReader::open() {
  if (open_) throw RosbagsError("reader is already open");
  try {
    for (const auto& reader : readers_) reader->open();
    const auto first_kind = readers_.front()->storage_kind();
    const bool first_is_ros1 = first_kind == StorageKind::Rosbag1;
    for (const auto& reader : readers_) {
      if ((reader->storage_kind() == StorageKind::Rosbag1) != first_is_ros1)
        throw RosbagsError("ROS1 and ROS2 inputs cannot be mixed");
    }

    connections_.clear();
    std::uint32_t next_id = 1;
    for (const auto& reader : readers_) {
      for (const auto& connection : reader->connections()) {
        auto copy = connection;
        copy.id = next_id++;
        connections_.push_back(std::move(copy));
      }
    }

    metadata_ = ReaderMetadata{};
    metadata_.storage = first_kind;
    metadata_.start_time = std::numeric_limits<std::uint64_t>::max();
    bool have_messages = false;
    for (const auto& reader : readers_) {
      const auto& metadata = reader->metadata();
      if (metadata.message_count) {
        have_messages = true;
        metadata_.start_time = std::min(metadata_.start_time, metadata.start_time);
        metadata_.end_time = std::max(metadata_.end_time, metadata.end_time);
      }
      metadata_.message_count += metadata.message_count;
      metadata_.files.insert(metadata_.files.end(), metadata.files.begin(), metadata.files.end());
    }
    if (!have_messages) metadata_.start_time = 0;
    metadata_.duration = metadata_.end_time >= metadata_.start_time
                             ? metadata_.end_time - metadata_.start_time
                             : 0;
    open_ = true;
  } catch (...) {
    close();
    throw;
  }
}

void AnyReader::close() noexcept {
  for (const auto& reader : readers_) reader->close();
  open_ = false;
  connections_.clear();
}
bool AnyReader::is_open() const noexcept { return open_; }
const ReaderMetadata& AnyReader::metadata() const {
  if (!open_) throw RosbagsError("reader is not open");
  return metadata_;
}
const std::vector<Connection>& AnyReader::connections() const {
  if (!open_) throw RosbagsError("reader is not open");
  return connections_;
}

void AnyReader::read_raw(const ReadFilter& filter, const MessageCallback& callback) const {
  if (!open_) throw RosbagsError("reader is not open");
  if (!callback) throw RosbagsError("message callback is empty");

  std::vector<Message> all;
  std::size_t global_index = 0;
  for (const auto& reader : readers_) {
    const auto local_count = reader->connections().size();
    ReadFilter local = filter;
    local.connection_ids.clear();
    if (!filter.connection_ids.empty()) {
      for (std::size_t i = 0; i < local_count; ++i) {
        const auto global_id = connections_[global_index + i].id;
        if (std::find(filter.connection_ids.begin(), filter.connection_ids.end(), global_id) !=
            filter.connection_ids.end())
          local.connection_ids.push_back(reader->connections()[i].id);
      }
      if (local.connection_ids.empty()) {
        global_index += local_count;
        continue;
      }
    }
    reader->read_raw(local, [&](const Message& message) {
      const auto* local_connection = message.connection;
      const auto local_pos = static_cast<std::size_t>(
          std::distance(reader->connections().data(), local_connection));
      if (local_pos >= local_count) throw RosbagsError("backend returned unknown connection");
      auto copy = message;
      copy.connection = &connections_[global_index + local_pos];
      all.push_back(std::move(copy));
    });
    global_index += local_count;
  }
  std::stable_sort(all.begin(), all.end(), [](const Message& a, const Message& b) {
    return a.timestamp < b.timestamp;
  });
  for (const auto& message : all) {
    if (internal::in_time(message.timestamp, filter)) callback(message);
  }
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
  if (!message.connection || !message.bytes) throw DecodeError("message has no connection/data");
  const auto* support = registry.find(profile, message.connection->type);
  if (!support) {
    const auto text = "unregistered message type: " + message.connection->type + " on " +
                      message.connection->topic;
    if (warning) warning(text);
    if (policy == UnknownTypePolicy::Error) throw DecodeError(text);
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
  } catch (const RosbagsError&) {
    throw;
  } catch (const std::exception& error) {
    throw DecodeError(error.what());
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
