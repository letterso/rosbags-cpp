#pragma once

#include "rosbags/rosbags.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <queue>
#include <sstream>
#include <type_traits>
#include <yaml-cpp/yaml.h>

namespace rosbags::internal {

inline std::string context(const std::string& path, std::string_view message) {
  return path + ": " + std::string(message);
}

inline std::uint16_t u16(const Byte* p) {
  return static_cast<std::uint16_t>(p[0]) |
         static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8U);
}
inline std::uint32_t u32(const Byte* p) {
  return static_cast<std::uint32_t>(p[0]) |
         (static_cast<std::uint32_t>(p[1]) << 8U) |
         (static_cast<std::uint32_t>(p[2]) << 16U) |
         (static_cast<std::uint32_t>(p[3]) << 24U);
}
inline std::uint64_t u64(const Byte* p) {
  std::uint64_t value = 0;
  for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(p[i]) << (8U * i);
  return value;
}
inline std::uint16_t u16be(const Byte* p) {
  return static_cast<std::uint16_t>(p[1]) |
         static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[0]) << 8U);
}
inline std::uint32_t u32be(const Byte* p) {
  return static_cast<std::uint32_t>(p[3]) |
         (static_cast<std::uint32_t>(p[2]) << 8U) |
         (static_cast<std::uint32_t>(p[1]) << 16U) |
         (static_cast<std::uint32_t>(p[0]) << 24U);
}
inline std::uint64_t u64be(const Byte* p) {
  std::uint64_t value = 0;
  for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(p[7 - i]) << (8U * i);
  return value;
}

class Cursor {
 public:
  explicit Cursor(ByteView view) : view_(view) {}
  std::size_t position() const noexcept { return pos_; }
  std::size_t remaining() const noexcept { return view_.size - pos_; }
  ByteView view() const noexcept { return view_; }
  void require(std::size_t count) const {
    if (count > remaining()) throw FormatError("truncated record or message payload");
  }
  void skip(std::size_t count) {
    require(count);
    pos_ += count;
  }
  ByteView bytes(std::size_t count) {
    require(count);
    ByteView result{view_.data + pos_, count};
    pos_ += count;
    return result;
  }
  std::uint8_t u8() {
    require(1);
    return view_.data[pos_++];
  }
  std::uint16_t u16le() { return read(&internal::u16, 2); }
  std::uint32_t u32le() { return read(&internal::u32, 4); }
  std::uint64_t u64le() { return read(&internal::u64, 8); }
  std::int32_t i32le() { return static_cast<std::int32_t>(u32le()); }
  std::string string32() {
    auto length = u32le();
    auto raw = bytes(length);
    if (length && raw.data[length - 1] == 0) --length;  // CDR strings include a NUL.
    return std::string(reinterpret_cast<const char*>(raw.data), length);
  }

 private:
  template <typename T>
  T read(T (*function)(const Byte*), std::size_t size) {
    require(size);
    auto value = function(view_.data + pos_);
    pos_ += size;
    return value;
  }
  ByteView view_;
  std::size_t pos_ = 0;
};

inline std::vector<Byte> read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) throw RosbagsError(context(path.string(), "cannot open file"));
  const auto end = input.tellg();
  if (end < 0) throw RosbagsError(context(path.string(), "cannot determine file size"));
  std::vector<Byte> data(static_cast<std::size_t>(end));
  input.seekg(0);
  if (!data.empty()) input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
  if (!input && !data.empty()) throw RosbagsError(context(path.string(), "cannot read file"));
  return data;
}

inline bool has_topic(const Connection& connection, const ReadFilter& filter) {
  if (!filter.topics.empty()) {
    const auto match = std::find_if(filter.topics.begin(), filter.topics.end(), [&](const auto& topic) {
      return topics_match(topic, connection.topic, filter.topic_match, filter.topic_namespace);
    });
    if (match == filter.topics.end()) return false;
  }
  if (!filter.connection_ids.empty() &&
      std::find(filter.connection_ids.begin(), filter.connection_ids.end(), connection.id) ==
          filter.connection_ids.end())
    return false;
  return true;
}

inline bool in_time(std::uint64_t time, const ReadFilter& filter) {
  return (!filter.start || time >= *filter.start) && (!filter.stop || time < *filter.stop);
}

class BackendCursor {
 public:
  virtual ~BackendCursor() = default;
  virtual bool next(Message& output) = 0;
};

struct Backend {
  virtual ~Backend() = default;
  virtual void open() = 0;
  virtual void close() noexcept = 0;
  virtual bool is_open() const noexcept = 0;
  virtual StorageKind kind() const = 0;
  virtual const ReaderMetadata& metadata() const = 0;
  virtual const std::vector<Connection>& connections() const = 0;
  virtual std::unique_ptr<BackendCursor> make_cursor(const ReadFilter&) const = 0;
  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const {
    auto cursor = make_cursor(filter);
    Message message;
    while (cursor->next(message)) callback(message);
  }
};

std::unique_ptr<Backend> make_rosbag1_backend(const std::string& path, ReaderOptions options);
std::unique_ptr<Backend> make_sqlite_backend(const std::string& path);
std::unique_ptr<Backend> make_directory_backend(const std::string& path);
std::unique_ptr<Backend> make_mcap_backend(const std::string& path);

}  // namespace rosbags::internal
