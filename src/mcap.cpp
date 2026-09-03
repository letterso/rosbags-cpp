#include "internal.hpp"

#if ROSBAGS_HAS_MCAP
#include <mcap/reader.hpp>
#endif

namespace rosbags::internal {
namespace {

class McapBackend final : public Backend {
 public:
  explicit McapBackend(std::string path) : path_(std::move(path)) {}
  void open() override {
#if ROSBAGS_HAS_MCAP
    if (open_) throw RosbagsError("MCAP reader is already open");
    close();
    try {
    const auto status = reader_.open(path_);
    if (!status.ok()) throw FormatError(context(path_, "MCAP SDK failed to open file"));
    const auto summary = reader_.readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);
    if (!summary.ok()) throw FormatError(context(path_, "MCAP summary scan failed"));
    schemas_ = reader_.schemas();
    channels_ = reader_.channels();
    connections_.clear();
    const auto statistics = reader_.statistics();
    for (const auto& item : channels_) {
      const auto& channel = *item.second;
      Connection connection;
      connection.id = channel.id;
      connection.topic = normalize_topic(channel.topic);
      connection.serialization_format = channel.messageEncoding;
      const auto schema = schemas_.find(channel.schemaId);
      if (schema != schemas_.end() && schema->second) {
        connection.type = normalize_type(schema->second->name);
        const auto encoding = schema->second->encoding;
        connection.definition.format = encoding == "ros2msg" ? DefinitionFormat::Msg :
                                      (encoding == "ros2idl" || encoding == "omgidl" ? DefinitionFormat::Idl : DefinitionFormat::None);
        connection.definition.data.assign(reinterpret_cast<const char*>(schema->second->data.data()), schema->second->data.size());
      }
      if (statistics) {
        const auto count = statistics->channelMessageCounts.find(channel.id);
        connection.message_count = count == statistics->channelMessageCounts.end() ? 0 : count->second;
      }
      connections_.push_back(std::move(connection));
    }
    metadata_ = ReaderMetadata{};
    metadata_.storage = StorageKind::Mcap;
    metadata_.files = {path_};
    if (statistics) {
      if (statistics->messageCount && statistics->messageEndTime == std::numeric_limits<std::uint64_t>::max())
        throw FormatError(context(path_, "MCAP message end time overflows uint64_t"));
      metadata_.start_time = statistics->messageStartTime;
      metadata_.end_time = statistics->messageEndTime + (statistics->messageCount ? 1 : 0);
      metadata_.duration = metadata_.end_time >= metadata_.start_time ? metadata_.end_time - metadata_.start_time : 0;
      metadata_.message_count = statistics->messageCount;
    }
    open_ = true;
    } catch (...) {
      close();
      throw;
    }
#else
    throw UnsupportedFeature("MCAP support is disabled: official mcap C++ SDK was not found");
#endif
  }
  void close() noexcept override {
#if ROSBAGS_HAS_MCAP
    reader_.close();
    schemas_.clear();
    channels_.clear();
#endif
    open_ = false;
  }
  bool is_open() const noexcept override { return open_; }
  StorageKind kind() const override { ensure_open(); return StorageKind::Mcap; }
  const ReaderMetadata& metadata() const override { ensure_open(); return metadata_; }
  const std::vector<Connection>& connections() const override { ensure_open(); return connections_; }
  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const override {
    ensure_open();
#if ROSBAGS_HAS_MCAP
    mcap::ReadMessageOptions options;
    options.startTime = filter.start.value_or(0);
    options.endTime = filter.stop.value_or(mcap::MaxTime);
    options.readOrder = mcap::ReadMessageOptions::ReadOrder::LogTimeOrder;
    if (!filter.topics.empty()) {
      options.topicFilter = [&filter](std::string_view topic) {
        const auto normalized = normalize_topic(topic);
        return std::any_of(filter.topics.begin(), filter.topics.end(), [&](const auto& requested) {
          return normalize_topic(requested) == normalized;
        });
      };
    }
    std::optional<mcap::Status> problem;
    const auto on_problem = [&problem](const mcap::Status& status) {
      if (!status.ok() && !problem) problem = status;
    };
    for (const auto& view : reader_.readMessages(on_problem, options)) {
      const auto& channel = view.channel;
      if (!channel) continue;
      const auto id = channel->id;
      const auto it = std::find_if(connections_.begin(), connections_.end(), [&](const auto& value) { return value.id == id; });
      if (it == connections_.end() || !has_topic(*it, filter)) continue;
      const auto& message = view.message;
      auto bytes = std::make_shared<Bytes>(reinterpret_cast<const Byte*>(message.data), reinterpret_cast<const Byte*>(message.data) + message.dataSize);
      callback(Message{std::move(bytes), message.logTime, &*it});
    }
    if (problem) throw FormatError(context(path_, "MCAP message scan failed: " + problem->message));
#else
    (void)filter;
    (void)callback;
#endif
  }

 private:
  void ensure_open() const { if (!open_) throw RosbagsError("MCAP reader is not open"); }
  std::string path_;
  ReaderMetadata metadata_;
  std::vector<Connection> connections_;
  bool open_ = false;
#if ROSBAGS_HAS_MCAP
  mutable mcap::McapReader reader_;
  std::unordered_map<mcap::SchemaId, mcap::SchemaPtr> schemas_;
  std::unordered_map<mcap::ChannelId, mcap::ChannelPtr> channels_;
#endif
};

}  // namespace

std::unique_ptr<Backend> make_mcap_backend(const std::string& path) {
  return std::make_unique<McapBackend>(path);
}

}  // namespace rosbags::internal
