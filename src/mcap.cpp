#include "internal.hpp"
#include "temporary_file.hpp"

#if ROSBAGS_HAS_MCAP
#include <mcap/reader.hpp>
#endif

namespace rosbags::internal {
namespace {

#if ROSBAGS_HAS_MCAP
struct ContainerShape {
  std::size_t chunks = 0;
  bool direct_messages = false;
};

// Check lengths before the SDK allocates record or decompression buffers.
ContainerShape inspect_container(const std::string& path, const ReaderOptions& options) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) throw FormatError(context(path, "cannot open MCAP file"));
  const auto end = input.tellg();
  if (end < 16) throw FormatError(context(path, "truncated MCAP file"));
  const auto data_end = static_cast<std::uint64_t>(end) - 8;
  std::uint64_t position = 8;
  std::size_t index_bytes = 0;
  ContainerShape shape;
  while (position < data_end) {
    if (data_end - position < 9) throw FormatError(context(path, "truncated MCAP record header"));
    input.seekg(static_cast<std::streamoff>(position));
    std::array<Byte,9> header{};
    input.read(reinterpret_cast<char*>(header.data()), header.size());
    if (!input) throw FormatError(context(path, "cannot read MCAP record header"));
    const auto length = u64(header.data()+1);
    position += header.size();
    if (length > data_end-position) throw FormatError(context(path, "MCAP record exceeds file size"));
    const auto opcode = static_cast<mcap::OpCode>(header[0]);
    if (opcode == mcap::OpCode::Chunk) {
      if (length < 40) throw FormatError(context(path, "truncated MCAP chunk"));
      std::array<Byte,24> prefix{};
      input.read(reinterpret_cast<char*>(prefix.data()), prefix.size());
      if (!input) throw FormatError(context(path, "cannot read MCAP chunk prefix"));
      if (u64(prefix.data()+16) > options.max_mcap_chunk_bytes ||
          (length > options.max_mcap_chunk_bytes && length-options.max_mcap_chunk_bytes > options.max_connection_body_bytes))
        throw ResourceLimitError(context(path, "MCAP chunk exceeds max_mcap_chunk_bytes"));
      if (u64(prefix.data()+8) == std::numeric_limits<std::uint64_t>::max())
        throw FormatError(context(path, "MCAP message end time overflows uint64_t"));
      ++shape.chunks;
    } else if (opcode == mcap::OpCode::Message) {
      if (length < 22) throw FormatError(context(path, "truncated MCAP message"));
      if (length-22 > options.max_message_bytes)
        throw ResourceLimitError(context(path, "MCAP message exceeds max_message_bytes"));
      std::array<Byte,14> prefix{};
      input.read(reinterpret_cast<char*>(prefix.data()), prefix.size());
      if (!input) throw FormatError(context(path, "cannot read MCAP message prefix"));
      if (u64(prefix.data()+6) == std::numeric_limits<std::uint64_t>::max())
        throw FormatError(context(path, "MCAP message end time overflows uint64_t"));
      shape.direct_messages = true;
    } else if (opcode == mcap::OpCode::Schema || opcode == mcap::OpCode::Channel ||
               opcode == mcap::OpCode::Header || opcode == mcap::OpCode::Metadata) {
      if (length > options.max_connection_body_bytes)
        throw ResourceLimitError(context(path, "MCAP metadata record exceeds max_connection_body_bytes"));
    } else if (opcode == mcap::OpCode::MessageIndex || opcode == mcap::OpCode::ChunkIndex ||
               opcode == mcap::OpCode::AttachmentIndex || opcode == mcap::OpCode::MetadataIndex ||
               opcode == mcap::OpCode::Statistics || opcode == mcap::OpCode::SummaryOffset) {
      if (length > options.max_index_bytes-index_bytes)
        throw ResourceLimitError(context(path, "MCAP index records exceed max_index_bytes"));
      index_bytes += static_cast<std::size_t>(length);
    } else if (opcode == mcap::OpCode::Attachment && length > options.max_message_bytes) {
      throw ResourceLimitError(context(path, "MCAP attachment exceeds max_message_bytes"));
    }
    position += length;
  }
  return shape;
}
#endif

class McapBackend final : public Backend {
 public:
  McapBackend(std::string path, ReaderOptions options) : path_(std::move(path)), options_(options) {}
  void open() override {
#if ROSBAGS_HAS_MCAP
    if (open_) throw RosbagsError("MCAP reader is already open");
    close();
    try {
      const auto shape = inspect_container(path_, options_);
      const auto status = reader_.open(path_);
      if (!status.ok()) throw FormatError(context(path_, "MCAP SDK failed to open file: " + status.message));
      const auto summary = reader_.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
      if (!summary.ok() && summary.code != mcap::StatusCode::MissingStatistics)
        throw FormatError(context(path_, "MCAP summary scan failed: " + summary.message));
      auto statistics = reader_.statistics();
      schemas_ = reader_.schemas();
      channels_ = reader_.channels();
      indexed_ = !shape.direct_messages && shape.chunks == reader_.chunkIndexes().size() &&
          !channels_.empty() && std::all_of(reader_.chunkIndexes().begin(), reader_.chunkIndexes().end(),
            [](const auto& chunk) { return chunk.messageIndexLength != 0 && !chunk.messageIndexOffsets.empty(); });
      indexed_ = indexed_ && std::all_of(channels_.begin(),channels_.end(),[&](const auto& item) {
        return item.second->schemaId == 0 || schemas_.count(item.second->schemaId);
      });
      // A separate scan recovers statistics without discarding valid message indexes.
      if (!statistics || !indexed_) {
        mcap::McapReader scan;
        const auto opened = scan.open(path_);
        if (!opened.ok()) throw FormatError(context(path_, "MCAP scan failed to open file"));
        const auto scanned = scan.readSummary(mcap::ReadSummaryMethod::ForceScan);
        if (!scanned.ok()) throw FormatError(context(path_, "MCAP statistics scan failed: " + scanned.message));
        statistics = scan.statistics();
        if (!indexed_) { schemas_ = scan.schemas(); channels_ = scan.channels(); }
        if (indexed_ && statistics && (channels_.size() != statistics->channelCount || schemas_.size() != statistics->schemaCount)) {
          indexed_ = false;
          schemas_ = scan.schemas(); channels_ = scan.channels();
        }
      }
      if (!statistics) throw FormatError(context(path_, "MCAP statistics are unavailable"));
      for (const auto& item : channels_) {
        const auto& channel = *item.second;
        Connection connection;
        connection.id = channel.id;
        connection.topic = normalize_topic(channel.topic);
        connection.original_topic = channel.topic;
        connection.serialization_format = channel.messageEncoding;
        const auto schema = schemas_.find(channel.schemaId);
        if (schema != schemas_.end() && schema->second) {
          connection.type = normalize_type(schema->second->name);
          const auto encoding = schema->second->encoding;
          connection.definition.format = encoding == "ros2msg" ? DefinitionFormat::Msg :
              (encoding == "ros2idl" || encoding == "omgidl" ? DefinitionFormat::Idl : DefinitionFormat::None);
          connection.definition.data.assign(reinterpret_cast<const char*>(schema->second->data.data()), schema->second->data.size());
        }
        const auto count = statistics->channelMessageCounts.find(channel.id);
        connection.message_count = count == statistics->channelMessageCounts.end() ? 0 : count->second;
        connections_.push_back(std::move(connection));
      }
      std::sort(connections_.begin(), connections_.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
      for (std::size_t i=0; i<connections_.size(); ++i) channel_positions_.emplace(connections_[i].id, i);
      metadata_ = ReaderMetadata{};
      metadata_.storage = StorageKind::Mcap;
      metadata_.files = {path_};
      if (statistics->messageCount && statistics->messageEndTime == std::numeric_limits<std::uint64_t>::max())
        throw FormatError(context(path_, "MCAP message end time overflows uint64_t"));
      metadata_.start_time = statistics->messageStartTime;
      metadata_.end_time = statistics->messageEndTime + (statistics->messageCount ? 1 : 0);
      metadata_.duration = metadata_.end_time >= metadata_.start_time ? metadata_.end_time-metadata_.start_time : 0;
      metadata_.message_count = statistics->messageCount;
      if (metadata_.message_count && !indexed_) build_sorted_spool();
      open_ = true;
    } catch (...) { close(); throw; }
#else
    throw UnsupportedFeature("MCAP support is disabled: official mcap C++ SDK was not found");
#endif
  }
  void close() noexcept override {
#if ROSBAGS_HAS_MCAP
    reader_.close();
    schemas_.clear(); channels_.clear();
    spool_.reset();
    std::vector<SpoolEntry>{}.swap(entries_);
#endif
    connections_.clear(); channel_positions_.clear();
    indexed_ = false;
    open_ = false;
  }
  bool is_open() const noexcept override { return open_; }
  StorageKind kind() const override { ensure_open(); return StorageKind::Mcap; }
  const ReaderMetadata& metadata() const override { ensure_open(); return metadata_; }
  const std::vector<Connection>& connections() const override { ensure_open(); return connections_; }
  std::unique_ptr<BackendCursor> make_cursor(const ReadFilter& filter) const override;

 private:
  class Cursor;
  void ensure_open() const { if (!open_) throw RosbagsError("MCAP reader is not open"); }
#if ROSBAGS_HAS_MCAP
  struct SpoolEntry {
    std::uint64_t timestamp;
    std::uint64_t offset;
    std::uint64_t size;
    std::uint32_t channel;
  };
  void build_sorted_spool() {
    mcap::McapReader scan;
    if (!scan.open(path_).ok()) throw FormatError(context(path_, "MCAP scan failed to open file"));
    spool_ = std::make_unique<TemporaryFile>("rosbags_mcap");
    std::optional<mcap::Status> problem;
    mcap::ReadMessageOptions options;
    options.readOrder = mcap::ReadMessageOptions::ReadOrder::FileOrder;
    auto view = scan.readMessages([&](const mcap::Status& status) { if (!status.ok() && !problem) problem=status; }, options);
    std::uint64_t offset = 0;
    const auto maximum = options_.max_index_bytes/sizeof(SpoolEntry);
    for (const auto& item : view) {
      const auto& message = item.message;
      if (message.dataSize > options_.max_message_bytes)
        throw ResourceLimitError(context(path_, "MCAP message exceeds max_message_bytes"));
      if (offset >= options_.max_temporary_bytes || message.dataSize > options_.max_temporary_bytes-offset-1)
        throw ResourceLimitError(context(path_, "MCAP fallback exceeds max_temporary_bytes"));
      if (entries_.size() == maximum)
        throw ResourceLimitError(context(path_, "MCAP fallback exceeds max_index_bytes"));
      if (entries_.size() == entries_.capacity()) {
        const auto capacity = entries_.capacity();
        const auto growth = capacity > maximum/2 ? maximum : std::max<std::size_t>(1,capacity*2);
        entries_.reserve(growth);
      }
      if (!channel_positions_.count(message.channelId))
        throw FormatError(context(path_, "MCAP message refers to an unknown channel"));
      entries_.push_back({message.logTime,offset,message.dataSize,message.channelId});
      spool_->write(ByteView{reinterpret_cast<const Byte*>(message.data),static_cast<std::size_t>(message.dataSize)});
      const Byte separator=0;
      spool_->write(ByteView{&separator,1}); // Unique offset also orders zero-length messages.
      offset += message.dataSize+1;
    }
    if (problem) throw FormatError(context(path_, "MCAP message scan failed: " + problem->message));
    if (entries_.size() != metadata_.message_count)
      throw FormatError(context(path_, "MCAP statistics disagree with scanned message count"));
    spool_->close();
    std::sort(entries_.begin(),entries_.end(),[](const auto& a,const auto& b) {
      return a.timestamp < b.timestamp || (a.timestamp == b.timestamp && a.offset < b.offset);
    });
  }
#endif
  std::string path_;
  ReaderOptions options_;
  ReaderMetadata metadata_;
  std::vector<Connection> connections_;
  std::unordered_map<std::uint32_t,std::size_t> channel_positions_;
  bool open_ = false;
  bool indexed_ = false;
#if ROSBAGS_HAS_MCAP
  mcap::McapReader reader_;
  std::unordered_map<mcap::SchemaId,mcap::SchemaPtr> schemas_;
  std::unordered_map<mcap::ChannelId,mcap::ChannelPtr> channels_;
  std::unique_ptr<TemporaryFile> spool_;
  std::vector<SpoolEntry> entries_;
#endif
};

class McapBackend::Cursor final : public BackendCursor {
 public:
  Cursor(const McapBackend& owner, ReadFilter filter) : owner_(owner), filter_(std::move(filter)) {
    owner_.ensure_open();
#if ROSBAGS_HAS_MCAP
    if (!owner_.metadata_.message_count || (filter_.start && filter_.stop && *filter_.start >= *filter_.stop)) {
      empty_ = true;
      return;
    }
    if (!owner_.indexed_) {
      input_.open(owner_.spool_->path(),std::ios::binary);
      if (!input_) throw RosbagsError(context(owner_.path_, "cannot open MCAP fallback spool"));
      position_ = static_cast<std::size_t>(std::lower_bound(owner_.entries_.begin(),owner_.entries_.end(),filter_.start.value_or(0),
          [](const auto& entry,std::uint64_t time) { return entry.timestamp < time; })-owner_.entries_.begin());
      return;
    }
    if (!reader_.open(owner_.path_).ok()) throw FormatError(context(owner_.path_, "MCAP SDK failed to open file"));
    const auto summary = reader_.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
    if (!summary.ok() && summary.code != mcap::StatusCode::MissingStatistics)
      throw FormatError(context(owner_.path_, "MCAP summary scan failed: " + summary.message));
    mcap::ReadMessageOptions options;
    options.startTime = filter_.start.value_or(0);
    options.endTime = filter_.stop.value_or(mcap::MaxTime);
    options.readOrder = mcap::ReadMessageOptions::ReadOrder::LogTimeOrder;
    options.topicFilter = [this](std::string_view topic) {
      return filter_.topics.empty() || std::any_of(filter_.topics.begin(),filter_.topics.end(),[&](const auto& requested) {
        return topics_match(requested,normalize_topic(topic),filter_.topic_match,filter_.topic_namespace);
      });
    };
    view_.emplace(reader_.readMessages([this](const mcap::Status& status) { if (!status.ok() && !problem_) problem_=status; }, options));
    iterator_.emplace(view_->begin());
#else
    throw UnsupportedFeature("MCAP support is disabled: official mcap C++ SDK was not found");
#endif
  }
  bool next(Message& output) override {
    owner_.ensure_open();
    if (empty_) return false;
#if ROSBAGS_HAS_MCAP
    if (!owner_.indexed_) {
      while (position_ < owner_.entries_.size()) {
        const auto& entry = owner_.entries_[position_++];
        if (filter_.stop && entry.timestamp >= *filter_.stop) return false;
        const auto& connection = owner_.connections_.at(owner_.channel_positions_.at(entry.channel));
        if (!has_topic(connection,filter_)) continue;
        auto bytes = std::make_shared<Bytes>(static_cast<std::size_t>(entry.size));
        input_.seekg(static_cast<std::streamoff>(entry.offset));
        if (entry.size) input_.read(reinterpret_cast<char*>(bytes->data()),static_cast<std::streamsize>(entry.size));
        if (!input_) throw RosbagsError(context(owner_.path_, "cannot read MCAP fallback spool"));
        output = Message{std::move(bytes),entry.timestamp,&connection,owner_.path_};
        return true;
      }
      return false;
    }
    while (iterator_ && *iterator_ != view_->end()) {
      const auto& item = **iterator_;
      const auto found = owner_.channel_positions_.find(item.message.channelId);
      if (!item.channel || found == owner_.channel_positions_.end())
        throw FormatError(context(owner_.path_, "MCAP message refers to an unknown channel"));
      const auto& connection = owner_.connections_[found->second];
      if (!has_topic(connection,filter_)) { ++(*iterator_); continue; }
      const auto& message = item.message;
      if (message.dataSize > owner_.options_.max_message_bytes)
        throw ResourceLimitError(context(owner_.path_, "MCAP message exceeds max_message_bytes"));
      auto bytes = std::make_shared<Bytes>(static_cast<std::size_t>(message.dataSize));
      if (message.dataSize) std::memcpy(bytes->data(),message.data,message.dataSize);
      const auto timestamp = message.logTime;
      ++(*iterator_);
      output = Message{std::move(bytes),timestamp,&connection,owner_.path_};
      return true;
    }
    if (problem_) throw FormatError(context(owner_.path_, "MCAP message scan failed: " + problem_->message));
    return false;
#else
    (void)output;
    return false;
#endif
  }
 private:
  const McapBackend& owner_;
  ReadFilter filter_;
  bool empty_ = false;
#if ROSBAGS_HAS_MCAP
  std::ifstream input_;
  std::size_t position_ = 0;
  mcap::McapReader reader_;
  std::optional<mcap::LinearMessageView> view_;
  std::optional<mcap::LinearMessageView::Iterator> iterator_;
  std::optional<mcap::Status> problem_;
#endif
};

std::unique_ptr<BackendCursor> McapBackend::make_cursor(const ReadFilter& filter) const {
  return std::make_unique<Cursor>(*this,filter);
}
}  // namespace

std::unique_ptr<Backend> make_mcap_backend(const std::string& path, ReaderOptions options) {
  return std::make_unique<McapBackend>(path,options);
}
}  // namespace rosbags::internal
