#include "internal.hpp"

#include <lz4frame.h>

#if ROSBAGS_HAS_BZ2
#include <bzlib.h>
#endif

namespace rosbags::internal {
namespace {

using Fields = std::unordered_map<std::string, Bytes>;

struct RecordHeader { Fields fields; };
struct Record { RecordHeader header; ByteView body; };

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

Record read_record(ByteView input, std::size_t& pos, std::optional<std::uint8_t> expected = {}) {
  if (pos + 4 > input.size) throw FormatError("truncated rosbag header length");
  const auto header_size = u32(input.data + pos);
  pos += 4;
  if (header_size > input.size - pos) throw FormatError("rosbag header exceeds input");
  const auto header = read_fields(input, pos, pos + header_size);
  if (expected && field_u8(header.fields, "op") != *expected) throw FormatError("unexpected rosbag record opcode");
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

Bytes read_bytes(std::istream& input, std::size_t size, std::string_view what) {
  Bytes result(size);
  if (size && !input.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(size)))
    throw FormatError("truncated ROS1 " + std::string(what));
  return result;
}
std::uint32_t read_u32(std::istream& input, std::string_view what) {
  const auto bytes = read_bytes(input, 4, what);
  return u32(bytes.data());
}
RecordHeader read_header(std::istream& input) {
  const auto size = read_u32(input, "header length");
  auto bytes = read_bytes(input, size, "header");
  std::size_t pos = 0;
  return read_fields(ByteView{bytes.data(), bytes.size()}, pos, bytes.size());
}
RecordHeader read_record_body_as_fields(std::istream& input) {
  const auto size = read_u32(input, "connection body length");
  auto bytes = read_bytes(input, size, "connection body");
  std::size_t pos = 0;
  return read_fields(ByteView{bytes.data(), bytes.size()}, pos, bytes.size());
}
std::uint64_t stream_position(std::istream& input, std::string_view what) {
  const auto value = input.tellg();
  if (value < 0) throw RosbagsError("cannot determine ROS1 " + std::string(what));
  return static_cast<std::uint64_t>(value);
}
void seek(std::istream& input, std::uint64_t position, std::uint64_t file_size, std::string_view what) {
  if (position > file_size) throw FormatError("ROS1 " + std::string(what) + " is outside file");
  input.clear();
  input.seekg(static_cast<std::streamoff>(position));
  if (!input) throw RosbagsError("cannot seek ROS1 " + std::string(what));
}
void skip(std::istream& input, std::uint64_t count, std::uint64_t file_size, std::string_view what) {
  const auto position = stream_position(input, what);
  if (count > file_size - position) throw FormatError("ROS1 " + std::string(what) + " exceeds file");
  seek(input, position + count, file_size, what);
}
void append_checked(Bytes& output, const Byte* data, std::size_t size, std::size_t limit) {
  if (size > limit - output.size()) throw ResourceLimitError("ROS1 decompressed chunk exceeds max_rosbag1_chunk_bytes");
  output.insert(output.end(), data, data + size);
}

Bytes decompress_lz4(std::ifstream& input, std::uint64_t position, std::uint64_t compressed_size,
                     std::size_t uncompressed_size, std::size_t limit, std::uint64_t file_size) {
  LZ4F_dctx* context = nullptr;
  if (LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION)))
    throw FormatError("cannot create LZ4 decompressor");
  const std::unique_ptr<LZ4F_dctx, decltype(&LZ4F_freeDecompressionContext)> guard(context, &LZ4F_freeDecompressionContext);
  seek(input, position, file_size, "chunk payload");
  std::array<Byte, 64 * 1024> source{};
  std::array<Byte, 64 * 1024> destination{};
  std::size_t available = 0;
  std::size_t source_pos = 0;
  std::uint64_t remaining = compressed_size;
  Bytes output;
  output.reserve(std::min(uncompressed_size, limit));
  for (;;) {
    if (source_pos == available) {
      if (remaining == 0) throw FormatError("truncated LZ4 compressed chunk");
      const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, source.size()));
      const auto bytes = read_bytes(input, count, "LZ4 chunk payload");
      std::copy(bytes.begin(), bytes.end(), source.begin());
      remaining -= count;
      available = count;
      source_pos = 0;
    }
    std::size_t source_size = available - source_pos;
    std::size_t destination_size = destination.size();
    const auto result = LZ4F_decompress(context, destination.data(), &destination_size,
                                        source.data() + source_pos, &source_size, nullptr);
    if (LZ4F_isError(result)) throw FormatError("LZ4 decompression failed");
    if (source_size == 0 && destination_size == 0) throw FormatError("LZ4 decompressor made no progress");
    source_pos += source_size;
    append_checked(output, destination.data(), destination_size, limit);
    if (result == 0) {
      if (source_pos != available || remaining != 0) throw FormatError("trailing bytes after LZ4 chunk");
      break;
    }
  }
  if (output.size() != uncompressed_size) throw FormatError("ROS1 LZ4 chunk size mismatch");
  return output;
}

#if ROSBAGS_HAS_BZ2
Bytes decompress_bz2(std::ifstream& input, std::uint64_t position, std::uint64_t compressed_size,
                     std::size_t uncompressed_size, std::size_t limit, std::uint64_t file_size) {
  bz_stream stream{};
  if (BZ2_bzDecompressInit(&stream, 0, 0) != BZ_OK) throw FormatError("cannot create BZip2 decompressor");
  struct Guard { bz_stream* value; ~Guard() { BZ2_bzDecompressEnd(value); } } guard{&stream};
  seek(input, position, file_size, "chunk payload");
  std::array<Byte, 64 * 1024> source{};
  std::array<Byte, 64 * 1024> destination{};
  std::uint64_t remaining = compressed_size;
  Bytes output;
  output.reserve(std::min(uncompressed_size, limit));
  for (;;) {
    if (stream.avail_in == 0 && remaining) {
      const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, source.size()));
      const auto bytes = read_bytes(input, count, "BZip2 chunk payload");
      std::copy(bytes.begin(), bytes.end(), source.begin());
      remaining -= count;
      stream.next_in = reinterpret_cast<char*>(source.data());
      stream.avail_in = static_cast<unsigned int>(count);
    }
    stream.next_out = reinterpret_cast<char*>(destination.data());
    stream.avail_out = static_cast<unsigned int>(destination.size());
    const auto before_in = stream.avail_in;
    const auto result = BZ2_bzDecompress(&stream);
    const auto produced = destination.size() - stream.avail_out;
    append_checked(output, destination.data(), produced, limit);
    if (result == BZ_STREAM_END) {
      if (stream.avail_in || remaining) throw FormatError("trailing bytes after BZip2 chunk");
      break;
    }
    if (result != BZ_OK) throw FormatError("BZip2 decompression failed");
    if (before_in == stream.avail_in && produced == 0) {
      if (remaining == 0) throw FormatError("truncated BZip2 compressed chunk");
      throw FormatError("BZip2 decompressor made no progress");
    }
  }
  if (output.size() != uncompressed_size) throw FormatError("ROS1 BZip2 chunk size mismatch");
  return output;
}
#endif

struct ChunkInfo {
  std::uint64_t position = 0;
  std::uint64_t start = 0;
  std::uint64_t end = 0;
  std::uint32_t index_record_count = 0;
};
struct Chunk {
  std::uint64_t data_position = 0;
  std::uint64_t compressed_size = 0;
  std::uint32_t uncompressed_size = 0;
  std::string compression;
};
struct IndexEntry {
  std::uint64_t timestamp = 0;
  std::uint32_t chunk_index = 0;
  std::uint32_t offset = 0;
};

class Rosbag1Backend final : public Backend {
 public:
  Rosbag1Backend(std::string path, ReaderOptions options) : path_(std::move(path)), options_(options) {
    if (options_.max_rosbag1_chunk_bytes == 0) throw RosbagsError("max_rosbag1_chunk_bytes must be greater than zero");
  }

  void open() override {
    if (open_) throw RosbagsError("ROS1 reader is already open");
    close();
    try {
      std::error_code error;
      file_size_ = std::filesystem::file_size(path_, error);
      if (error) throw RosbagsError(context(path_, "cannot determine file size"));
      std::ifstream input(path_, std::ios::binary);
      if (!input) throw RosbagsError(context(path_, "cannot open file"));
      std::string magic;
      if (!std::getline(input, magic) || magic != "#ROSBAG V2.0") throw FormatError(context(path_, "unsupported ROS1 bag magic/version"));
      const auto bag_header = read_header(input);
      if (field_u8(bag_header.fields, "op") != 3) throw FormatError(context(path_, "expected BAGHEADER record"));
      const auto index_pos = field_u64(bag_header.fields, "index_pos");
      const auto connection_count = field_u32(bag_header.fields, "conn_count");
      const auto chunk_count = field_u32(bag_header.fields, "chunk_count");
      if (!field_string(bag_header.fields, "encryptor", false).empty()) throw UnsupportedFeature(context(path_, "encrypted ROS1 bags are unsupported"));
      if (index_pos == 0) throw FormatError(context(path_, "ROS1 bag is not indexed"));
      seek(input, index_pos, file_size_, "index position");

      for (std::uint32_t index = 0; index < connection_count; ++index) {
        const auto first = read_header(input);
        if (field_u8(first.fields, "op") != 7) throw FormatError(context(path_, "expected CONNECTION record"));
        const auto connection_id = field_u32(first.fields, "conn");
        const auto raw_topic = field_string(first.fields, "topic");
        const auto second = read_record_body_as_fields(input);
        Connection connection;
        connection.id = connection_id;
        connection.topic = normalize_topic(raw_topic);
        connection.type = normalize_type(field_string(second.fields, "type"));
        connection.definition = {DefinitionFormat::Msg, field_string(second.fields, "message_definition", false)};
        connection.digest = field_string(second.fields, "md5sum", false);
        connection.serialization_format = "ros1";
        if (const auto caller = field_string(second.fields, "callerid", false); !caller.empty()) connection.callerid = caller;
        if (const auto latch = field_string(second.fields, "latching", false); !latch.empty()) connection.latching = std::stoi(latch);
        connections_.push_back(std::move(connection));
      }

      chunk_infos_.reserve(chunk_count);
      for (std::uint32_t index = 0; index < chunk_count; ++index) {
        const auto header = read_header(input);
        if (field_u8(header.fields, "op") != 6 || field_u32(header.fields, "ver") != 1)
          throw FormatError(context(path_, "invalid CHUNK_INFO record"));
        ChunkInfo info;
        info.position = field_u64(header.fields, "chunk_pos");
        info.index_record_count = field_u32(header.fields, "count");
        info.start = info.index_record_count ? ros_time(header.fields.at("start_time")) : std::numeric_limits<std::uint64_t>::max();
        info.end = info.index_record_count ? ros_time(header.fields.at("end_time")) + 1 : 0;
        if (read_u32(input, "CHUNK_INFO body length") != static_cast<std::uint64_t>(info.index_record_count) * 8)
          throw FormatError(context(path_, "invalid CHUNK_INFO body size"));
        for (std::uint32_t count = 0; count < info.index_record_count; ++count) {
          (void)read_u32(input, "CHUNK_INFO connection id");
          (void)read_u32(input, "CHUNK_INFO message count");
        }
        chunk_infos_.push_back(info);
      }

      chunks_.reserve(chunk_infos_.size());
      for (std::size_t chunk_index = 0; chunk_index < chunk_infos_.size(); ++chunk_index) {
        const auto& info = chunk_infos_[chunk_index];
        seek(input, info.position, file_size_, "chunk position");
        const auto chunk_header = read_header(input);
        if (field_u8(chunk_header.fields, "op") != 5) throw FormatError(context(path_, "expected CHUNK record"));
        Chunk chunk;
        chunk.compression = field_string(chunk_header.fields, "compression");
        chunk.uncompressed_size = field_u32(chunk_header.fields, "size");
        chunk.compressed_size = read_u32(input, "chunk body length");
        chunk.data_position = stream_position(input, "chunk payload position");
        skip(input, chunk.compressed_size, file_size_, "chunk payload");
        chunks_.push_back(std::move(chunk));
        for (std::uint32_t record_index = 0; record_index < info.index_record_count; ++record_index) {
          const auto index_header = read_header(input);
          if (field_u8(index_header.fields, "op") != 4 || field_u32(index_header.fields, "ver") != 1)
            throw FormatError(context(path_, "invalid IDXDATA record"));
          const auto count = field_u32(index_header.fields, "count");
          const auto connection = field_u32(index_header.fields, "conn");
          if (read_u32(input, "IDXDATA body length") != static_cast<std::uint64_t>(count) * 12)
            throw FormatError(context(path_, "invalid IDXDATA body size"));
          auto& entries = indexes_[connection];
          entries.reserve(entries.size() + count);
          for (std::uint32_t entry_index = 0; entry_index < count; ++entry_index) {
            const auto sec = read_u32(input, "IDXDATA seconds");
            const auto nsec = read_u32(input, "IDXDATA nanoseconds");
            entries.push_back({static_cast<std::uint64_t>(sec) * 1000000000ULL + nsec,
                               static_cast<std::uint32_t>(chunk_index), read_u32(input, "IDXDATA offset")});
          }
        }
      }
      for (auto& item : indexes_) std::stable_sort(item.second.begin(), item.second.end(), [](const IndexEntry& left, const IndexEntry& right) { return left.timestamp < right.timestamp; });
      for (auto& connection : connections_) connection.message_count = indexes_[connection.id].size();
      metadata_.storage = StorageKind::Rosbag1;
      metadata_.files = {path_};
      metadata_.start_time = std::numeric_limits<std::uint64_t>::max();
      for (const auto& info : chunk_infos_) {
        metadata_.start_time = std::min(metadata_.start_time, info.start);
        metadata_.end_time = std::max(metadata_.end_time, info.end);
      }
      for (const auto& connection : connections_) metadata_.message_count += connection.message_count;
      if (metadata_.message_count == 0) metadata_.start_time = metadata_.end_time = 0;
      metadata_.duration = metadata_.end_time >= metadata_.start_time ? metadata_.end_time - metadata_.start_time : 0;
      open_ = true;
    } catch (...) {
      close();
      throw;
    }
  }

  void close() noexcept override {
    open_ = false;
    connections_.clear();
    chunk_infos_.clear();
    chunks_.clear();
    indexes_.clear();
    metadata_ = ReaderMetadata{};
    file_size_ = 0;
  }
  bool is_open() const noexcept override { return open_; }
  StorageKind kind() const override { ensure_open(); return StorageKind::Rosbag1; }
  const ReaderMetadata& metadata() const override { ensure_open(); return metadata_; }
  const std::vector<Connection>& connections() const override { ensure_open(); return connections_; }
  std::unique_ptr<BackendCursor> make_cursor(const ReadFilter& filter) const override;

 private:
  class Cursor;
  Bytes load_chunk(std::ifstream& input, const Chunk& chunk) const {
    const auto limit = options_.max_rosbag1_chunk_bytes;
    if (chunk.uncompressed_size > limit) throw ResourceLimitError(context(path_, "ROS1 chunk exceeds max_rosbag1_chunk_bytes"));
    if (chunk.compression.empty() || chunk.compression == "none") {
      if (chunk.compressed_size != chunk.uncompressed_size) throw FormatError(context(path_, "uncompressed chunk size mismatch"));
      seek(input, chunk.data_position, file_size_, "chunk payload");
      return read_bytes(input, static_cast<std::size_t>(chunk.compressed_size), "chunk payload");
    }
    if (chunk.compression == "lz4") return decompress_lz4(input, chunk.data_position, chunk.compressed_size, chunk.uncompressed_size, limit, file_size_);
    if (chunk.compression == "bz2") {
#if ROSBAGS_HAS_BZ2
      return decompress_bz2(input, chunk.data_position, chunk.compressed_size, chunk.uncompressed_size, limit, file_size_);
#else
      throw UnsupportedFeature("BZ2 support is disabled (libbz2 development files not found)");
#endif
    }
    throw UnsupportedFeature("unsupported ROS1 chunk compression: " + chunk.compression);
  }
  const Connection* find_connection(std::uint32_t id) const {
    const auto it = std::find_if(connections_.begin(), connections_.end(), [id](const auto& value) { return value.id == id; });
    return it == connections_.end() ? nullptr : &*it;
  }
  void ensure_open() const { if (!open_) throw RosbagsError("ROS1 reader is not open"); }

  std::string path_;
  ReaderOptions options_;
  std::vector<Connection> connections_;
  std::vector<ChunkInfo> chunk_infos_;
  std::vector<Chunk> chunks_;
  std::unordered_map<std::uint32_t, std::vector<IndexEntry>> indexes_;
  ReaderMetadata metadata_;
  std::uint64_t file_size_ = 0;
  bool open_ = false;
};

class Rosbag1Backend::Cursor final : public BackendCursor {
 public:
  Cursor(const Rosbag1Backend& owner, const ReadFilter& filter) : owner_(owner), stop_(filter.stop) {
    owner_.ensure_open();
    input_.open(owner_.path_, std::ios::binary);
    if (!input_) throw RosbagsError(context(owner_.path_, "cannot open file"));
    std::size_t rank = 0;
    for (const auto& connection : owner_.connections_) {
      if (!has_topic(connection, filter)) continue;
      const auto found = owner_.indexes_.find(connection.id);
      if (found == owner_.indexes_.end()) { ++rank; continue; }
      const auto& entries = found->second;
      const auto first = filter.start ? std::lower_bound(entries.begin(), entries.end(), *filter.start, [](const IndexEntry& entry, std::uint64_t time) { return entry.timestamp < time; }) : entries.begin();
      if (first != entries.end() && (!stop_ || first->timestamp < *stop_)) {
        heap_.push({first->timestamp, rank, static_cast<std::size_t>(std::distance(entries.begin(), first)), &entries});
      }
      ++rank;
    }
  }

  bool next(Message& output) override {
    owner_.ensure_open();
    if (heap_.empty()) return false;
    const auto item = heap_.top();
    heap_.pop();
    const auto& entry = (*item.entries)[item.position];
    if (!cached_chunk_index_ || *cached_chunk_index_ != entry.chunk_index) {
      cached_chunk_ = owner_.load_chunk(input_, owner_.chunks_.at(entry.chunk_index));
      cached_chunk_index_ = entry.chunk_index;
    }
    ByteView chunk{cached_chunk_.data(), cached_chunk_.size()};
    std::size_t position = entry.offset;
    Record record = read_record(chunk, position);
    while (field_u8(record.header.fields, "op") == 7) record = read_record(chunk, position);
    if (field_u8(record.header.fields, "op") != 2) throw FormatError(context(owner_.path_, "index does not point to MSGDATA"));
    const auto time = ros_time(record.header.fields.at("time"));
    if (time != entry.timestamp) throw FormatError(context(owner_.path_, "IDXDATA timestamp mismatch"));
    const auto* connection = owner_.find_connection(field_u32(record.header.fields, "conn"));
    if (!connection) throw FormatError(context(owner_.path_, "MSGDATA references unknown connection"));
    output = Message{std::make_shared<Bytes>(record.body.data, record.body.data + record.body.size), time, connection};
    const auto next_position = item.position + 1;
    if (next_position < item.entries->size()) {
      const auto& next = (*item.entries)[next_position];
      if (!stop_ || next.timestamp < *stop_) heap_.push({next.timestamp, item.rank, next_position, item.entries});
    }
    return true;
  }

 private:
  struct HeapItem {
    std::uint64_t timestamp = 0;
    std::size_t rank = 0;
    std::size_t position = 0;
    const std::vector<IndexEntry>* entries = nullptr;
  };
  struct HeapCompare {
    bool operator()(const HeapItem& left, const HeapItem& right) const {
      if (left.timestamp != right.timestamp) return left.timestamp > right.timestamp;
      if (left.rank != right.rank) return left.rank > right.rank;
      return left.position > right.position;
    }
  };
  const Rosbag1Backend& owner_;
  std::ifstream input_;
  std::priority_queue<HeapItem, std::vector<HeapItem>, HeapCompare> heap_;
  Bytes cached_chunk_;
  std::optional<std::uint32_t> cached_chunk_index_;
  std::optional<std::uint64_t> stop_;
};

std::unique_ptr<BackendCursor> Rosbag1Backend::make_cursor(const ReadFilter& filter) const {
  return std::make_unique<Cursor>(*this, filter);
}

}  // namespace

std::unique_ptr<Backend> make_rosbag1_backend(const std::string& path, ReaderOptions options) {
  return std::make_unique<Rosbag1Backend>(path, options);
}

}  // namespace rosbags::internal
