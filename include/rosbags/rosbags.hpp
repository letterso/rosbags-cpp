#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace rosbags {

using Byte = std::uint8_t;
using Bytes = std::vector<Byte>;

struct ByteView {
  const Byte* data = nullptr;
  std::size_t size = 0;
  const Byte& operator[](std::size_t index) const { return data[index]; }
  bool empty() const { return size == 0; }
};

enum class StorageKind { Rosbag1, Sqlite3, Mcap };
enum class DefinitionFormat { None, Msg, Idl };
enum class UnknownTypePolicy { WarnAndSkip, WarnAndRaw, Error };

struct MessageDefinition {
  DefinitionFormat format = DefinitionFormat::None;
  std::string data;
};

struct QosProfile {
  std::string raw;
};

struct Connection {
  std::uint32_t id = 0;
  std::string topic;
  std::string type;
  MessageDefinition definition;
  std::string digest;
  std::uint64_t message_count = 0;
  std::string serialization_format;
  std::vector<QosProfile> qos_profiles;
  std::optional<std::string> callerid;
  std::optional<int> latching;
};

struct ReaderMetadata {
  StorageKind storage = StorageKind::Rosbag1;
  std::uint64_t start_time = 0;
  std::uint64_t end_time = 0;
  std::uint64_t duration = 0;
  std::uint64_t message_count = 0;
  std::string compression_format;
  std::string compression_mode;
  std::vector<std::string> files;
};

struct Message {
  std::shared_ptr<const Bytes> bytes;
  std::uint64_t timestamp = 0;
  const Connection* connection = nullptr;
};

struct ReadFilter {
  std::vector<std::string> topics;
  std::vector<std::uint32_t> connection_ids;
  std::optional<std::uint64_t> start;
  std::optional<std::uint64_t> stop;
};

class RosbagsError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};
class FormatError : public RosbagsError { using RosbagsError::RosbagsError; };
class UnsupportedFeature : public RosbagsError { using RosbagsError::RosbagsError; };
class DecodeError : public RosbagsError { using RosbagsError::RosbagsError; };

using WarningCallback = std::function<void(std::string_view)>;
using MessageCallback = std::function<void(const Message&)>;

class TypeRegistry;
struct DecodedMessage;
using DecodedCallback = std::function<void(const Message&, const DecodedMessage&)>;

class Reader {
 public:
  explicit Reader(std::string path);
  virtual ~Reader();
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;
  Reader(Reader&&) noexcept;
  Reader& operator=(Reader&&) noexcept;

  void open();
  void close() noexcept;
  bool is_open() const noexcept;
  StorageKind storage_kind() const;
  const std::string& path() const noexcept;
  const ReaderMetadata& metadata() const;
  const std::vector<Connection>& connections() const;
  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const;
  void read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                    const DecodedCallback& callback, UnknownTypePolicy policy = UnknownTypePolicy::WarnAndSkip,
                    const WarningCallback& warning = {}) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class AnyReader {
 public:
  explicit AnyReader(std::vector<std::string> paths);
  void open();
  void close() noexcept;
  bool is_open() const noexcept;
  const ReaderMetadata& metadata() const;
  const std::vector<Connection>& connections() const;
  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const;
  void read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                    const DecodedCallback& callback, UnknownTypePolicy policy = UnknownTypePolicy::WarnAndSkip,
                    const WarningCallback& warning = {}) const;

 private:
  std::vector<std::string> paths_;
  std::vector<std::unique_ptr<Reader>> readers_;
  std::vector<Connection> connections_;
  ReaderMetadata metadata_;
  bool open_ = false;
};

struct TypeSupportBase {
  std::string type_name;
  std::string profile;
  std::type_index cpp_type = typeid(void);
  virtual ~TypeSupportBase() = default;
  virtual std::shared_ptr<void> deserialize_ros1(ByteView data) const = 0;
  virtual std::shared_ptr<void> deserialize_cdr(ByteView data) const = 0;
  virtual std::string to_text(const void* object) const = 0;
};

template <typename T>
struct TypeSupport final : TypeSupportBase {
  using Ros1Decoder = std::function<T(ByteView)>;
  using CdrDecoder = std::function<T(ByteView)>;
  Ros1Decoder ros1_decoder;
  CdrDecoder cdr_decoder;
  std::function<std::string(const T&)> formatter;

  TypeSupport(std::string name, std::string profile, Ros1Decoder ros1, CdrDecoder cdr,
              std::function<std::string(const T&)> format = {})
      : ros1_decoder(std::move(ros1)), cdr_decoder(std::move(cdr)), formatter(std::move(format)) {
    type_name = std::move(name);
    this->profile = std::move(profile);
    cpp_type = typeid(T);
  }
  std::shared_ptr<void> deserialize_ros1(ByteView data) const override {
    return std::make_shared<T>(ros1_decoder(data));
  }
  std::shared_ptr<void> deserialize_cdr(ByteView data) const override {
    return std::make_shared<T>(cdr_decoder(data));
  }
  std::string to_text(const void* object) const override {
    if (formatter) return formatter(*static_cast<const T*>(object));
    return type_name;
  }
};

class TypeRegistry {
 public:
  template <typename T>
  void register_type(std::shared_ptr<const TypeSupport<T>> support) {
    supports_[key(support->profile, support->type_name)] = std::move(support);
  }
  void register_type(std::shared_ptr<const TypeSupportBase> support);
  const TypeSupportBase* find(std::string_view profile, std::string_view type) const;
  // Returns canonical type names; a type registered for multiple profiles may
  // appear more than once.
  std::vector<std::string> types() const;

 private:
  static std::string key(std::string_view profile, std::string_view type);
  std::unordered_map<std::string, std::shared_ptr<const TypeSupportBase>> supports_;
};

struct DecodedMessage {
  std::shared_ptr<void> object;
  const TypeSupportBase* support = nullptr;
  const Message* source = nullptr;
  // WarnAndRaw delivers a value without a registered C++ type support.
  bool raw_only() const noexcept { return support == nullptr; }
  template <typename T>
  const T& as() const {
    if (!support || support->cpp_type != typeid(T)) throw DecodeError("decoded type mismatch");
    return *static_cast<const T*>(object.get());
  }
};

std::optional<DecodedMessage> decode(const Message& message, const TypeRegistry& registry,
                                     std::string_view profile, UnknownTypePolicy policy,
                                     const WarningCallback& warning = {});

std::string storage_kind_name(StorageKind kind);
std::string definition_format_name(DefinitionFormat format);
std::string normalize_topic(std::string_view topic);
std::string normalize_type(std::string_view type);

}  // namespace rosbags
