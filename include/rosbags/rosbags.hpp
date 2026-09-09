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
enum class TopicMatchPolicy { Strict, IgnoreLeadingSlash, ResolveNamespace };
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
  // Original storage spelling for diagnostics; topic retains normalized spelling.
  std::string original_topic = {};
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
  std::string source_path = {};
};

struct ReadFilter {
  std::vector<std::string> topics;
  std::vector<std::uint32_t> connection_ids;
  std::optional<std::uint64_t> start;
  std::optional<std::uint64_t> stop;
  TopicMatchPolicy topic_match = TopicMatchPolicy::Strict;
  std::string topic_namespace;
};

struct ErrorContext {
  std::optional<std::string> source_path;
  std::optional<std::uint32_t> connection_id;
  std::optional<std::string> topic;
  std::optional<std::string> type;
  std::optional<std::string> serialization_format;
  std::optional<std::uint64_t> timestamp;
  std::optional<std::size_t> byte_offset;
};

class RosbagsError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
  const ErrorContext& context() const noexcept { return context_; }
  // Fill missing context without replacing more precise information or slicing
  // derived exceptions. The original reason remains available via reason().
  void add_context(const ErrorContext& context);
  const char* reason() const noexcept { return std::runtime_error::what(); }
  const char* what() const noexcept override;
 private:
  ErrorContext context_;
  std::string diagnostic_;
};
class FormatError : public RosbagsError { using RosbagsError::RosbagsError; };
class UnsupportedFeature : public RosbagsError { using RosbagsError::RosbagsError; };
class DecodeError : public RosbagsError { using RosbagsError::RosbagsError; };
class ResourceLimitError : public RosbagsError { using RosbagsError::RosbagsError; };

using WarningCallback = std::function<void(std::string_view)>;
using MessageCallback = std::function<void(const Message&)>;

struct ReaderOptions {
  // ROS1 compresses whole chunks.  The reader must materialize one such chunk
  // before it can seek to an indexed message offset.
  std::size_t max_rosbag1_chunk_bytes = 256U * 1024U * 1024U;
};

class Reader;
class AnyReader;
class AnyReaderCursorImpl;

class MessageCursor {
 public:
  struct Impl;

  ~MessageCursor();
  MessageCursor(MessageCursor&&) noexcept;
  MessageCursor& operator=(MessageCursor&&) noexcept;
  MessageCursor(const MessageCursor&) = delete;
  MessageCursor& operator=(const MessageCursor&) = delete;

  // Stores the next message in output and returns false at end of input.
  bool next(Message& output);

 private:
  explicit MessageCursor(std::unique_ptr<Impl> impl);
  std::unique_ptr<Impl> impl_;

  friend class Reader;
  friend class AnyReader;
};

class TypeRegistry;
struct DecodedMessage;
using DecodedCallback = std::function<void(const Message&, const DecodedMessage&)>;

class Reader {
 public:
  explicit Reader(std::string path, ReaderOptions options = {});
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
  MessageCursor messages(const ReadFilter& filter = {}) const;
  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const;
  void read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                    const DecodedCallback& callback, UnknownTypePolicy policy = UnknownTypePolicy::WarnAndSkip,
                    const WarningCallback& warning = {}) const;

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

class AnyReader {
 public:
  explicit AnyReader(std::vector<std::string> paths, ReaderOptions options = {});
  ~AnyReader();
  AnyReader(const AnyReader&) = delete;
  AnyReader& operator=(const AnyReader&) = delete;
  AnyReader(AnyReader&&) noexcept;
  AnyReader& operator=(AnyReader&&) noexcept;

  void open();
  void close() noexcept;
  bool is_open() const noexcept;
  const ReaderMetadata& metadata() const;
  const std::vector<Connection>& connections() const;
  MessageCursor messages(const ReadFilter& filter = {}) const;
  void read_raw(const ReadFilter& filter, const MessageCallback& callback) const;
  void read_decoded(const ReadFilter& filter, const TypeRegistry& registry, std::string_view profile,
                    const DecodedCallback& callback, UnknownTypePolicy policy = UnknownTypePolicy::WarnAndSkip,
                    const WarningCallback& warning = {}) const;

 private:
  struct Impl;
  friend class AnyReaderCursorImpl;
  std::shared_ptr<Impl> impl_;
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

  TypeSupport(std::string name, std::string profile_name, Ros1Decoder ros1, CdrDecoder cdr,
              std::function<std::string(const T&)> format = {})
      : ros1_decoder(std::move(ros1)), cdr_decoder(std::move(cdr)), formatter(std::move(format)) {
    type_name = std::move(name);
    profile = std::move(profile_name);
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
    if (!support || support->type_name.empty() || support->profile.empty())
      throw RosbagsError("invalid type support");
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
    if (!support || !object || support->cpp_type != typeid(T)) throw DecodeError("decoded type mismatch");
    return *static_cast<const T*>(object.get());
  }
};

std::optional<DecodedMessage> decode(const Message& message, const TypeRegistry& registry,
                                     std::string_view profile, UnknownTypePolicy policy,
                                     const WarningCallback& warning = {});

std::string storage_kind_name(StorageKind kind);
std::string definition_format_name(DefinitionFormat format);
// Normalization preserves relative versus absolute names; it does not resolve names.
std::string normalize_topic(std::string_view topic);
bool topics_match(std::string_view left, std::string_view right,
                  TopicMatchPolicy policy = TopicMatchPolicy::Strict,
                  std::string_view topic_namespace = {});
// Bag timestamps are unsigned integer nanoseconds. Negative seconds, invalid
// subsecond values and overflow are rejected; conversion to microseconds floors.
std::uint64_t timestamp_nanoseconds(std::int64_t seconds, std::uint32_t nanoseconds);
std::uint64_t nanoseconds_to_microseconds(std::uint64_t nanoseconds) noexcept;
std::uint64_t microseconds_to_nanoseconds(std::uint64_t microseconds);
std::string normalize_type(std::string_view type);

}  // namespace rosbags
