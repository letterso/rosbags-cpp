#include "internal.hpp"

#include <lz4frame.h>

#if ROSBAGS_HAS_BZ2
#include <bzlib.h>
#endif

#include <map>

namespace rosbags::internal {
namespace {

using Fields = std::unordered_map<std::string, Bytes>;

struct RecordHeader {
  Fields fields;
};

struct Record {
  RecordHeader header;
  ByteView body;
};

std::uint64_t field_u64(const Fields& fields, const char* name) {
  const auto it = fields.find(name);
  if (it == fields.end() || it->second.size() != 8) throw FormatError(std::string("missing/invalid field ") + name);
  return u64(it->second.data());
}
std::uint32_t field_u32(const Fields& fields, const char* name) {
  const auto it = fields.find(name);
  if (it == fields.end() || it->second.size() != 4) throw FormatError(std::string("missing/invalid field ") + name);
  return u32(it->second.data());
}
std::uint8_t field_u8(const Fields& fields, const char* name) {
  const auto it = fields.find(name);
  if (it == fields.end() || it->second.size() != 1) throw FormatError(std::string("missing/invalid field ") + name);
  return it->second[0];
}
std::string field_string(const Fields& fields, const char* name, bool required = true) {
  const auto it = fields.find(name);
  if (it == fields.end()) {
    if (required) throw FormatError(std::string("missing field ") + name);
    return {};
  }
  return std::string(reinterpret_cast<const char*>(it->second.data()), it->second.size());
}

RecordHeader read_fields(ByteView input, std::size_t& pos, std::size_t end) {
  if (end > input.size || pos > end) throw FormatError("rosbag header exceeds input");
  RecordHeader result;
  while (pos < end) {
    if (end - pos < 4) throw FormatError("truncated rosbag header field length");
    const auto field_size = u32(input.data + pos);
    pos += 4;
    if (field_size > end - pos) throw FormatError("rosbag header field exceeds header");
    const auto field = ByteView{input.data + pos, field_size};
    pos += field_size;
    const auto equals = static_cast<std::size_t>(std::find(field.data, field.data + field.size, '=') - field.data);
    if (equals == field.size || equals == 0) throw FormatError("malformed rosbag header field");
    result.fields[std::string(reinterpret_cast<const char*>(field.data), equals)] =
        Bytes(field.data + equals + 1, field.data + field.size);
  }
  if (pos != end) throw FormatError("rosbag header cursor mismatch");
  return result;
}

RecordHeader read_header(ByteView input, std::size_t& pos) {
  if (pos + 4 > input.size) throw FormatError("truncated rosbag header length");
  const auto size = u32(input.data + pos);
  pos += 4;
  if (size > input.size - pos) throw FormatError("rosbag header exceeds input");
  return read_fields(input, pos, pos + size);
}

Record read_record(ByteView input, std::size_t& pos, std::optional<std::uint8_t> expected = {}) {
  const auto header = read_header(input, pos);
  const auto op = field_u8(header.fields, "op");
  if (expected && op != *expected) throw FormatError("unexpected rosbag record opcode");
  if (pos + 4 > input.size) throw FormatError("truncated rosbag record size");
  const auto size = u32(input.data + pos);
  pos += 4;
  if (size > input.size - pos) throw FormatError("rosbag record body exceeds input");
  const auto body = ByteView{input.data + pos, size};
  pos += size;
  return {header, body};
}

std::uint64_t ros_time(const Bytes& value) {
  if (value.size() != 8) throw FormatError("invalid rosbag time field");
  return static_cast<std::uint64_t>(u32(value.data())) * 1000000000ULL + u32(value.data() + 4);
}

Bytes decompress_lz4(ByteView compressed) {
  LZ4F_dctx* context = nullptr;
  const auto code = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
  if (LZ4F_isError(code)) throw FormatError("cannot create LZ4 decompressor");
  std::vector<Byte> output;
  std::array<Byte, 64 * 1024> buffer{};
  std::size_t source_pos = 0;
  bool finished = false;
  while (source_pos < compressed.size) {
    std::size_t source_size = compressed.size - source_pos;
    std::size_t destination_size = buffer.size();
    const auto result = LZ4F_decompress(context, buffer.data(), &destination_size,
                                        compressed.data + source_pos, &source_size, nullptr);
    if (LZ4F_isError(result)) {
      LZ4F_freeDecompressionContext(context);
      throw FormatError("LZ4 decompression failed");
    }
    output.insert(output.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(destination_size));
    source_pos += source_size;
    if (result == 0) {
      finished = true;
      break;
    }
    if (source_size == 0 && destination_size == 0) {
      LZ4F_freeDecompressionContext(context);
      throw FormatError("LZ4 decompressor made no progress");
    }
  }
  LZ4F_freeDecompressionContext(context);
  if (!finished) throw FormatError("truncated LZ4 compressed chunk");
  return output;
}

Bytes decompress_chunk(std::string_view compression, ByteView data) {
  if (compression.empty() || compression == "none") return Bytes(data.data, data.data + data.size);
  if (compression == "lz4") return decompress_lz4(data);
  if (compression == "bz2") {
#if ROSBAGS_HAS_BZ2
    if (data.size > std::numeric_limits<unsigned int>::max()) throw FormatError("BZ2 input is too large");
    unsigned int output_size = static_cast<unsigned int>(std::max<std::size_t>(data.size * 4, 1024));
    for (;;) {
      Bytes output(output_size);
      auto result = BZ2_bzBuffToBuffDecompress(reinterpret_cast<char*>(output.data()), &output_size,
                                               const_cast<char*>(reinterpret_cast<const char*>(data.data)),
                                               static_cast<unsigned int>(data.size), 0, 0);
      if (result == BZ_OK) {
        output.resize(output_size);
        return output;
      }
      if (result != BZ_OUTBUFF_FULL || output_size > std::numeric_limits<unsigned int>::max() / 2)
        throw FormatError("BZ2 decompression failed");
      output_size *= 2;
    }
#else
    throw UnsupportedFeature("BZ2 support is disabled (libbz2 development files not found)");
#endif
  }
  throw UnsupportedFeature("unsupported ROS1 chunk compression: " + std::string(compression));
}

struct ChunkInfo {
  std::uint64_t position = 0;
  std::uint64_t start = 0;
  std::uint64_t end = 0;
  std::map<std::uint32_t, std::uint32_t> counts;
};
struct Chunk {
  std::uint64_t data_position = 0;
  std::uint32_t data_size = 0;
  std::string compression;
};
struct IndexEntry {
  std::uint64_t timestamp = 0;
  std::uint64_t chunk_position = 0;
  std::uint32_t offset = 0;
  std::uint64_t order = 0;
};

class Rosbag1Backend final : public Backend {
 public:
  explicit Rosbag1Backend(std::string path) : path_(std::move(path)) {}
  void open() override {
    if (open_) throw RosbagsError("ROS1 reader is already open");
    data_ = read_file(path_);
    ByteView view{data_.data(), data_.size()};
    const auto magic_end = std::find(data_.begin(), data_.end(), static_cast<Byte>('\n'));
    if (magic_end == data_.end()) throw FormatError(context(path_, "missing ROS1 bag magic"));
    const auto magic_size = static_cast<std::size_t>(std::distance(data_.begin(), magic_end)) + 1;
    const std::string magic(reinterpret_cast<const char*>(data_.data()), magic_size);
    if (magic != "#ROSBAG V2.0\n") throw FormatError(context(path_, "unsupported ROS1 bag magic/version"));
    std::size_t pos = magic_size;
    const auto bag_header = read_header(view, pos);
    if (field_u8(bag_header.fields, "op") != 3) throw FormatError(context(path_, "expected BAGHEADER record"));
    const auto index_pos = field_u64(bag_header.fields, "index_pos");
    const auto connection_count = field_u32(bag_header.fields, "conn_count");
    const auto chunk_count = field_u32(bag_header.fields, "chunk_count");
    const auto encryptor = field_string(bag_header.fields, "encryptor", false);
    if (!encryptor.empty()) throw UnsupportedFeature(context(path_, "encrypted ROS1 bags are unsupported"));
    if (index_pos == 0) throw FormatError(context(path_, "ROS1 bag is not indexed"));
    if (index_pos > data_.size()) throw FormatError(context(path_, "ROS1 index_pos is outside file"));
    pos = static_cast<std::size_t>(index_pos);

    connections_.clear();
    for (std::uint32_t i = 0; i < connection_count; ++i) {
      const auto first = read_header(view, pos);
      if (field_u8(first.fields, "op") != 7) throw FormatError(context(path_, "expected CONNECTION record"));
      const auto connection_id = field_u32(first.fields, "conn");
      const auto raw_topic = field_string(first.fields, "topic");
      const auto second = read_record_as_header(view, pos);
      const auto type = field_string(second.fields, "type");
      Connection connection;
      connection.id = connection_id;
      connection.topic = normalize_topic(raw_topic);
      connection.type = normalize_type(type);
      connection.definition = {DefinitionFormat::Msg, field_string(second.fields, "message_definition", false)};
      connection.digest = field_string(second.fields, "md5sum", false);
      connection.serialization_format = "ros1";
      const auto caller = field_string(second.fields, "callerid", false);
      if (!caller.empty()) connection.callerid = caller;
      const auto latch = field_string(second.fields, "latching", false);
      if (!latch.empty()) connection.latching = std::stoi(latch);
      connections_.push_back(std::move(connection));
    }

    chunk_infos_.clear();
    for (std::uint32_t i = 0; i < chunk_count; ++i) {
      const auto record = read_record(view, pos, 6);
      if (field_u32(record.header.fields, "ver") != 1) throw FormatError(context(path_, "unsupported CHUNK_INFO version"));
      ChunkInfo info;
      info.position = field_u64(record.header.fields, "chunk_pos");
      const auto count = field_u32(record.header.fields, "count");
      info.start = count ? ros_time(record.header.fields.at("start_time")) : std::numeric_limits<std::uint64_t>::max();
      info.end = count ? ros_time(record.header.fields.at("end_time")) + 1 : 0;
      if (record.body.size != static_cast<std::size_t>(count) * 8) throw FormatError(context(path_, "invalid CHUNK_INFO body size"));
      for (std::uint32_t j = 0; j < count; ++j) info.counts[u32(record.body.data + j * 8)] = u32(record.body.data + j * 8 + 4);
      chunk_infos_.push_back(std::move(info));
    }

    chunks_.clear();
    indexes_.clear();
    for (const auto& info : chunk_infos_) {
      if (info.position >= data_.size()) throw FormatError(context(path_, "chunk position is outside file"));
      std::size_t chunk_pos = static_cast<std::size_t>(info.position);
      const auto chunk_record = read_record(view, chunk_pos, 5);
      Chunk chunk;
      chunk.compression = field_string(chunk_record.header.fields, "compression");
      chunk.data_position = static_cast<std::uint64_t>(chunk_record.body.data - data_.data());
      chunk.data_size = static_cast<std::uint32_t>(chunk_record.body.size);
      chunks_[info.position] = std::move(chunk);
      for (std::size_t i = 0; i < info.counts.size(); ++i) {
        const auto index_record = read_record(view, chunk_pos, 4);
        if (field_u32(index_record.header.fields, "ver") != 1) throw FormatError(context(path_, "unsupported IDXDATA version"));
        const auto count = field_u32(index_record.header.fields, "count");
        const auto connection = field_u32(index_record.header.fields, "conn");
        if (index_record.body.size != static_cast<std::size_t>(count) * 12) throw FormatError(context(path_, "invalid IDXDATA body size"));
        auto& index = indexes_[connection];
        for (std::uint32_t j = 0; j < count; ++j) {
          const auto* entry = index_record.body.data + j * 12;
          index.push_back({static_cast<std::uint64_t>(u32(entry)) * 1000000000ULL + u32(entry + 4), info.position, u32(entry + 8), order_++});
        }
      }
    }
    for (auto& item : indexes_) {
      std::stable_sort(item.second.begin(), item.second.end(), [](const IndexEntry& a, const IndexEntry& b) {
        return a.timestamp < b.timestamp;
      });
    }
    for (auto& connection : connections_) connection.message_count = indexes_[connection.id].size();
    metadata_ = ReaderMetadata{};
    metadata_.storage = StorageKind::Rosbag1;
    metadata_.files = {path_};
    metadata_.start_time = std::numeric_limits<std::uint64_t>::max();
    for (const auto& info : chunk_infos_) {
      metadata_.start_time = std::min(metadata_.start_time, info.start);
      metadata_.end_time = std::max(metadata_.end_time, info.end);
    }
    metadata_.message_count = 0;
    for (const auto& connection : connections_) metadata_.message_count += connection.message_count;
    if (metadata_.message_count == 0) metadata_.start_time = 0;
    metadata_.duration = metadata_.end_time >= metadata_.start_time ? metadata_.end_time - metadata_.start_time : 0;
    open_ = true;
  }

  void close() noexcept override {
    open_ = false;
    data_.clear();
    decompressed_.clear();
  }
  bool is_open() const noexcept override { return open_; }
  StorageKind kind() const override { return StorageKind::Rosbag1; }
  const ReaderMetadata& metadata() const override { ensure_open(); return metadata_; }
  const std::vector<Connection>& connections() const override { ensure_open(); return connections_; }

  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const override {
    ensure_open();
    std::vector<const IndexEntry*> entries;
    for (const auto& connection : connections_) {
      if (!has_topic(connection, filter)) continue;
      for (const auto& entry : indexes_.at(connection.id)) {
        if (in_time(entry.timestamp, filter)) entries.push_back(&entry);
      }
    }
    std::stable_sort(entries.begin(), entries.end(), [](const auto* a, const auto* b) { return a->timestamp < b->timestamp; });
    for (const auto* entry : entries) {
      auto& chunk = decompressed_[entry->chunk_position];
      if (chunk.empty()) {
        const auto& header = chunks_.at(entry->chunk_position);
        chunk = decompress_chunk(header.compression, ByteView{data_.data() + header.data_position, header.data_size});
      }
      ByteView chunk_view{chunk.data(), chunk.size()};
      std::size_t position = entry->offset;
      Record record = read_record(chunk_view, position);
      while (field_u8(record.header.fields, "op") == 7) record = read_record(chunk_view, position);
      if (field_u8(record.header.fields, "op") != 2) throw FormatError(context(path_, "index does not point to MSGDATA"));
      const auto connection_id = field_u32(record.header.fields, "conn");
      const auto time = ros_time(record.header.fields.at("time"));
      if (time != entry->timestamp) throw FormatError(context(path_, "IDXDATA timestamp mismatch"));
      const auto* connection = find_connection(connection_id);
      if (!connection) throw FormatError(context(path_, "MSGDATA references unknown connection"));
      auto bytes = std::make_shared<Bytes>(record.body.data, record.body.data + record.body.size);
      callback(Message{std::move(bytes), time, connection});
    }
  }

 private:
  static RecordHeader read_record_as_header(ByteView input, std::size_t& pos) {
    if (pos + 4 > input.size) throw FormatError("truncated connection data size");
    const auto body_size = u32(input.data + pos);
    pos += 4;
    if (body_size > input.size - pos) throw FormatError("connection data exceeds input");
    std::size_t body_pos = 0;
    const auto body = ByteView{input.data + pos, body_size};
    const auto result = read_fields(body, body_pos, body.size);
    if (body_pos != body_size) throw FormatError("connection data contains trailing bytes");
    pos += body_size;
    return result;
  }
  const Connection* find_connection(std::uint32_t id) const {
    const auto it = std::find_if(connections_.begin(), connections_.end(), [id](const auto& value) { return value.id == id; });
    return it == connections_.end() ? nullptr : &*it;
  }
  void ensure_open() const { if (!open_) throw RosbagsError("ROS1 reader is not open"); }

  std::string path_;
  mutable Bytes data_;
  mutable std::unordered_map<std::uint64_t, Bytes> decompressed_;
  std::vector<Connection> connections_;
  std::vector<ChunkInfo> chunk_infos_;
  std::unordered_map<std::uint64_t, Chunk> chunks_;
  std::unordered_map<std::uint32_t, std::vector<IndexEntry>> indexes_;
  ReaderMetadata metadata_;
  bool open_ = false;
  std::uint64_t order_ = 0;
};

}  // namespace

std::unique_ptr<Backend> make_rosbag1_backend(const std::string& path) {
  return std::make_unique<Rosbag1Backend>(path);
}

}  // namespace rosbags::internal
