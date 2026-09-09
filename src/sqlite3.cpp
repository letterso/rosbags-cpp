#include "internal.hpp"

#include <sqlite3.h>
#include <zstd.h>

#include <cstdio>
#include <cctype>
#include <cerrno>
#include <unistd.h>

namespace rosbags::internal {
namespace {

std::string sqlite_error(sqlite3* database, std::string_view prefix) {
  return std::string(prefix) + ": " + (database ? sqlite3_errmsg(database) : "SQLite error");
}

struct SqliteHandle {
  sqlite3* value = nullptr;
  ~SqliteHandle() { if (value) sqlite3_close(value); }
  void reset() noexcept {
    if (value) sqlite3_close(value);
    value = nullptr;
  }
};

struct SqliteStatement {
  sqlite3_stmt* value = nullptr;
  ~SqliteStatement() { if (value) sqlite3_finalize(value); }
  void reset() noexcept {
    if (value) sqlite3_finalize(value);
    value = nullptr;
  }
};

std::string node_string(const YAML::Node& node, const char* key) {
  const auto value = node[key];
  return value && !value.IsNull() ? value.as<std::string>() : std::string{};
}

std::uint64_t node_uint64(const YAML::Node& node, const char* key, std::uint64_t fallback = 0) {
  const auto value = node[key];
  return value && !value.IsNull() ? value.as<std::uint64_t>() : fallback;
}

Bytes zstd_bytes(ByteView compressed) {
  const auto size = ZSTD_getFrameContentSize(compressed.data, compressed.size);
  if (size == ZSTD_CONTENTSIZE_ERROR)
    throw FormatError("invalid zstd frame");
  if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
    ZSTD_DStream* stream = ZSTD_createDStream();
    if (!stream) throw RosbagsError("cannot create zstd decompressor");
    const std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)> stream_guard(
        stream, &ZSTD_freeDStream);
    if (ZSTD_isError(ZSTD_initDStream(stream)))
      throw FormatError("zstd decompression initialization failed");
    Bytes output;
    std::array<Byte, 64 * 1024> buffer{};
    ZSTD_inBuffer input{compressed.data, compressed.size, 0};
    for (;;) {
      const auto previous_input = input.pos;
      ZSTD_outBuffer destination{buffer.data(), buffer.size(), 0};
      const auto result = ZSTD_decompressStream(stream, &destination, &input);
      if (ZSTD_isError(result)) throw FormatError("zstd decompression failed");
      output.insert(output.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(destination.pos));
      if (result == 0 && input.pos == input.size) break;
      if (input.pos == previous_input && destination.pos == 0) {
        if (input.pos == input.size) throw FormatError("truncated zstd frame");
        throw FormatError("zstd decompressor made no progress");
      }
    }
    return output;
  }
  if (size > std::numeric_limits<std::size_t>::max())
    throw FormatError("cannot determine zstd frame size");
  Bytes output(static_cast<std::size_t>(size));
  const auto result = ZSTD_decompress(output.data(), output.size(), compressed.data, compressed.size);
  if (ZSTD_isError(result) || result != output.size()) throw FormatError("zstd decompression failed");
  return output;
}

class SqliteBackend final : public Backend {
 public:
  explicit SqliteBackend(std::string path) : path_(std::move(path)) {}
  void open() override {
    if (open_) throw RosbagsError("SQLite reader is already open");
    database_.reset();
    connections_.clear();
    topic_to_connection_.clear();
    definition_digests_.clear();
    metadata_ = ReaderMetadata{};
    sqlite3* raw = nullptr;
    const auto result = sqlite3_open_v2(path_.c_str(), &raw, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr);
    database_.value = raw;
    if (result != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot open SQLite database"));
    sqlite3_busy_timeout(database_.value, 5000);
    if (!has_table("messages") || !has_table("topics")) throw FormatError(sqlite_error(database_.value, "database missing topics/messages tables"));
    schema_ = detect_schema();
    load_connections();
    load_metadata();
    metadata_.files = {path_};
    metadata_.storage = StorageKind::Sqlite3;
    open_ = true;
  }
  void close() noexcept override {
    open_ = false;
    connections_.clear();
    topic_to_connection_.clear();
    database_.reset();
  }
  bool is_open() const noexcept override { return open_; }
  StorageKind kind() const override { ensure_open(); return StorageKind::Sqlite3; }
  const ReaderMetadata& metadata() const override { ensure_open(); return metadata_; }
  const std::vector<Connection>& connections() const override { ensure_open(); return connections_; }
  std::unique_ptr<BackendCursor> make_cursor(const ReadFilter& filter) const override;

 private:
  class Cursor;
  bool has_table(const char* name) const {
    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?";
    if (sqlite3_prepare_v2(database_.value, sql, -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot inspect SQLite schema"));
    sqlite3_bind_text(statement, 1, name, -1, SQLITE_STATIC);
    const auto result = sqlite3_step(statement);
    const auto count = result == SQLITE_ROW ? sqlite3_column_int(statement, 0) : 0;
    sqlite3_finalize(statement);
    if (result != SQLITE_ROW) throw RosbagsError(sqlite_error(database_.value, "cannot inspect SQLite schema"));
    return count == 1;
  }
  int detect_schema() const {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.value, "PRAGMA table_info(schema)", -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot inspect schema table"));
    bool has_schema_table = false;
    while (sqlite3_step(statement) == SQLITE_ROW) has_schema_table = true;
    sqlite3_finalize(statement);
    if (has_schema_table) {
      if (sqlite3_prepare_v2(database_.value, "SELECT schema_version FROM schema LIMIT 1", -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot read schema version"));
      const auto result = sqlite3_step(statement);
      const auto version = result == SQLITE_ROW ? sqlite3_column_int(statement, 0) : -1;
      sqlite3_finalize(statement);
      if (result != SQLITE_ROW || version < 1) throw FormatError("invalid SQLite schema version");
      return version;
    }
    if (sqlite3_prepare_v2(database_.value, "PRAGMA table_info(topics)", -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot inspect topics table"));
    bool qos = false;
    while (sqlite3_step(statement) == SQLITE_ROW) {
      const auto* name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
      if (name && std::string_view(name) == "offered_qos_profiles") qos = true;
    }
    sqlite3_finalize(statement);
    return qos ? 2 : 1;
  }
  std::unordered_map<std::string, MessageDefinition> definitions() const {
    std::unordered_map<std::string, MessageDefinition> result;
    if (schema_ < 4 || !has_table("message_definitions")) return result;
    sqlite3_stmt* statement = nullptr;
    const auto sql = "SELECT topic_type,encoding,encoded_message_definition,type_description_hash FROM message_definitions ORDER BY id";
    if (sqlite3_prepare_v2(database_.value, sql, -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot read message definitions"));
    while (sqlite3_step(statement) == SQLITE_ROW) {
      const auto* type = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
      const auto* encoding = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
      const auto* data = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
      if (!type || !encoding || !data) continue;
      const auto format = std::string_view(encoding) == "ros2msg" ? DefinitionFormat::Msg :
                          (std::string_view(encoding) == "ros2idl" ? DefinitionFormat::Idl : DefinitionFormat::None);
      result[normalize_type(type)] = {format, data};
      definition_digests_[normalize_type(type)] = reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)) ? reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)) : "";
    }
    sqlite3_finalize(statement);
    return result;
  }
  void load_connections() {
    const auto defs = definitions();
    const char* sql_v1 = "SELECT topics.id,name,type,count(messages.id),serialization_format FROM topics LEFT JOIN messages ON topics.id=messages.topic_id GROUP BY topics.id ORDER BY topics.id";
    const char* sql_v2 = "SELECT topics.id,name,type,count(messages.id),serialization_format,offered_qos_profiles FROM topics LEFT JOIN messages ON topics.id=messages.topic_id GROUP BY topics.id ORDER BY topics.id";
    const char* sql_v4 = "SELECT topics.id,name,type,count(messages.id),serialization_format,offered_qos_profiles,type_description_hash FROM topics LEFT JOIN messages ON topics.id=messages.topic_id GROUP BY topics.id ORDER BY topics.id";
    const char* sql = schema_ >= 4 ? sql_v4 : (schema_ >= 2 ? sql_v2 : sql_v1);
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.value, sql, -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot read SQLite topics"));
    while (sqlite3_step(statement) == SQLITE_ROW) {
      Connection connection;
      connection.id = static_cast<std::uint32_t>(sqlite3_column_int64(statement, 0));
      const auto* topic = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
      const auto* type = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
      const auto* format = reinterpret_cast<const char*>(sqlite3_column_text(statement, 4));
      connection.topic = topic ? normalize_topic(topic) : "";
      connection.original_topic = topic ? topic : "";
      connection.type = type ? normalize_type(type) : "";
      connection.message_count = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 3));
      connection.serialization_format = format ? format : "";
      const auto definition = defs.find(connection.type);
      if (definition != defs.end()) connection.definition = definition->second;
      if (schema_ >= 4) {
        const auto* digest = reinterpret_cast<const char*>(sqlite3_column_text(statement, 6));
        connection.digest = digest ? digest : "";
      }
      if (schema_ >= 2) {
        const auto* qos = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5));
        if (qos && *qos) connection.qos_profiles.push_back({qos});
      }
      topic_to_connection_[connection.id] = connections_.size();
      connections_.push_back(std::move(connection));
    }
    sqlite3_finalize(statement);
  }
  void load_metadata() {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.value, "SELECT MIN(timestamp),MAX(timestamp),COUNT(*) FROM messages", -1, &statement, nullptr) != SQLITE_OK) throw RosbagsError(sqlite_error(database_.value, "cannot read SQLite message metadata"));
    const auto result = sqlite3_step(statement);
    if (result != SQLITE_ROW) {
      sqlite3_finalize(statement);
      throw RosbagsError(sqlite_error(database_.value, "cannot read SQLite message metadata"));
    }
    const auto count = sqlite3_column_int64(statement, 2);
    metadata_.message_count = count < 0 ? 0 : static_cast<std::uint64_t>(count);
    if (metadata_.message_count) {
      const auto start = sqlite3_column_int64(statement, 0);
      const auto end = sqlite3_column_int64(statement, 1);
      if (start < 0 || end < 0) {
        sqlite3_finalize(statement);
        throw FormatError("negative SQLite timestamp cannot be represented by uint64_t");
      }
      metadata_.start_time = static_cast<std::uint64_t>(start);
      if (static_cast<std::uint64_t>(end) == std::numeric_limits<std::uint64_t>::max()) {
        sqlite3_finalize(statement);
        throw FormatError("SQLite timestamp overflows metadata end time");
      }
      metadata_.end_time = static_cast<std::uint64_t>(end) + 1;
      metadata_.duration = metadata_.end_time - metadata_.start_time;
    }
    sqlite3_finalize(statement);
  }
  const Connection* find_connection(std::uint32_t id) const {
    const auto it = topic_to_connection_.find(id);
    return it == topic_to_connection_.end() ? nullptr : &connections_[it->second];
  }
  void ensure_open() const { if (!open_) throw RosbagsError("SQLite reader is not open"); }

  std::string path_;
  mutable SqliteHandle database_;
  std::vector<Connection> connections_;
  std::unordered_map<std::uint32_t, std::size_t> topic_to_connection_;
  mutable std::unordered_map<std::string, std::string> definition_digests_;
  ReaderMetadata metadata_;
  int schema_ = 0;
  bool open_ = false;
};

class SqliteBackend::Cursor final : public BackendCursor {
 public:
  Cursor(const SqliteBackend& owner, const ReadFilter& filter) : owner_(owner) {
    owner_.ensure_open();
    std::string sql = "SELECT topics.id,messages.timestamp,messages.data FROM messages JOIN topics ON messages.topic_id=topics.id";
    std::vector<std::uint32_t> topic_ids;
    for (const auto& connection : owner_.connections_)
      if (has_topic(connection, filter)) topic_ids.push_back(connection.id);
    if (!filter.topics.empty() || !filter.connection_ids.empty()) {
      if (topic_ids.empty()) return;
      sql += " WHERE topics.id IN (";
      for (std::size_t index = 0; index < topic_ids.size(); ++index) sql += index ? ",?" : "?";
      sql += ")";
    }
    if (filter.start) sql += (sql.find(" WHERE ") == std::string::npos ? " WHERE" : " AND") + std::string(" messages.timestamp >= ?");
    if (filter.stop) sql += (sql.find(" WHERE ") == std::string::npos ? " WHERE" : " AND") + std::string(" messages.timestamp < ?");
    sql += " ORDER BY messages.timestamp,messages.id";
    if (sqlite3_prepare_v2(owner_.database_.value, sql.c_str(), -1, &statement_.value, nullptr) != SQLITE_OK)
      throw RosbagsError(sqlite_error(owner_.database_.value, "cannot prepare SQLite message query"));
    int parameter = 1;
    for (const auto id : topic_ids) sqlite3_bind_int64(statement_.value, parameter++, static_cast<sqlite3_int64>(id));
    if (filter.start) sqlite3_bind_int64(statement_.value, parameter++, static_cast<sqlite3_int64>(*filter.start));
    if (filter.stop) sqlite3_bind_int64(statement_.value, parameter++, static_cast<sqlite3_int64>(*filter.stop));
  }

  bool next(Message& output) override {
    owner_.ensure_open();
    if (!statement_.value) return false;
    const auto result = sqlite3_step(statement_.value);
    if (result == SQLITE_DONE) {
      statement_.reset();
      return false;
    }
    if (result != SQLITE_ROW) throw RosbagsError(sqlite_error(owner_.database_.value, "SQLite message query failed"));
    const auto id = static_cast<std::uint32_t>(sqlite3_column_int64(statement_.value, 0));
    const auto timestamp = sqlite3_column_int64(statement_.value, 1);
    if (timestamp < 0) throw FormatError("negative SQLite timestamp cannot be represented by uint64_t");
    const auto* connection = owner_.find_connection(id);
    if (!connection) throw FormatError("message references unknown SQLite topic id");
    const auto* data = static_cast<const Byte*>(sqlite3_column_blob(statement_.value, 2));
    const auto size = sqlite3_column_bytes(statement_.value, 2);
    if (size < 0 || (size != 0 && !data)) throw FormatError("invalid SQLite message blob");
    auto bytes = std::make_shared<Bytes>();
    if (size) bytes->assign(data, data + size);
    output = Message{std::move(bytes), static_cast<std::uint64_t>(timestamp), connection, owner_.path_};
    return true;
  }

 private:
  const SqliteBackend& owner_;
  SqliteStatement statement_;
};

std::unique_ptr<BackendCursor> SqliteBackend::make_cursor(const ReadFilter& filter) const {
  return std::make_unique<Cursor>(*this, filter);
}

class DirectoryBackend final : public Backend {
 public:
  explicit DirectoryBackend(std::string path) : path_(std::move(path)) {}
  ~DirectoryBackend() override { close_impl(); }
  void open() override {
    if (open_) throw RosbagsError("ROS2 directory reader is already open");
    // A failed open may have left child readers or temporary files behind.
    close();
    try {
      const auto metadata_path = std::filesystem::path(path_) / "metadata.yaml";
      YAML::Node root;
      try {
        root = YAML::LoadFile(metadata_path.string());
      } catch (const std::exception& error) {
        throw FormatError(context(metadata_path.string(), error.what()));
      }
      const auto info = root["rosbag2_bagfile_information"];
      if (!info) throw FormatError(context(metadata_path.string(), "missing rosbag2_bagfile_information"));
      const auto version = info["version"] ? info["version"].as<int>() : -1;
      if (version < 0 || version > 9) throw UnsupportedFeature("unsupported rosbag2 metadata version");
      auto storage = node_string(info, "storage_identifier");
      std::transform(storage.begin(), storage.end(), storage.begin(),
                     [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
      if (storage != "sqlite3" && storage != "mcap")
        throw UnsupportedFeature("unsupported rosbag2 storage identifier: " + storage);
      const auto paths = info["relative_file_paths"];
      if (!paths || !paths.IsSequence() || paths.size() == 0)
        throw FormatError("rosbag2 metadata has no relative_file_paths");

      connections_.clear();
      const auto topics = info["topics_with_message_count"];
      if (topics && topics.IsSequence()) {
        std::uint32_t id = 1;
        for (const auto& item : topics) {
          const auto topic = item["topic_metadata"];
          Connection connection;
          connection.id = id++;
          connection.original_topic = node_string(topic, "name");
          connection.topic = normalize_topic(connection.original_topic);
          connection.type = normalize_type(node_string(topic, "type"));
          connection.serialization_format = node_string(topic, "serialization_format");
          connection.message_count = node_uint64(item, "message_count");
          connection.digest = node_string(topic, "type_description_hash");
          const auto qos = topic["offered_qos_profiles"];
          if (qos && qos.IsScalar() && !qos.as<std::string>().empty())
            connection.qos_profiles.push_back({qos.as<std::string>()});
          connections_.push_back(std::move(connection));
        }
      }
      compression_mode_ = node_string(info, "compression_mode");
      compression_format_ = node_string(info, "compression_format");
      std::transform(compression_mode_.begin(), compression_mode_.end(), compression_mode_.begin(),
                     [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
      std::transform(compression_format_.begin(), compression_format_.end(), compression_format_.begin(),
                     [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
      if (compression_mode_ == "none") compression_mode_.clear();
      if (!compression_mode_.empty() && compression_mode_ != "file" && compression_mode_ != "message")
        throw UnsupportedFeature("unsupported rosbag2 compression mode: " + compression_mode_);
      if (!compression_mode_.empty() && compression_format_ != "zstd")
        throw UnsupportedFeature("unsupported rosbag2 compression format: " + compression_format_);
      metadata_.storage = storage == "sqlite3" ? StorageKind::Sqlite3 : StorageKind::Mcap;
      metadata_.message_count = node_uint64(info, "message_count");
      const auto start = info["starting_time"];
      metadata_.start_time = start ? node_uint64(start, "nanoseconds_since_epoch") : 0;
      const auto duration = info["duration"];
      const auto recorded_duration = duration ? node_uint64(duration, "nanoseconds") : 0;
      if (metadata_.message_count && recorded_duration == std::numeric_limits<std::uint64_t>::max())
        throw FormatError("rosbag2 metadata duration overflows end time");
      metadata_.duration = metadata_.message_count ? recorded_duration + 1 : 0;
      if (metadata_.start_time > std::numeric_limits<std::uint64_t>::max() - metadata_.duration)
        throw FormatError("rosbag2 metadata end time overflows uint64_t");
      metadata_.end_time = metadata_.start_time + metadata_.duration;
      if (!metadata_.message_count) metadata_.start_time = metadata_.end_time = 0;
      metadata_.compression_mode = compression_mode_;
      metadata_.compression_format = compression_format_;
      metadata_.files.clear();

      for (const auto& item : paths) {
        const auto relative = std::filesystem::path(item.as<std::string>());
        if (relative.empty() || relative.is_absolute())
          throw FormatError("rosbag2 relative_file_paths contains an invalid path");
        // Match rosbag2's storage convention: metadata paths are basenames.
        const auto input = std::filesystem::path(path_) / relative.filename();
        if (!std::filesystem::exists(input)) {
          FormatError error("storage file is missing");
          ErrorContext details;
          details.source_path = input.string();
          error.add_context(details);
          throw error;
        }
        std::string actual = input.string();
        std::unique_ptr<Backend> backend;
        try {
          if (compression_mode_ == "file") actual = decompress_file(input);
          backend = storage == "sqlite3" ? make_sqlite_backend(actual) : make_mcap_backend(actual);
          backend->open();
        } catch (RosbagsError& error) {
          ErrorContext context;
          context.source_path = input.string();
          error.add_context(context);
          throw;
        }
        backends_.push_back(std::move(backend));
        metadata_.files.push_back(actual);
      }
      for (auto& connection : connections_) {
        for (const auto& backend : backends_) {
          const auto it =
              std::find_if(backend->connections().begin(), backend->connections().end(), [&](const auto& candidate) {
                return candidate.topic == connection.topic && candidate.type == connection.type;
              });
          if (it != backend->connections().end() && it->definition.format != DefinitionFormat::None) {
            connection.definition = it->definition;
            break;
          }
        }
      }
      open_ = true;
    } catch (...) {
      close_impl();
      throw;
    }
  }
  void close() noexcept override { close_impl(); }
  bool is_open() const noexcept override { return open_; }
  StorageKind kind() const override { ensure_open(); return metadata_.storage; }
  const ReaderMetadata& metadata() const override { ensure_open(); return metadata_; }
  const std::vector<Connection>& connections() const override { ensure_open(); return connections_; }
  std::unique_ptr<BackendCursor> make_cursor(const ReadFilter& filter) const override;

 private:
  class Cursor;
  void close_impl() noexcept {
    for (auto it = backends_.rbegin(); it != backends_.rend(); ++it) (*it)->close();
    backends_.clear();
    for (const auto& file : temporary_files_) std::remove(file.c_str());
    temporary_files_.clear();
    open_ = false;
  }
  std::string decompress_file(const std::filesystem::path& input) {
    const auto compressed = read_file(input);
    const auto bytes = zstd_bytes(ByteView{compressed.data(), compressed.size()});
    auto pattern = (std::filesystem::temp_directory_path() / "rosbags_cpp_XXXXXX").string();
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    int descriptor = ::mkstemp(buffer.data());
    if (descriptor < 0)
      throw RosbagsError(context(pattern, std::string("cannot create temporary file: ") + std::strerror(errno)));
    const std::string output(buffer.data());
    try {
      std::size_t offset = 0;
      while (offset < bytes.size()) {
        const auto remaining = std::min(
            bytes.size() - offset,
            static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
        const auto count = ::write(descriptor, bytes.data() + offset, remaining);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0)
          throw RosbagsError(context(output, std::string("cannot write decompressed storage file: ") +
                                               std::strerror(errno)));
        if (count == 0)
          throw RosbagsError(context(output, "cannot write decompressed storage file: wrote zero bytes"));
        offset += static_cast<std::size_t>(count);
      }
      if (::close(descriptor) != 0) {
        descriptor = -1;
        throw RosbagsError(context(output, std::string("cannot close decompressed storage file: ") +
                                             std::strerror(errno)));
      }
      descriptor = -1;
      temporary_files_.push_back(output);
      return output;
    } catch (...) {
      if (descriptor >= 0) ::close(descriptor);
      std::remove(output.c_str());
      throw;
    }
  }
  void ensure_open() const { if (!open_) throw RosbagsError("ROS2 directory reader is not open"); }

  std::string path_;
  std::vector<std::unique_ptr<Backend>> backends_;
  std::vector<Connection> connections_;
  mutable std::vector<std::string> temporary_files_;
  ReaderMetadata metadata_;
  std::string compression_format_;
  std::string compression_mode_;
  bool open_ = false;
};

class DirectoryBackend::Cursor final : public BackendCursor {
 public:
  Cursor(const DirectoryBackend& owner, const ReadFilter& filter) : owner_(owner), filter_(filter) {
    owner_.ensure_open();
    for (const auto& backend : owner_.backends_) {
      ReadFilter local = filter;
      local.connection_ids.clear();
      sources_.push_back({backend->make_cursor(local), std::nullopt});
    }
    for (std::size_t index = 0; index < sources_.size(); ++index) pull(index);
  }

  bool next(Message& output) override {
    owner_.ensure_open();
    if (pending_source_) {
      pull(*pending_source_);
      pending_source_.reset();
    }
    if (heap_.empty()) return false;
    const auto item = heap_.top();
    heap_.pop();
    auto& source = sources_[item.source];
    auto message = std::move(*source.head);
    source.head.reset();
    const auto* connection = map_connection(message.connection);
    if (!connection) throw FormatError("message references unknown ROS2 directory topic");
    if (owner_.compression_mode_ == "message") {
      message.bytes = std::make_shared<Bytes>(zstd_bytes(ByteView{message.bytes->data(), message.bytes->size()}));
    }
    message.connection = connection;
    output = std::move(message);
    pending_source_ = item.source;
    return true;
  }

 private:
  struct Source {
    std::unique_ptr<BackendCursor> cursor;
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

  const Connection* map_connection(const Connection* child) const {
    if (!child) return nullptr;
    const auto it = std::find_if(owner_.connections_.begin(), owner_.connections_.end(), [&](const auto& connection) {
      return connection.topic == child->topic && connection.type == child->type;
    });
    return it == owner_.connections_.end() ? nullptr : &*it;
  }

  void pull(std::size_t source_index) {
    auto& source = sources_[source_index];
    Message message;
    try {
      while (source.cursor->next(message)) {
        const auto* connection = map_connection(message.connection);
        if (!connection || !has_topic(*connection, filter_) || !in_time(message.timestamp, filter_)) continue;
        source.head = std::move(message);
        heap_.push({source.head->timestamp, source_index});
        return;
      }
    } catch (RosbagsError& error) {
      ErrorContext context;
      context.source_path = owner_.metadata_.files.at(source_index);
      error.add_context(context);
      throw;
    }
  }

  const DirectoryBackend& owner_;
  ReadFilter filter_;
  std::vector<Source> sources_;
  std::priority_queue<HeapItem, std::vector<HeapItem>, HeapCompare> heap_;
  std::optional<std::size_t> pending_source_;
};

std::unique_ptr<BackendCursor> DirectoryBackend::make_cursor(const ReadFilter& filter) const {
  return std::make_unique<Cursor>(*this, filter);
}

}  // namespace

std::unique_ptr<Backend> make_sqlite_backend(const std::string& path) { return std::make_unique<SqliteBackend>(path); }
std::unique_ptr<Backend> make_directory_backend(const std::string& path) { return std::make_unique<DirectoryBackend>(path); }

}  // namespace rosbags::internal
