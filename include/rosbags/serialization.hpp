#pragma once

#include "rosbags/rosbags.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace rosbags::serialization {

inline std::uint16_t load16(const Byte* data, bool little) {
  return little ? static_cast<std::uint16_t>(data[0] | (data[1] << 8U))
                : static_cast<std::uint16_t>((data[0] << 8U) | data[1]);
}
inline std::uint32_t load32(const Byte* data, bool little) {
  if (little) return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8U) |
      (static_cast<std::uint32_t>(data[2]) << 16U) | (static_cast<std::uint32_t>(data[3]) << 24U);
  return static_cast<std::uint32_t>(data[3]) | (static_cast<std::uint32_t>(data[2]) << 8U) |
      (static_cast<std::uint32_t>(data[1]) << 16U) | (static_cast<std::uint32_t>(data[0]) << 24U);
}
inline std::uint64_t load64(const Byte* data, bool little) {
  std::uint64_t value = 0;
  if (little) for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(data[i]) << (8U * i);
  else for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(data[7 - i]) << (8U * i);
  return value;
}
inline float float32(std::uint32_t value) { float result{}; std::memcpy(&result, &value, sizeof(result)); return result; }
inline double float64(std::uint64_t value) { double result{}; std::memcpy(&result, &value, sizeof(result)); return result; }

class ReaderBase {
 public:
  explicit ReaderBase(ByteView bytes) : bytes_(bytes) {}
  std::size_t position() const noexcept { return position_; }
  std::size_t remaining() const noexcept { return bytes_.size - position_; }
  void require(std::size_t size) {
    if (size > bytes_.size - position_) throw DecodeError("serialized message is truncated");
  }
  ByteView read_bytes(std::size_t size) {
    require(size);
    const auto result = ByteView{bytes_.data + position_, size};
    position_ += size;
    return result;
  }
  void skip(std::size_t size) { (void)read_bytes(size); }
  void finish() const {
    if (position_ != bytes_.size) throw DecodeError("serialized message has trailing bytes");
  }
 protected:
  ByteView bytes_;
  std::size_t position_ = 0;
};

class Ros1Reader final : public ReaderBase {
 public:
  explicit Ros1Reader(ByteView bytes) : ReaderBase(bytes) {}
  std::uint8_t u8() { return read_bytes(1)[0]; }
  std::int8_t i8() { return static_cast<std::int8_t>(u8()); }
  std::uint16_t u16() { return load16(read_bytes(2).data, true); }
  std::int16_t i16() { return static_cast<std::int16_t>(u16()); }
  std::uint32_t u32() { return load32(read_bytes(4).data, true); }
  std::int32_t i32() { return static_cast<std::int32_t>(u32()); }
  std::uint64_t u64() { return load64(read_bytes(8).data, true); }
  std::int64_t i64() { return static_cast<std::int64_t>(u64()); }
  float f32() { return float32(u32()); }
  double f64() { return float64(u64()); }
  bool boolean() { return u8() != 0; }
  std::string string() {
    const auto size = u32();
    if (size > std::numeric_limits<std::size_t>::max()) throw DecodeError("ROS1 string is too large");
    const auto bytes = read_bytes(static_cast<std::size_t>(size));
    return std::string(reinterpret_cast<const char*>(bytes.data), bytes.size);
  }
};

class CdrReader final : public ReaderBase {
 public:
  explicit CdrReader(ByteView bytes) : ReaderBase(bytes) {
    if (bytes.size < 4 || bytes.data[0] != 0) throw DecodeError("invalid CDR encapsulation");
    if (bytes.data[1] != 0 && bytes.data[1] != 1) throw DecodeError("invalid CDR byte order");
    little_ = bytes.data[1] == 1;
    position_ = 4;
  }
  void align(std::size_t alignment) {
    if (!alignment) return;
    const auto remainder = (position_ - data_origin_) % alignment;
    if (remainder) skip(alignment - remainder);
  }
  std::uint8_t u8() { return read_bytes(1)[0]; }
  std::int8_t i8() { return static_cast<std::int8_t>(u8()); }
  std::uint16_t u16() { align(2); return load16(read_bytes(2).data, little_); }
  std::int16_t i16() { return static_cast<std::int16_t>(u16()); }
  std::uint32_t u32() { align(4); return load32(read_bytes(4).data, little_); }
  std::int32_t i32() { return static_cast<std::int32_t>(u32()); }
  std::uint64_t u64() { align(8); return load64(read_bytes(8).data, little_); }
  std::int64_t i64() { return static_cast<std::int64_t>(u64()); }
  float f32() { return float32(u32()); }
  double f64() { return float64(u64()); }
  bool boolean() { return u8() != 0; }
  std::string string() {
    const auto size = u32();
    const auto bytes = read_bytes(size);
    if (size && bytes.data[size - 1] == 0) return std::string(reinterpret_cast<const char*>(bytes.data), size - 1);
    return std::string(reinterpret_cast<const char*>(bytes.data), size);
  }
  bool little_endian() const noexcept { return little_; }
 private:
  bool little_ = true;
  static constexpr std::size_t data_origin_ = 4;
};

template <typename T>
inline T ros1_scalar(Ros1Reader& reader);
template <> inline bool ros1_scalar<bool>(Ros1Reader& reader) { return reader.boolean(); }
template <> inline std::uint8_t ros1_scalar<std::uint8_t>(Ros1Reader& reader) { return reader.u8(); }
template <> inline std::int8_t ros1_scalar<std::int8_t>(Ros1Reader& reader) { return reader.i8(); }
template <> inline std::uint16_t ros1_scalar<std::uint16_t>(Ros1Reader& reader) { return reader.u16(); }
template <> inline std::int16_t ros1_scalar<std::int16_t>(Ros1Reader& reader) { return reader.i16(); }
template <> inline std::uint32_t ros1_scalar<std::uint32_t>(Ros1Reader& reader) { return reader.u32(); }
template <> inline std::int32_t ros1_scalar<std::int32_t>(Ros1Reader& reader) { return reader.i32(); }
template <> inline std::uint64_t ros1_scalar<std::uint64_t>(Ros1Reader& reader) { return reader.u64(); }
template <> inline std::int64_t ros1_scalar<std::int64_t>(Ros1Reader& reader) { return reader.i64(); }
template <> inline float ros1_scalar<float>(Ros1Reader& reader) { return reader.f32(); }
template <> inline double ros1_scalar<double>(Ros1Reader& reader) { return reader.f64(); }

template <typename T>
inline T cdr_scalar(CdrReader& reader);
template <> inline bool cdr_scalar<bool>(CdrReader& reader) { return reader.boolean(); }
template <> inline std::uint8_t cdr_scalar<std::uint8_t>(CdrReader& reader) { return reader.u8(); }
template <> inline std::int8_t cdr_scalar<std::int8_t>(CdrReader& reader) { return reader.i8(); }
template <> inline std::uint16_t cdr_scalar<std::uint16_t>(CdrReader& reader) { return reader.u16(); }
template <> inline std::int16_t cdr_scalar<std::int16_t>(CdrReader& reader) { return reader.i16(); }
template <> inline std::uint32_t cdr_scalar<std::uint32_t>(CdrReader& reader) { return reader.u32(); }
template <> inline std::int32_t cdr_scalar<std::int32_t>(CdrReader& reader) { return reader.i32(); }
template <> inline std::uint64_t cdr_scalar<std::uint64_t>(CdrReader& reader) { return reader.u64(); }
template <> inline std::int64_t cdr_scalar<std::int64_t>(CdrReader& reader) { return reader.i64(); }
template <> inline float cdr_scalar<float>(CdrReader& reader) { return reader.f32(); }
template <> inline double cdr_scalar<double>(CdrReader& reader) { return reader.f64(); }

}  // namespace rosbags::serialization
