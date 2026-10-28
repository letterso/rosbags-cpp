#include <doctest/doctest.h>
#include "support/temp_directory.hpp"

#include <rosbags/rosbags.hpp>
#include <rosbags/profiles.hpp>
#include <rosbags/codegen.hpp>

#include <sqlite3.h>
#include <lz4frame.h>
#include <zstd.h>
#if ROSBAGS_HAS_MCAP
#include <mcap/writer.hpp>
#endif

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

using rosbags::Byte;
using rosbags::Bytes;

void put32(Bytes& output, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) output.push_back(static_cast<Byte>(value >> (i * 8U)));
}
void put64(Bytes& output, std::uint64_t value) {
  for (unsigned i = 0; i < 8; ++i) output.push_back(static_cast<Byte>(value >> (i * 8U)));
}
Bytes serialized_fields(std::initializer_list<std::pair<std::string, Bytes>> fields) {
  Bytes result;
  for (const auto& field : fields) {
    const auto text = field.first + "=";
    Bytes value(text.begin(), text.end());
    value.insert(value.end(), field.second.begin(), field.second.end());
    put32(result, static_cast<std::uint32_t>(value.size()));
    result.insert(result.end(), value.begin(), value.end());
  }
  return result;
}
Bytes header(std::initializer_list<std::pair<std::string, Bytes>> fields) {
  const auto body = serialized_fields(fields);
  Bytes result;
  put32(result, static_cast<std::uint32_t>(body.size()));
  result.insert(result.end(), body.begin(), body.end());
  return result;
}
Bytes record(std::initializer_list<std::pair<std::string, Bytes>> fields, const Bytes& body) {
  auto result = header(fields);
  put32(result, static_cast<std::uint32_t>(body.size()));
  result.insert(result.end(), body.begin(), body.end());
  return result;
}
Bytes u8(std::uint8_t value) { return Bytes{value}; }
Bytes u32(std::uint32_t value) { Bytes result; put32(result, value); return result; }
Bytes u64(std::uint64_t value) { Bytes result; put64(result, value); return result; }
Bytes time_value(std::uint32_t sec, std::uint32_t nsec) { Bytes result; put32(result, sec); put32(result, nsec); return result; }
void put_cdr_string(Bytes& output, std::string_view value) {
  put32(output, static_cast<std::uint32_t>(value.size() + 1));
  output.insert(output.end(), value.begin(), value.end());
  output.push_back(0);
}
void align_cdr(Bytes& output, std::size_t alignment) {
  const auto relative = (output.size() - 4) % alignment;
  if (relative) output.resize(output.size() + alignment - relative, 0);
}

std::filesystem::path temp_path(const std::string& suffix) {
  static rosbags::test::TempDirectory directory;
  static std::uint32_t counter = 0;
  return directory.path() / (std::to_string(++counter) + suffix);
}

struct Ros1TestMessage {
  std::uint64_t timestamp;
  Bytes payload;
};

void write_ros1_bag(const std::filesystem::path& path,
                    const std::vector<std::vector<Ros1TestMessage>>& message_chunks,
                    bool use_lz4 = false) {
  struct ChunkData {
    Bytes bytes;
    std::uint64_t start = 0;
    std::uint64_t end = 0;
  };
  std::vector<ChunkData> chunks;
  for (const auto& messages : message_chunks) {
    REQUIRE_FALSE(messages.empty());
    Bytes payload;
    Bytes index_body;
    auto start = messages.front().timestamp;
    auto end = messages.front().timestamp;
    for (const auto& item : messages) {
      const auto offset = static_cast<std::uint32_t>(payload.size());
      const auto sec = static_cast<std::uint32_t>(item.timestamp / 1000000000ULL);
      const auto nsec = static_cast<std::uint32_t>(item.timestamp % 1000000000ULL);
      const auto message = record({{"op", u8(2)}, {"conn", u32(0)}, {"time", time_value(sec, nsec)}}, item.payload);
      payload.insert(payload.end(), message.begin(), message.end());
      put32(index_body, sec);
      put32(index_body, nsec);
      put32(index_body, offset);
      start = std::min(start, item.timestamp);
      end = std::max(end, item.timestamp);
    }
    auto chunk_payload = payload;
    Bytes compression{'n','o','n','e'};
    if (use_lz4) {
      chunk_payload.resize(LZ4F_compressFrameBound(payload.size(), nullptr));
      const auto compressed_size = LZ4F_compressFrame(
          chunk_payload.data(), chunk_payload.size(), payload.data(), payload.size(), nullptr);
      REQUIRE_FALSE(LZ4F_isError(compressed_size));
      chunk_payload.resize(compressed_size);
      compression = Bytes{'l','z','4'};
    }
    auto bytes = record({{"op", u8(5)}, {"compression", compression},
                         {"size", u32(static_cast<std::uint32_t>(payload.size()))}}, chunk_payload);
    const auto index = record({{"op", u8(4)}, {"ver", u32(1)}, {"conn", u32(0)},
                               {"count", u32(static_cast<std::uint32_t>(messages.size()))}}, index_body);
    bytes.insert(bytes.end(), index.begin(), index.end());
    chunks.push_back({std::move(bytes), start, end});
  }
  std::vector<std::uint64_t> positions;
  std::uint64_t position = 13 + 4096;
  for (const auto& chunk : chunks) {
    positions.push_back(position);
    position += chunk.bytes.size();
  }
  const auto index_position = position;
  const auto connection_data = serialized_fields({{"type", Bytes{'t','e','s','t','/','M'}},
                                                   {"md5sum", Bytes{'x'}},
                                                   {"message_definition", Bytes{'i','n','t','3','2',' ','v'}}});
  const auto connection = record({{"op", u8(7)}, {"conn", u32(0)}, {"topic", Bytes{'/','c'}}}, connection_data);
  Bytes index_section = connection;
  for (std::size_t index = 0; index < chunks.size(); ++index) {
    const auto start_sec = static_cast<std::uint32_t>(chunks[index].start / 1000000000ULL);
    const auto start_nsec = static_cast<std::uint32_t>(chunks[index].start % 1000000000ULL);
    const auto end_sec = static_cast<std::uint32_t>(chunks[index].end / 1000000000ULL);
    const auto end_nsec = static_cast<std::uint32_t>(chunks[index].end % 1000000000ULL);
    Bytes counts;
    put32(counts, 0);
    put32(counts, static_cast<std::uint32_t>(message_chunks[index].size()));
    const auto info = record({{"op", u8(6)}, {"ver", u32(1)}, {"chunk_pos", u64(positions[index])},
                              {"start_time", time_value(start_sec, start_nsec)},
                              {"end_time", time_value(end_sec, end_nsec)}, {"count", u32(1)}}, counts);
    index_section.insert(index_section.end(), info.begin(), info.end());
  }
  auto bag_header = header({{"op", u8(3)}, {"index_pos", u64(index_position)}, {"conn_count", u32(1)},
                            {"chunk_count", u32(static_cast<std::uint32_t>(chunks.size()))}});
  const auto padding = static_cast<std::uint32_t>(4096 - 4 - bag_header.size());
  put32(bag_header, padding);
  bag_header.resize(4096, 0x20);
  std::ofstream output(path, std::ios::binary);
  output << "#ROSBAG V2.0\n";
  output.write(reinterpret_cast<const char*>(bag_header.data()), static_cast<std::streamsize>(bag_header.size()));
  for (const auto& chunk : chunks)
    output.write(reinterpret_cast<const char*>(chunk.bytes.data()), static_cast<std::streamsize>(chunk.bytes.size()));
  output.write(reinterpret_cast<const char*>(index_section.data()), static_cast<std::streamsize>(index_section.size()));
}

TEST_CASE("reads SQLite bags in timestamp order" * doctest::test_suite("project")) {
  const auto path = temp_path(".dbs");
  sqlite3* database = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
  const char* schema =
      "CREATE TABLE schema(schema_version INTEGER PRIMARY KEY, ros_distro TEXT NOT NULL);"
      "INSERT INTO schema VALUES(4,'test');"
      "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT NOT NULL,type TEXT NOT NULL,serialization_format TEXT NOT NULL,offered_qos_profiles TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
      "CREATE TABLE message_definitions(id INTEGER PRIMARY KEY,topic_type TEXT NOT NULL,encoding TEXT NOT NULL,encoded_message_definition TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER NOT NULL,timestamp INTEGER NOT NULL,data BLOB NOT NULL);"
      "INSERT INTO topics VALUES(1,'/numbers','test_msgs/msg/Numbers','cdr','','');"
      "INSERT INTO message_definitions VALUES(1,'test_msgs/msg/Numbers','ros2msg','int32 value','');"
      "INSERT INTO messages VALUES(1,1,20,X'0102');"
      "INSERT INTO messages VALUES(2,1,10,X'0304');";
  REQUIRE(sqlite3_exec(database, schema, nullptr, nullptr, nullptr) == SQLITE_OK);
  sqlite3_close(database);

  rosbags::Reader reader(path.string());
  reader.open();
  CHECK(reader.storage_kind() == rosbags::StorageKind::Sqlite3);
  REQUIRE(reader.connections().size() == 1);
  CHECK(reader.metadata().message_count == 2);
  std::vector<std::uint64_t> timestamps;
  reader.read_raw({}, [&](const rosbags::Message& message) { timestamps.push_back(message.timestamp); });
  CHECK((timestamps == std::vector<std::uint64_t>{10, 20}));
  rosbags::ReadFilter filter;
  filter.topics = {"numbers"};
  rosbags::Message filtered;
  CHECK_FALSE(reader.messages(filter).next(filtered));
  filter.topic_match = rosbags::TopicMatchPolicy::IgnoreLeadingSlash;
  CHECK(reader.messages(filter).next(filtered));
  CHECK(filtered.source_path == path.string());
  filter.topic_match = rosbags::TopicMatchPolicy::ResolveNamespace;
  filter.topic_namespace = "/";
  CHECK(reader.messages(filter).next(filtered));
  {
    auto cursor = reader.messages();
    rosbags::Reader replacement(path.string());
    replacement.open();
    reader = std::move(replacement);
    CHECK(reader.is_open());
    rosbags::Message message;
    std::vector<std::uint64_t> cursor_timestamps;
    while (cursor.next(message)) cursor_timestamps.push_back(message.timestamp);
    CHECK(cursor_timestamps == timestamps);
  }
  reader.close();

  std::optional<rosbags::MessageCursor> detached;
  {
    rosbags::Reader scoped(path.string());
    scoped.open();
    detached.emplace(scoped.messages());
  }
  rosbags::Message detached_message;
  std::vector<std::uint64_t> detached_timestamps;
  while (detached->next(detached_message)) detached_timestamps.push_back(detached_message.timestamp);
  CHECK(detached_timestamps == timestamps);
  detached.reset();
  std::filesystem::remove(path);
}

TEST_CASE("reads ROS2 directory metadata and decoded messages" * doctest::test_suite("project")) {
  const auto directory = temp_path("_ros2");
  std::filesystem::remove_all(directory);
  std::filesystem::create_directories(directory);
  const auto path = directory / "bag_0.db3";
  sqlite3* database = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
  const char* schema =
      "CREATE TABLE schema(schema_version INTEGER PRIMARY KEY, ros_distro TEXT NOT NULL);"
      "INSERT INTO schema VALUES(4,'test');"
      "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT NOT NULL,type TEXT NOT NULL,serialization_format TEXT NOT NULL,offered_qos_profiles TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
      "CREATE TABLE message_definitions(id INTEGER PRIMARY KEY,topic_type TEXT NOT NULL,encoding TEXT NOT NULL,encoded_message_definition TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER NOT NULL,timestamp INTEGER NOT NULL,data BLOB NOT NULL);"
      "INSERT INTO topics VALUES(1,'/text','std_msgs/msg/String','cdr','','');"
      "INSERT INTO messages VALUES(1,1,42,X'0001000003000000686900');";
  REQUIRE(sqlite3_exec(database, schema, nullptr, nullptr, nullptr) == SQLITE_OK);
  sqlite3_close(database);

  std::ofstream metadata(directory / "metadata.yaml");
  metadata << "rosbag2_bagfile_information:\n"
           << "  version: 5\n"
           << "  storage_identifier: sqlite3\n"
           << "  relative_file_paths: [bag_0.db3]\n"
           << "  duration:\n"
           << "    nanoseconds: 0\n"
           << "  starting_time:\n"
           << "    nanoseconds_since_epoch: 42\n"
           << "  message_count: 1\n"
           << "  topics_with_message_count:\n"
           << "    - topic_metadata:\n"
           << "        name: /text\n"
           << "        type: std_msgs/msg/String\n"
           << "        serialization_format: cdr\n"
           << "      message_count: 1\n";
  metadata.close();

  rosbags::Reader reader(directory.string());
  reader.open();
  CHECK(reader.storage_kind() == rosbags::StorageKind::Sqlite3);
  REQUIRE(reader.connections().size() == 1);
  CHECK(reader.metadata().duration == 1);
  CHECK(reader.metadata().end_time == 43);
  std::size_t count = 0;
  reader.read_raw({}, [&](const rosbags::Message& message) {
    REQUIRE(message.connection != nullptr);
    CHECK(message.connection->topic == "/text");
    CHECK(message.timestamp == 42);
    CHECK(message.source_path == path.string());
    ++count;
  });
  CHECK(count == 1);
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry, "ros2_humble");
  std::size_t decoded_count = 0;
  reader.read_decoded({}, registry, "ros2_humble",
                     [&](const rosbags::Message& message, const rosbags::DecodedMessage& decoded) {
                       CHECK(message.timestamp == 42);
                       CHECK(decoded.as<rosbags::profiles::String>().data == "hi");
                       ++decoded_count;
                     },
                     rosbags::UnknownTypePolicy::Error);
  CHECK(decoded_count == 1);
  rosbags::TypeRegistry empty_registry;
  try {
    reader.read_decoded({}, empty_registry, "ros2_humble",
                        [](const auto&, const auto&) {}, rosbags::UnknownTypePolicy::Error);
    FAIL("expected unknown type in a shard");
  } catch (const rosbags::DecodeError& error) {
    CHECK(error.context().source_path == path.string());
    CHECK(error.context().timestamp == 42);
  }
  reader.close();
  std::filesystem::remove(path);
  try {
    reader.open();
    FAIL("expected missing shard");
  } catch (const rosbags::FormatError& error) {
    CHECK(error.context().source_path == path.string());
  }
  std::filesystem::remove_all(directory);
}

TEST_CASE("rejects truncated unknown-size zstd message frames" * doctest::test_suite("project")) {
  const auto directory = temp_path("_zstd_ros2");
  std::filesystem::remove_all(directory);
  std::filesystem::create_directories(directory);
  const auto path = directory / "bag_0.db3";

  Bytes payload(128 * 1024, 0x5a);
  std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> context(ZSTD_createCCtx(), &ZSTD_freeCCtx);
  REQUIRE(context != nullptr);
  REQUIRE(!ZSTD_isError(ZSTD_CCtx_setParameter(context.get(), ZSTD_c_contentSizeFlag, 0)));
  Bytes compressed(ZSTD_compressBound(payload.size()));
  const auto compressed_size = ZSTD_compress2(
      context.get(), compressed.data(), compressed.size(), payload.data(), payload.size());
  REQUIRE(!ZSTD_isError(compressed_size));
  compressed.resize(compressed_size);
  REQUIRE(ZSTD_getFrameContentSize(compressed.data(), compressed.size()) == ZSTD_CONTENTSIZE_UNKNOWN);

  sqlite3* database = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
  const char* schema =
      "CREATE TABLE schema(schema_version INTEGER PRIMARY KEY, ros_distro TEXT NOT NULL);"
      "INSERT INTO schema VALUES(4,'test');"
      "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT NOT NULL,type TEXT NOT NULL,serialization_format TEXT NOT NULL,offered_qos_profiles TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER NOT NULL,timestamp INTEGER NOT NULL,data BLOB NOT NULL);"
      "INSERT INTO topics VALUES(1,'/compressed','test_msgs/msg/Bytes','cdr','','');";
  REQUIRE(sqlite3_exec(database, schema, nullptr, nullptr, nullptr) == SQLITE_OK);
  sqlite3_stmt* insert = nullptr;
  REQUIRE(sqlite3_prepare_v2(database, "INSERT INTO messages VALUES(1,1,42,?)", -1, &insert, nullptr) ==
          SQLITE_OK);
  REQUIRE(sqlite3_bind_blob(insert, 1, compressed.data(), static_cast<int>(compressed.size()), SQLITE_TRANSIENT) ==
          SQLITE_OK);
  REQUIRE(sqlite3_step(insert) == SQLITE_DONE);
  sqlite3_finalize(insert);
  sqlite3_close(database);

  std::ofstream metadata(directory / "metadata.yaml");
  metadata << "rosbag2_bagfile_information:\n"
           << "  version: 5\n"
           << "  storage_identifier: sqlite3\n"
           << "  relative_file_paths: [bag_0.db3]\n"
           << "  duration: {nanoseconds: 0}\n"
           << "  starting_time: {nanoseconds_since_epoch: 42}\n"
           << "  message_count: 1\n"
           << "  compression_format: zstd\n"
           << "  compression_mode: message\n"
           << "  topics_with_message_count:\n"
           << "    - topic_metadata:\n"
           << "        name: /compressed\n"
           << "        type: test_msgs/msg/Bytes\n"
           << "        serialization_format: cdr\n"
           << "      message_count: 1\n";
  metadata.close();

  rosbags::Reader reader(directory.string());
  reader.open();
  std::size_t count = 0;
  reader.read_raw({}, [&](const rosbags::Message& message) {
    REQUIRE(message.bytes != nullptr);
    CHECK(*message.bytes == payload);
    ++count;
  });
  CHECK(count == 1);
  reader.close();

  rosbags::ReaderOptions output_limit;
  output_limit.max_message_bytes = 64 * 1024;
  rosbags::Reader limited(directory.string(), output_limit);
  limited.open();
  CHECK_THROWS_AS(limited.read_raw({}, [](const rosbags::Message&) {}), rosbags::ResourceLimitError);
  limited.close();

  compressed.pop_back();
  REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
  sqlite3_stmt* update = nullptr;
  REQUIRE(sqlite3_prepare_v2(database, "UPDATE messages SET data=? WHERE id=1", -1, &update, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_bind_blob(update, 1, compressed.data(), static_cast<int>(compressed.size()), SQLITE_TRANSIENT) ==
          SQLITE_OK);
  REQUIRE(sqlite3_step(update) == SQLITE_DONE);
  sqlite3_finalize(update);
  sqlite3_close(database);

  reader.open();
  CHECK_THROWS_AS(reader.read_raw({}, [](const rosbags::Message&) {}), rosbags::FormatError);
  reader.close();
  std::filesystem::remove_all(directory);
}

TEST_CASE("reads ROS1 bag chunks and indexes" * doctest::test_suite("project")) {
  const auto path = temp_path(".bag");
  Bytes chunk;
  const auto connection_data = serialized_fields({{"type", Bytes{'t','e','s','t','/','M'}}, {"md5sum", Bytes{'x'}}, {"message_definition", Bytes{'i','n','t','3','2',' ','v','a','l','u','e'}}});
  const auto connection = record({{"op", u8(7)}, {"conn", u32(0)}, {"topic", Bytes{'/','c'}}}, connection_data);
  chunk.insert(chunk.end(), connection.begin(), connection.end());
  const auto message_body = Bytes{1, 2, 3, 4};
  const auto message = record({{"op", u8(2)}, {"conn", u32(0)}, {"time", time_value(1, 2)}}, message_body);
  const auto message_offset = static_cast<std::uint32_t>(chunk.size());
  chunk.insert(chunk.end(), message.begin(), message.end());
  const auto chunk_record = record({{"op", u8(5)}, {"compression", Bytes{'n','o','n','e'}}, {"size", u32(static_cast<std::uint32_t>(chunk.size()))}}, chunk);
  const auto index_record = record({{"op", u8(4)}, {"ver", u32(1)}, {"conn", u32(0)}, {"count", u32(1)}}, [&] { Bytes value; put32(value, 1); put32(value, 2); put32(value, message_offset); return value; }());
  const std::uint64_t chunk_position = 13 + 4096;
  Bytes chunk_with_index = chunk_record;
  chunk_with_index.insert(chunk_with_index.end(), index_record.begin(), index_record.end());
  const auto chunk_info = record({{"op", u8(6)}, {"ver", u32(1)}, {"chunk_pos", u64(chunk_position)}, {"start_time", time_value(1, 2)}, {"end_time", time_value(1, 2)}, {"count", u32(1)}}, [&] { Bytes value; put32(value, 0); put32(value, 1); return value; }());
  const std::uint64_t index_position = chunk_position + chunk_with_index.size();
  Bytes bag_header = header({{"op", u8(3)}, {"index_pos", u64(index_position)}, {"conn_count", u32(1)}, {"chunk_count", u32(1)}});
  const auto padding = static_cast<std::uint32_t>(4096 - 4 - bag_header.size());
  put32(bag_header, padding);
  bag_header.resize(4096, 0x20);
  std::ofstream output(path, std::ios::binary);
  output << "#ROSBAG V2.0\n";
  output.write(reinterpret_cast<const char*>(bag_header.data()), static_cast<std::streamsize>(bag_header.size()));
  output.write(reinterpret_cast<const char*>(chunk_with_index.data()), static_cast<std::streamsize>(chunk_with_index.size()));
  output.write(reinterpret_cast<const char*>(connection.data()), static_cast<std::streamsize>(connection.size()));
  output.write(reinterpret_cast<const char*>(chunk_info.data()), static_cast<std::streamsize>(chunk_info.size()));
  output.close();

  rosbags::Reader reader(path.string());
  reader.open();
  REQUIRE(reader.connections().size() == 1);
  CHECK(reader.connections()[0].topic == "/c");
  std::size_t count = 0;
  reader.read_raw({}, [&](const rosbags::Message& output_message) {
    CHECK(output_message.timestamp == 1000000002ULL);
    REQUIRE(output_message.bytes != nullptr);
    CHECK(output_message.bytes->size() == 4);
    ++count;
  });
  CHECK(count == 1);
  reader.close();
  std::filesystem::remove(path);
}

TEST_CASE("AnyReader merges cursor heads in stable timestamp order" * doctest::test_suite("project")) {
  const auto first = temp_path("_first.db3");
  const auto second = temp_path("_second.db3");
  const auto write_sqlite = [](const std::filesystem::path& path, const char* topic, const char* rows) {
    sqlite3* database = nullptr;
    REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
    const std::string schema =
        "CREATE TABLE schema(schema_version INTEGER PRIMARY KEY, ros_distro TEXT NOT NULL);"
        "INSERT INTO schema VALUES(4,'test');"
        "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT NOT NULL,type TEXT NOT NULL,serialization_format TEXT NOT NULL,offered_qos_profiles TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
        "CREATE TABLE message_definitions(id INTEGER PRIMARY KEY,topic_type TEXT NOT NULL,encoding TEXT NOT NULL,encoded_message_definition TEXT NOT NULL,type_description_hash TEXT NOT NULL);"
        "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER NOT NULL,timestamp INTEGER NOT NULL,data BLOB NOT NULL);"
        "INSERT INTO topics VALUES(1,'" + std::string(topic) + "','test_msgs/msg/Numbers','cdr','','');"
        "INSERT INTO message_definitions VALUES(1,'test_msgs/msg/Numbers','ros2msg','int32 value','');" + rows;
    REQUIRE(sqlite3_exec(database, schema.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(database);
  };
  write_sqlite(first, "/first", "INSERT INTO messages VALUES(1,1,20,X'01');INSERT INTO messages VALUES(2,1,10,X'02');");
  write_sqlite(second, "/second", "INSERT INTO messages VALUES(1,1,15,X'03');INSERT INTO messages VALUES(2,1,10,X'04');");

  rosbags::AnyReader reader({first.string(), second.string()});
  reader.open();
  {
    auto cursor = reader.messages();
    auto moved = std::move(reader);
    CHECK_FALSE(reader.is_open());
    rosbags::Message message;
    std::vector<std::string> observed;
    while (cursor.next(message)) observed.push_back(std::to_string(message.timestamp) + message.connection->topic);
    CHECK((observed == std::vector<std::string>{"10/first", "10/second", "15/second", "20/first"}));
    reader = std::move(moved);
    CHECK(reader.is_open());
  }
  {
    rosbags::ReadFilter filter;
    filter.connection_ids = {2};
    auto cursor = reader.messages(filter);
    rosbags::Message message;
    REQUIRE(cursor.next(message));
    CHECK(message.connection->topic == "/second");
    CHECK(message.timestamp == 10);
    REQUIRE(cursor.next(message));
    CHECK(message.connection->topic == "/second");
    CHECK(message.timestamp == 15);
    CHECK_FALSE(cursor.next(message));
  }
  reader.close();

  std::optional<rosbags::MessageCursor> detached;
  {
    rosbags::AnyReader scoped({first.string(), second.string()});
    scoped.open();
    detached.emplace(scoped.messages());
  }
  rosbags::Message detached_message;
  std::size_t detached_count = 0;
  while (detached->next(detached_message)) {
    CHECK(detached_message.connection != nullptr);
    ++detached_count;
  }
  CHECK(detached_count == 4);
  detached.reset();

  std::filesystem::remove(first);
  std::filesystem::remove(second);
}

TEST_CASE("ROS1 cursor limits one chunk without limiting the whole bag" * doctest::test_suite("project")) {
  const auto oversized = temp_path("_oversized.bag");
  write_ros1_bag(oversized, {{{1000000001ULL, Bytes(96, 0x31)}}});
  rosbags::ReaderOptions strict;
  strict.max_rosbag1_chunk_bytes = 64;
  rosbags::Reader limited(oversized.string(), strict);
  limited.open();
  CHECK_THROWS_AS(limited.read_raw({}, [](const rosbags::Message&) {}), rosbags::ResourceLimitError);
  limited.close();
  std::filesystem::remove(oversized);

  const auto multi_chunk = temp_path("_multi_chunk.bag");
  write_ros1_bag(multi_chunk, {
      {{1000000001ULL, Bytes(12, 0x11)}},
      {{1000000002ULL, Bytes(12, 0x22)}},
      {{1000000003ULL, Bytes(12, 0x33)}},
  });
  rosbags::ReaderOptions per_chunk;
  per_chunk.max_rosbag1_chunk_bytes = 128;
  rosbags::Reader reader(multi_chunk.string(), per_chunk);
  reader.open();
  std::size_t count = 0;
  reader.read_raw({}, [&](const rosbags::Message& message) {
    CHECK(message.bytes->size() == 12);
    ++count;
  });
  CHECK(count == 3);
  reader.close();
  std::filesystem::remove(multi_chunk);
}

TEST_CASE("ROS1 cursor streams LZ4 chunks" * doctest::test_suite("project")) {
  const auto path = temp_path("_lz4.bag");
  const Bytes payload(96, 0x4a);
  write_ros1_bag(path, {{{1000000001ULL, payload}, {1000000002ULL, payload},
                         {1000000003ULL, payload}}}, true);
  rosbags::ReaderOptions options;
  options.max_rosbag1_chunk_bytes = 512;
  rosbags::AnyReader reader({path.string()}, options);
  reader.open();
  std::size_t count = 0;
  reader.read_raw({}, [&](const rosbags::Message& message) {
    CHECK(*message.bytes == payload);
    ++count;
  });
  CHECK(count == 3);
  reader.close();
  std::filesystem::remove(path);
}

TEST_CASE("decodes built-in profiles and handles unknown types" * doctest::test_suite("project")) {
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry, "ros2_humble");
  rosbags::Connection connection;
  connection.topic = "/text";
  connection.type = "std_msgs/msg/String";
  connection.serialization_format = "cdr";
  auto bytes = std::make_shared<Bytes>(Bytes{0, 1, 0, 0, 3, 0, 0, 0, 'h', 'i', 0});
  rosbags::Message message{bytes, 1, &connection};
  const auto decoded = rosbags::decode(message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
  REQUIRE(decoded);
  CHECK(decoded->as<rosbags::profiles::String>().data == "hi");

  rosbags::Connection unknown_connection;
  unknown_connection.topic = "/unknown";
  unknown_connection.type = "example/msg/Unknown";
  unknown_connection.serialization_format = "cdr";
  rosbags::Message unknown{bytes, 2, &unknown_connection};
  bool warned = false;
  const auto raw = rosbags::decode(unknown, registry, "ros2_humble", rosbags::UnknownTypePolicy::WarnAndRaw,
                                   [&](std::string_view text) { warned = text.find("unregistered") != std::string_view::npos; });
  REQUIRE(raw);
  CHECK(raw->raw_only());
  CHECK(warned);
  CHECK(raw->source == &unknown);
  CHECK_FALSE(rosbags::decode(unknown, registry, "ros2_humble", rosbags::UnknownTypePolicy::WarnAndSkip));

  Bytes cdr_vector{0, 1, 0, 0};
  put64(cdr_vector, 0);
  put64(cdr_vector, 0);
  put64(cdr_vector, 0);
  rosbags::Connection cdr_connection;
  cdr_connection.topic = "/vector";
  cdr_connection.type = "geometry_msgs/msg/Vector3";
  cdr_connection.serialization_format = "cdr";
  rosbags::Message cdr_message{std::make_shared<Bytes>(cdr_vector), 3, &cdr_connection};
  const auto cdr_decoded = rosbags::decode(cdr_message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
  REQUIRE(cdr_decoded);
  CHECK(cdr_decoded->as<rosbags::profiles::Vector3>().x == 0);

  Bytes ros1_imu;
  put32(ros1_imu, 7);
  put32(ros1_imu, 1);
  put32(ros1_imu, 2);
  put32(ros1_imu, 4);
  ros1_imu.insert(ros1_imu.end(), {'i', 'm', 'u', '4'});
  ros1_imu.resize(ros1_imu.size() + 32 + 216 + 48, 0);
  rosbags::Connection ros1_connection;
  ros1_connection.topic = "/imu0";
  ros1_connection.type = "sensor_msgs/msg/Imu";
  ros1_connection.serialization_format = "ros1";
  rosbags::Message ros1_message{std::make_shared<Bytes>(ros1_imu), 4, &ros1_connection};
  rosbags::profiles::register_builtin_types(registry, "ros1_noetic");
  const auto ros1_decoded = rosbags::decode(ros1_message, registry, "ros1_noetic", rosbags::UnknownTypePolicy::Error);
  REQUIRE(ros1_decoded);
  CHECK(ros1_decoded->as<rosbags::profiles::Imu>().header.frame_id == "imu4");

}

TEST_CASE("registers ROS1 and ROS2 built-in message packages" * doctest::test_suite("project")) {
  rosbags::TypeRegistry invalid;
  std::shared_ptr<const rosbags::TypeSupport<rosbags::profiles::String>> null_support;
  CHECK_THROWS_AS(invalid.register_type(null_support), rosbags::RosbagsError);

  rosbags::TypeRegistry ros1;
  rosbags::profiles::register_builtin_types(ros1, "ros1_noetic");
  CHECK(ros1.find("ros1_noetic", "std_msgs/Bool") != nullptr);
  CHECK(ros1.find("ros1_noetic", "geometry_msgs/Pose") != nullptr);
  CHECK(ros1.find("ros1_noetic", "sensor_msgs/PointCloud2") != nullptr);
  CHECK(ros1.find("ros1_noetic", "nav_msgs/Odometry") != nullptr);
  CHECK(ros1.find("ros1_noetic", "std_msgs/Time") != nullptr);

  rosbags::TypeRegistry foxy;
  rosbags::profiles::register_builtin_types(foxy, "ros2_foxy");
  CHECK(foxy.find("ros2_foxy", "std_msgs/Header") != nullptr);
  CHECK(foxy.find("ros2_foxy", "geometry_msgs/PolygonInstance") == nullptr);
  CHECK(foxy.find("ros2_foxy", "std_msgs/Time") == nullptr);

  rosbags::TypeRegistry humble;
  rosbags::profiles::register_builtin_types(humble, "ros2_humble");
  CHECK(humble.find("ros2_humble", "geometry_msgs/PolygonInstance") != nullptr);
  CHECK(humble.find("ros2_humble", "geometry_msgs/VelocityStamped") != nullptr);
  CHECK(humble.find("ros2_humble", "sensor_msgs/Image") != nullptr);
  CHECK(humble.find("ros2_humble", "nav_msgs/Path") != nullptr);
}

TEST_CASE("decodes profile-specific Header and CDR sensor messages" * doctest::test_suite("project")) {
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry, "ros1_noetic");
  rosbags::Connection header_connection;
  header_connection.type = "std_msgs/Header";
  header_connection.serialization_format = "ros1";
  Bytes ros1_header;
  put32(ros1_header, 19);
  put32(ros1_header, 3);
  put32(ros1_header, 4);
  put32(ros1_header, 6);
  ros1_header.insert(ros1_header.end(), {'c', 'a', 'm', 'e', 'r', 'a'});
  rosbags::Message header_message{std::make_shared<Bytes>(ros1_header), 0, &header_connection};
  const auto header_decoded = rosbags::decode(header_message, registry, "ros1_noetic", rosbags::UnknownTypePolicy::Error);
  REQUIRE(header_decoded);
  const auto& decoded_header = header_decoded->as<rosbags::profiles::Header>();
  CHECK(decoded_header.seq == 19);
  CHECK(decoded_header.stamp.sec == 3);
  CHECK(decoded_header.stamp.nanosec == 4);
  CHECK(decoded_header.frame_id == "camera");

  rosbags::profiles::register_builtin_types(registry, "ros2_humble");
  rosbags::Connection image_connection;
  image_connection.type = "sensor_msgs/msg/Image";
  image_connection.serialization_format = "cdr";
  Bytes image{0, 1, 0, 0};
  put32(image, 1);
  put32(image, 2);
  put_cdr_string(image, "camera");
  align_cdr(image, 4);
  put32(image, 480);
  put32(image, 640);
  put_cdr_string(image, "mono8");
  image.push_back(0);
  align_cdr(image, 4);
  put32(image, 640);
  put32(image, 2);
  image.insert(image.end(), {11, 22});
  rosbags::Message image_message{std::make_shared<Bytes>(image), 0, &image_connection};
  const auto image_decoded = rosbags::decode(image_message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
  REQUIRE(image_decoded);
  const auto& decoded_image = image_decoded->as<rosbags::profiles::sensor_msgs::Image>();
  CHECK(decoded_image.header.frame_id == "camera");
  CHECK(decoded_image.height == 480);
  CHECK(decoded_image.width == 640);
  CHECK(decoded_image.encoding == "mono8");
  CHECK(decoded_image.step == 640);
  CHECK((decoded_image.data == std::vector<std::uint8_t>{11, 22}));

  image.push_back(0xff);
  rosbags::Message trailing_image{std::make_shared<Bytes>(image), 0, &image_connection};
  CHECK_THROWS_AS(rosbags::decode(trailing_image, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error), rosbags::DecodeError);
}

TEST_CASE("generates C++ types and registry from message definitions" * doctest::test_suite("project")) {
  const auto output = temp_path("_generated");
  std::filesystem::remove_all(output);
  rosbags::codegen::GenerateOptions options;
  options.profile = "ros2_humble";
  options.inputs = {(std::filesystem::path(__FILE__).parent_path() / "data" / "simple").string()};
  options.output_directory = output.string();
  rosbags::codegen::generate(options);
  const auto header = output / "ros2_humble_messages.hpp";
  const auto registry = output / "ros2_humble_registry.hpp";
  CHECK(std::filesystem::exists(header));
  CHECK(std::filesystem::exists(registry));
  std::ifstream generated(header);
  const std::string text((std::istreambuf_iterator<char>(generated)), std::istreambuf_iterator<char>());
  CHECK(text.find("struct Pair") != std::string::npos);
  CHECK(text.find("deserialize_cdr_simple_msg_Pair") != std::string::npos);
  CHECK(text.find("using rosbags::profiles::detail::read_ros1") != std::string::npos);
  std::filesystem::remove_all(output);

  const auto builtin_input = temp_path("_builtin_definitions");
  std::filesystem::create_directories(builtin_input / "demo" / "msg");
  std::ofstream definition(builtin_input / "demo" / "msg" / "Wrapper.msg");
  definition << "std_msgs/Header header\ngeometry_msgs/Point point\nsensor_msgs/Image image\nnav_msgs/Path path\n";
  definition.close();
  const auto builtin_output = temp_path("_builtin_generated");
  rosbags::codegen::GenerateOptions builtin_options;
  builtin_options.profile = "ros2_humble";
  builtin_options.inputs = {(builtin_input / "demo").string()};
  builtin_options.output_directory = builtin_output.string();
  rosbags::codegen::generate(builtin_options);
  std::ifstream builtin_header(builtin_output / "ros2_humble_messages.hpp");
  const std::string builtin_text((std::istreambuf_iterator<char>(builtin_header)), std::istreambuf_iterator<char>());
  CHECK(builtin_text.find("::rosbags::profiles::std_msgs::Header header") != std::string::npos);
  CHECK(builtin_text.find("::rosbags::profiles::geometry_msgs::Point point") != std::string::npos);
  CHECK(builtin_text.find("::rosbags::profiles::sensor_msgs::Image image") != std::string::npos);
  CHECK(builtin_text.find("::rosbags::profiles::nav_msgs::Path path") != std::string::npos);
  std::filesystem::remove_all(builtin_input);
  std::filesystem::remove_all(builtin_output);

}

}  // namespace

TEST_CASE("topic matching is explicit and namespace aware" * doctest::test_suite("project")) {
  using rosbags::TopicMatchPolicy;
  CHECK(rosbags::normalize_topic("imu//data/") == "imu/data");
  CHECK(rosbags::topics_match("/imu//data/", "/imu/data"));
  CHECK_FALSE(rosbags::topics_match("imu", "/imu"));
  CHECK(rosbags::topics_match("imu", "///imu/", TopicMatchPolicy::IgnoreLeadingSlash));
  CHECK(rosbags::topics_match("imu", "/robot/imu", TopicMatchPolicy::ResolveNamespace, "/robot//"));
  CHECK_FALSE(rosbags::topics_match("/imu", "/robot/imu", TopicMatchPolicy::ResolveNamespace, "/robot"));
  CHECK_THROWS_AS(rosbags::topics_match("imu", "imu", TopicMatchPolicy::ResolveNamespace, "robot"), rosbags::RosbagsError);
}

TEST_CASE("integer timestamp conversions check ranges" * doctest::test_suite("project")) {
  const auto maximum = std::numeric_limits<std::uint64_t>::max();
  CHECK(rosbags::timestamp_nanoseconds(0, 1) == 1);
  CHECK(rosbags::timestamp_nanoseconds(1, 999999999) == 1999999999);
  CHECK(rosbags::timestamp_nanoseconds(maximum / 1000000000, maximum % 1000000000) == maximum);
  CHECK_THROWS_AS(rosbags::timestamp_nanoseconds(-1, 0), rosbags::RosbagsError);
  CHECK_THROWS_AS(rosbags::timestamp_nanoseconds(0, 1000000000), rosbags::RosbagsError);
  CHECK_THROWS_AS(rosbags::timestamp_nanoseconds(maximum / 1000000000, maximum % 1000000000 + 1), rosbags::RosbagsError);
  CHECK(rosbags::nanoseconds_to_microseconds(1999) == 1);
  CHECK(rosbags::microseconds_to_nanoseconds(maximum / 1000) == maximum - maximum % 1000);
  CHECK_THROWS_AS(rosbags::microseconds_to_nanoseconds(maximum / 1000 + 1), rosbags::RosbagsError);
}

TEST_CASE("decode errors retain category reason and structured context" * doctest::test_suite("project")) {
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry, "ros2_humble");
  rosbags::Connection connection;
  connection.id = 7;
  connection.topic = "/imu";
  connection.type = "sensor_msgs/Imu";
  connection.serialization_format = "cdr";
  rosbags::Message message{std::make_shared<Bytes>(Bytes{0, 1, 0, 0}), 123, &connection, "part_2.db3"};
  try {
    rosbags::decode(message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
    FAIL("expected truncated IMU");
  } catch (const rosbags::DecodeError& error) {
    CHECK(error.context().source_path == message.source_path);
    CHECK(error.context().connection_id == 7);
    CHECK(error.context().topic == "/imu");
    CHECK(error.context().type == "sensor_msgs/msg/Imu");
    CHECK(error.context().serialization_format == "cdr");
    CHECK(error.context().timestamp == 123);
    CHECK(error.context().byte_offset == 4);
    CHECK(std::string(error.reason()) == "serialized message is truncated");
    CHECK(std::string(error.what()).find("part_2.db3") != std::string::npos);
  }
  connection.serialization_format = "other";
  try {
    rosbags::decode(message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
    FAIL("expected unsupported format");
  } catch (const rosbags::UnsupportedFeature& error) {
    CHECK(error.context().serialization_format == "other");
    CHECK(error.context().timestamp == 123);
  }
  connection.type = "custom/Unknown";
  try {
    rosbags::decode(message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
    FAIL("expected unregistered type");
  } catch (const rosbags::DecodeError& error) {
    CHECK(error.context().type == "custom/msg/Unknown");
    CHECK_FALSE(error.context().byte_offset);
  }
}

#if ROSBAGS_HAS_MCAP
TEST_CASE("MCAP filtering preserves name policy and source context" * doctest::test_suite("project")) {
  const auto path = temp_path(".mcap");
  mcap::McapWriter writer;
  mcap::McapWriterOptions options("ros2");
  options.forceCompression = true;
  REQUIRE(writer.open(path.string(), options).ok());
  mcap::Schema schema("std_msgs/msg/String", "ros2msg", "string data");
  writer.addSchema(schema);
  mcap::Channel channel("/robot//text/", "cdr", schema.id);
  writer.addChannel(channel);
  const Bytes payload{0, 1, 0, 0, 3, 0, 0, 0, 'h', 'i', 0};
  mcap::Message record{};
  record.channelId = channel.id;
  record.data = reinterpret_cast<const std::byte*>(payload.data());
  record.dataSize = payload.size();
  for (const auto time : {20, 10}) {
    record.logTime = record.publishTime = time;
    REQUIRE(writer.write(record).ok());
  }
  writer.close();
  rosbags::Reader reader(path.string());
  reader.open();
  rosbags::ReadFilter filter;
  filter.topics = {"robot/text"};
  rosbags::Message message;
  CHECK_FALSE(reader.messages(filter).next(message));
  filter.topic_match = rosbags::TopicMatchPolicy::IgnoreLeadingSlash;
  CHECK(reader.messages(filter).next(message));
  filter.topics = {"text"};
  filter.topic_match = rosbags::TopicMatchPolicy::ResolveNamespace;
  filter.topic_namespace = "/robot";
  auto cursor = reader.messages(filter);
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry, "ros2_humble");
  for (const auto time : {10, 20}) {
    REQUIRE(cursor.next(message));
    CHECK(message.timestamp == time);
    CHECK(message.connection->original_topic == "/robot//text/");
    CHECK(message.source_path == path.string());
    auto decoded = rosbags::decode(message, registry, "ros2_humble", rosbags::UnknownTypePolicy::Error);
    REQUIRE(decoded);
    CHECK(decoded->as<rosbags::profiles::String>().data == "hi");
  }
  CHECK_FALSE(cursor.next(message));
}
#endif

TEST_CASE("SQLite time filters bind precisely and preserve unsigned boundaries" * doctest::test_suite("project")) {
  const auto path = temp_path("_ranges.db3");
  sqlite3* database = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
  REQUIRE(sqlite3_exec(database,
      "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT,type TEXT,serialization_format TEXT);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER,timestamp INTEGER,data BLOB);"
      "INSERT INTO topics VALUES(1,'/sensor','test/M','cdr');"
      "INSERT INTO messages VALUES(1,1,10,X'00'),(2,1,20,X'00'),(3,1,9223372036854775807,X'00');",
      nullptr, nullptr, nullptr) == SQLITE_OK);
  sqlite3_close(database);
  rosbags::Reader reader(path.string());
  reader.open();
  const auto maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
  for (int selection = 0; selection != 3; ++selection) {
    rosbags::ReadFilter filter;
    if (selection == 1) filter.topics = {"/sensor"};
    if (selection == 2) filter.connection_ids = {1};
    const auto query = [&] {
      std::vector<std::uint64_t> result;
      reader.read_raw(filter, [&](const auto& message) { result.push_back(message.timestamp); });
      return result;
    };
    filter.start = 15;
    CHECK(query() == std::vector<std::uint64_t>{20, maximum});
    filter.start.reset(); filter.stop = 15;
    CHECK(query() == std::vector<std::uint64_t>{10});
    filter.start = 10; filter.stop = 20;
    CHECK(query() == std::vector<std::uint64_t>{10});
    filter.start = 20; filter.stop = 10;
    CHECK(query().empty());
    filter.start = maximum; filter.stop = maximum + 1;
    CHECK(query() == std::vector<std::uint64_t>{maximum});
    filter.start = maximum + 1;
    CHECK(query().empty());
    filter.start.reset(); filter.stop = std::numeric_limits<std::uint64_t>::max();
    CHECK(query() == std::vector<std::uint64_t>{10,20,maximum});
    filter.stop = 0;
    CHECK(query().empty());
  }
}

TEST_CASE("ROS2 directory validates metadata against every storage connection" * doctest::test_suite("project")) {
  const auto directory = temp_path("_connection_validation");
  std::filesystem::create_directories(directory);
  const auto path = directory / "storage.db3";
  sqlite3* database = nullptr;
  REQUIRE(sqlite3_open(path.c_str(), &database) == SQLITE_OK);
  REQUIRE(sqlite3_exec(database,
      "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT,type TEXT,serialization_format TEXT);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER,timestamp INTEGER,data BLOB);"
      "INSERT INTO topics VALUES(1,'/text','std_msgs/msg/String','cdr');"
      "INSERT INTO messages VALUES(1,1,10,X'000100000100000000');",
      nullptr,nullptr,nullptr) == SQLITE_OK);
  sqlite3_close(database);
  const auto metadata = [&](std::string_view format, std::string_view type) {
    std::ofstream output(directory / "metadata.yaml");
    output << "rosbag2_bagfile_information:\n  version: 9\n  storage_identifier: sqlite3\n"
           << "  relative_file_paths: [storage.db3]\n  message_count: 1\n"
           << "  topics_with_message_count:\n    - topic_metadata:\n        name: /text\n"
           << "        type: " << type << "\n        serialization_format: " << format << "\n"
           << "      message_count: 1\n";
  };
  metadata("ros1", "std_msgs/msg/String");
  rosbags::Reader reader(directory.string());
  CHECK_THROWS_AS(reader.open(), rosbags::FormatError);
  CHECK_FALSE(reader.is_open());
  metadata("cdr", "std_msgs/msg/UInt32");
  CHECK_THROWS_AS(reader.open(), rosbags::FormatError);
  metadata("cdr", "std_msgs/msg/String");
  reader.open();
  rosbags::ReadFilter filter; filter.connection_ids={reader.connections().front().id};
  std::size_t count=0;
  reader.read_raw(filter, [&](const auto& message) {
    CHECK(message.connection->serialization_format == "cdr"); ++count;
  });
  CHECK(count==1);
}

TEST_CASE("ROS1 rejects index entries for a different connection at the same timestamp" * doctest::test_suite("project")) {
  const auto path = temp_path("_wrong_connection.bag");
  const auto time = time_value(1, 0);
  const auto message = record({{"op",u8(2)},{"conn",u32(1)},{"time",time}}, u32(42));
  const auto chunk = record({{"op",u8(5)},{"compression",Bytes{'n','o','n','e'}},
                             {"size",u32(static_cast<std::uint32_t>(message.size()))}}, message);
  const auto index = record({{"op",u8(4)},{"ver",u32(1)},{"conn",u32(0)},{"count",u32(1)}}, [&] {
    auto body=time; put32(body,0); return body;
  }());
  auto bag_header = header({{"op",u8(3)},{"index_pos",u64(4109+chunk.size()+index.size())},
                            {"conn_count",u32(2)},{"chunk_count",u32(1)}});
  put32(bag_header, static_cast<std::uint32_t>(4096-4-bag_header.size()));
  bag_header.resize(4096, ' ');
  std::ofstream output(path, std::ios::binary);
  output << "#ROSBAG V2.0\n";
  const auto write = [&](const Bytes& bytes) {
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  };
  write(bag_header); write(chunk); write(index);
  for (unsigned id=0; id!=2; ++id) {
    const std::string topic=id ? "/other" : "/wanted";
    write(record({{"op",u8(7)},{"conn",u32(id)},{"topic",Bytes(topic.begin(),topic.end())}},
                 serialized_fields({{"type",Bytes{'t','e','s','t','/','M'}}})));
  }
  write(record({{"op",u8(6)},{"ver",u32(1)},{"chunk_pos",u64(4109)},
                {"start_time",time},{"end_time",time},{"count",u32(1)}}, [&] {
    Bytes body; put32(body,0); put32(body,1); return body;
  }()));
  output.close();
  rosbags::Reader reader(path.string()); reader.open();
  rosbags::ReadFilter filter; filter.topics={"/wanted"};
  CHECK_THROWS_AS(reader.read_raw(filter, [](const auto&) {}), rosbags::FormatError);
}

namespace {
std::vector<std::string> temporary_files(std::string_view prefix) {
  std::vector<std::string> paths;
  for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::temp_directory_path())) {
    if (entry.path().filename().string().rfind(prefix,0)==0) paths.push_back(entry.path().string());
  }
  std::sort(paths.begin(),paths.end());
  return paths;
}

Bytes compress_unknown_size(const Byte* data, std::size_t size) {
  std::unique_ptr<ZSTD_CCtx,decltype(&ZSTD_freeCCtx)> ctx(ZSTD_createCCtx(),&ZSTD_freeCCtx);
  REQUIRE(ctx);
  REQUIRE(!ZSTD_isError(ZSTD_CCtx_setParameter(ctx.get(),ZSTD_c_contentSizeFlag,0)));
  Bytes result(ZSTD_compressBound(size));
  const auto count=ZSTD_compress2(ctx.get(),result.data(),result.size(),data,size);
  REQUIRE(!ZSTD_isError(count));
  result.resize(count);
  return result;
}
}

TEST_CASE("streams concatenated zstd storage with bounded output and cleanup" * doctest::test_suite("project")) {
  const auto directory=temp_path("_stream_zstd");
  std::filesystem::create_directories(directory);
  const auto raw=directory/"raw.db3";
  sqlite3* db=nullptr;
  REQUIRE(sqlite3_open(raw.c_str(),&db)==SQLITE_OK);
  REQUIRE(sqlite3_exec(db,
      "CREATE TABLE topics(id INTEGER PRIMARY KEY,name TEXT,type TEXT,serialization_format TEXT);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY,topic_id INTEGER,timestamp INTEGER,data BLOB);"
      "INSERT INTO topics VALUES(1,'/bytes','test/Bytes','cdr');",nullptr,nullptr,nullptr)==SQLITE_OK);
  Bytes payload(512*1024);
  for (std::size_t i=0;i<payload.size();++i) payload[i]=static_cast<Byte>(i*17);
  sqlite3_stmt* insert=nullptr;
  REQUIRE(sqlite3_prepare_v2(db,"INSERT INTO messages VALUES(1,1,42,?)",-1,&insert,nullptr)==SQLITE_OK);
  REQUIRE(sqlite3_bind_blob(insert,1,payload.data(),static_cast<int>(payload.size()),SQLITE_TRANSIENT)==SQLITE_OK);
  REQUIRE(sqlite3_step(insert)==SQLITE_DONE);
  sqlite3_finalize(insert); sqlite3_close(db);
  std::ifstream input(raw,std::ios::binary);
  const Bytes file((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
  input.close();
  auto compressed=compress_unknown_size(file.data(),file.size()/2);
  auto second=compress_unknown_size(file.data()+file.size()/2,file.size()-file.size()/2);
  compressed.insert(compressed.end(),second.begin(),second.end());
  const auto write_compressed=[&] {
    std::ofstream output(directory/"bag.db3.zstd",std::ios::binary);
    output.write(reinterpret_cast<const char*>(compressed.data()),compressed.size());
  };
  const auto write_metadata=[&](bool two_files) {
    std::ofstream output(directory/"metadata.yaml");
    output << "rosbag2_bagfile_information:\n"
           << "  version: 5\n  storage_identifier: sqlite3\n"
           << "  relative_file_paths: [bag.db3.zstd" << (two_files ? ", bag.db3.zstd" : "") << "]\n"
           << "  starting_time: {nanoseconds_since_epoch: 42}\n  duration: {nanoseconds: 0}\n"
           << "  message_count: " << (two_files ? 2 : 1) << "\n"
           << "  compression_format: zstd\n  compression_mode: file\n"
           << "  topics_with_message_count:\n    - topic_metadata:\n"
           << "        name: /bytes\n        type: test/Bytes\n        serialization_format: cdr\n"
           << "      message_count: " << (two_files ? 2 : 1) << "\n";
  };
  write_compressed(); write_metadata(false);
  const auto before=temporary_files("rosbags_cpp_");
  {
    rosbags::ReaderOptions limits;
    limits.max_decompressed_file_bytes=file.size(); limits.max_temporary_bytes=file.size();
    rosbags::Reader reader(directory.string(),limits);
    reader.open();
    CHECK(temporary_files("rosbags_cpp_").size()==before.size()+1);
    std::size_t count=0;
    reader.read_raw({},[&](const auto& message) { CHECK(*message.bytes==payload); CHECK(message.timestamp==42); ++count; });
    CHECK(count==1);
    reader.close();
  }
  CHECK(temporary_files("rosbags_cpp_")==before);
  for (int budget=0;budget!=3;++budget) {
    rosbags::ReaderOptions limits;
    if (budget==0) limits.max_decompressed_file_bytes=file.size()-1;
    if (budget==1) limits.max_temporary_bytes=file.size()-1;
    if (budget==2) limits.max_zstd_window_bytes=1024;
    rosbags::Reader reader(directory.string(),limits);
    CHECK_THROWS_AS(reader.open(),rosbags::ResourceLimitError);
    CHECK_FALSE(reader.is_open());
    CHECK(temporary_files("rosbags_cpp_")==before);
  }
  write_metadata(true);
  rosbags::ReaderOptions aggregate;
  aggregate.max_temporary_bytes=file.size()*2-1;
  rosbags::Reader multiple(directory.string(),aggregate);
  CHECK_THROWS_AS(multiple.open(),rosbags::ResourceLimitError);
  CHECK(temporary_files("rosbags_cpp_")==before);
  write_metadata(false);
  compressed.pop_back(); write_compressed();
  rosbags::Reader truncated(directory.string());
  CHECK_THROWS_AS(truncated.open(),rosbags::FormatError);
  CHECK(temporary_files("rosbags_cpp_")==before);
  compressed.clear(); write_compressed();
  CHECK_THROWS_AS(truncated.open(),rosbags::FormatError);
  CHECK(temporary_files("rosbags_cpp_")==before);
}

#if ROSBAGS_HAS_MCAP
namespace {
void write_mcap_variant(const std::filesystem::path& path,int variant,bool empty=false) {
  mcap::McapWriterOptions options("ros2");
  options.noMessageIndex=variant==1;
  options.noChunking=variant==2;
  options.noStatistics=variant==3;
  options.noSummary=variant==4;
  options.noRepeatedChannels=variant==5;
  options.noRepeatedSchemas=variant==6;
  options.chunkSize=1; // Force multiple chunks and timestamp overlap.
  mcap::McapWriter writer;
  REQUIRE(writer.open(path.string(),options).ok());
  mcap::Schema schema("test/msg/Byte","ros2msg","uint8 data"); writer.addSchema(schema);
  mcap::Channel channel("/sensor","cdr",schema.id); writer.addChannel(channel);
  if (!empty) {
    const std::array<std::uint64_t,4> times{20,10,10,30};
    for (std::size_t i=0;i<times.size();++i) {
      const Byte payload=static_cast<Byte>(i);
      mcap::Message message{}; message.channelId=channel.id;
      message.logTime=message.publishTime=times[i];
      message.data=reinterpret_cast<const std::byte*>(&payload); message.dataSize=1;
      REQUIRE(writer.write(message).ok());
    }
  }
  writer.close();
}
}
TEST_CASE("MCAP variants sort timestamps preserve ties and merge correctly" * doctest::test_suite("project")) {
  for (int variant=0;variant!=7;++variant) {
    CAPTURE(variant);
    const auto path=temp_path("_variant.mcap"); write_mcap_variant(path,variant);
    const auto before=temporary_files("rosbags_mcap_");
    rosbags::Reader reader(path.string()); reader.open();
    CHECK(reader.metadata().message_count==4);
    CHECK(reader.connections().front().type=="test/msg/Byte");
    std::vector<std::uint64_t> times;
    std::vector<Byte> values;
    reader.read_raw({},[&](const auto& message) {
      times.push_back(message.timestamp); values.push_back(message.bytes->at(0));
      CHECK(message.source_path==path.string());
    });
    CHECK(times==std::vector<std::uint64_t>{10,10,20,30});
    CHECK(values==std::vector<Byte>{1,2,0,3});
    rosbags::ReadFilter filter; filter.start=10; filter.stop=20;
    filter.connection_ids={reader.connections().front().id};
    values.clear();
    reader.read_raw(filter,[&](const auto& message) { values.push_back(message.bytes->at(0)); });
    CHECK(values==std::vector<Byte>{1,2});
    filter.topics={"/missing"};
    rosbags::Message message;
    CHECK_FALSE(reader.messages(filter).next(message));
    filter.topics.clear(); filter.start=20; filter.stop=10;
    CHECK_FALSE(reader.messages(filter).next(message));
    reader.close();
    CHECK(temporary_files("rosbags_mcap_")==before);
    const auto empty=temp_path("_empty.mcap"); write_mcap_variant(empty,variant,true);
    rosbags::Reader empty_reader(empty.string()); empty_reader.open();
    CHECK(empty_reader.metadata().message_count==0);
    CHECK_FALSE(empty_reader.messages().next(message));
    const auto other=temp_path("_other.mcap"); write_mcap_variant(other,0);
    rosbags::AnyReader merged({path.string(),other.string()}); merged.open();
    times.clear();
    merged.read_raw({},[&](const auto& msg) { times.push_back(msg.timestamp); });
    CHECK(times==std::vector<std::uint64_t>{10,10,10,10,20,20,30,30});
  }
}
TEST_CASE("MCAP rejects chunk index message and temporary budgets before allocation" * doctest::test_suite("project")) {
  const auto path=temp_path("_budget.mcap"); write_mcap_variant(path,1);
  const auto before=temporary_files("rosbags_mcap_");
  for (int kind=0;kind!=4;++kind) {
    rosbags::ReaderOptions options;
    if (kind==0) options.max_mcap_chunk_bytes=1;
    if (kind==1) options.max_index_bytes=1;
    if (kind==2) options.max_message_bytes=0;
    if (kind==3) options.max_temporary_bytes=3;
    rosbags::Reader reader(path.string(),options);
    CHECK_THROWS_AS(reader.open(),rosbags::ResourceLimitError);
    CHECK_FALSE(reader.is_open());
    CHECK(temporary_files("rosbags_mcap_")==before);
  }
  // Valid indexed files need no payload spool, even when statistics are omitted.
  const auto indexed=temp_path("_indexed.mcap"); write_mcap_variant(indexed,3);
  rosbags::ReaderOptions options; options.max_temporary_bytes=0;
  rosbags::Reader reader(indexed.string(),options); reader.open();
  std::size_t count=0;
  reader.read_raw({},[&](const auto&) { ++count; });
  CHECK(count==4);
  CHECK(temporary_files("rosbags_mcap_")==before);
}
#endif

TEST_CASE("builtins retain ROS1 signed bytes and full time ranges" * doctest::test_suite("project")) {
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry,"ros1_noetic");
  rosbags::profiles::register_builtin_types(registry,"ros2_humble");
  rosbags::Connection connection;
  const auto decode=[&](Bytes payload,bool cdr) {
    connection.serialization_format=cdr ? "cdr" : "ros1";
    rosbags::Message message{std::make_shared<Bytes>(std::move(payload)),0,&connection};
    return rosbags::decode(message,registry,cdr ? "ros2_humble" : "ros1_noetic",rosbags::UnknownTypePolicy::Error);
  };
  connection.type="std_msgs/Byte";
  for (const Byte value : {Byte{128},Byte{255},Byte{0},Byte{127}}) {
    const auto ros1=decode(Bytes{value},false);
    CHECK(ros1->as<rosbags::profiles::std_msgs::Byte>().data==static_cast<std::int8_t>(value));
    const auto ros2=decode(Bytes{0,1,0,0,value},true);
    CHECK(ros2->as<rosbags::profiles::std_msgs::Byte>().data==value);
  }
  connection.type="std_msgs/ByteMultiArray";
  Bytes array; put32(array,0); put32(array,0); put32(array,4); array.insert(array.end(),{128,255,0,127});
  CHECK(decode(array,false)->as<rosbags::profiles::std_msgs::ByteMultiArray>().data==std::vector<std::int16_t>{-128,-1,0,127});
  array.insert(array.begin(),{0,1,0,0});
  CHECK(decode(array,true)->as<rosbags::profiles::std_msgs::ByteMultiArray>().data==std::vector<std::int16_t>{128,255,0,127});
  connection.type="std_msgs/Time";
  Bytes time; put32(time,0xffffffffU); put32(time,42);
  CHECK(decode(time,false)->as<rosbags::profiles::std_msgs::Time>().data.sec==4294967295LL);
  time.insert(time.begin(),{0,1,0,0});
  rosbags::serialization::CdrReader time_reader({time.data(),time.size()});
  rosbags::profiles::Time decoded_time;
  rosbags::profiles::detail::read_cdr(time_reader,decoded_time);
  CHECK(decoded_time.sec==-1);
  connection.type="std_msgs/Duration";
  Bytes duration; put32(duration,0xffffffffU); put32(duration,0xffffffffU);
  CHECK(decode(duration,false)->as<rosbags::profiles::std_msgs::Duration>().data.nanosec==-1);
  duration.insert(duration.begin(),{0,1,0,0});
  rosbags::serialization::CdrReader duration_reader({duration.data(),duration.size()});
  rosbags::profiles::Duration decoded_duration;
  rosbags::profiles::detail::read_cdr(duration_reader,decoded_duration);
  CHECK(decoded_duration.nanosec==4294967295LL);
}

TEST_CASE("builtin sequences and decoded callbacks enforce independent budgets" * doctest::test_suite("project")) {
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry,"ros1_noetic");
  const auto* support=registry.find("ros1_noetic","std_msgs/UInt64MultiArray");
  REQUIRE(support);
  Bytes malformed; put32(malformed,0); put32(malformed,0); put32(malformed,65536);
  malformed.resize(malformed.size()+65536,0); // Count fits byte length but elements do not.
  CHECK_THROWS_AS(support->deserialize_ros1({malformed.data(),malformed.size()}),rosbags::DecodeError);
  Bytes valid; put32(valid,0); put32(valid,0); put32(valid,3); put64(valid,1); put64(valid,2); put64(valid,3);
  rosbags::DecodeLimits limits;
  limits.max_sequence_elements=2;
  CHECK_THROWS_AS(support->deserialize_ros1({valid.data(),valid.size()},limits),rosbags::ResourceLimitError);
  limits={}; limits.max_decoded_bytes=sizeof(rosbags::profiles::std_msgs::UInt64MultiArray)+3*sizeof(std::uint64_t)-1;
  CHECK_THROWS_AS(support->deserialize_ros1({valid.data(),valid.size()},limits),rosbags::ResourceLimitError);
  limits={}; limits.max_decoded_bytes=sizeof(rosbags::profiles::std_msgs::UInt64MultiArray)+3*sizeof(std::uint64_t);
  CHECK_NOTHROW(support->deserialize_ros1({valid.data(),valid.size()},limits));
  const auto path=temp_path("_decode_budget.bag"); write_ros1_bag(path,{{{42,u32(7)}}});
  // A custom limited decoder also receives the limit through Reader and AnyReader.
  auto custom=std::make_shared<const rosbags::TypeSupport<std::uint32_t>>("test/M","test",
      [](rosbags::ByteView bytes,const rosbags::DecodeLimits& budget) {
        rosbags::serialization::Ros1Reader reader(bytes,budget); reader.account_allocation(sizeof(std::uint32_t));
        const auto value=reader.u32(); reader.finish(); return value;
      },[](rosbags::ByteView,const rosbags::DecodeLimits&) { return 0U; });
  registry.register_type(custom);
  limits.max_decoded_bytes=3;
  rosbags::Reader reader(path.string()); reader.open();
  CHECK_THROWS_AS(reader.read_decoded({},registry,"test",[](const auto&,const auto&) {},rosbags::UnknownTypePolicy::Error,{},limits),rosbags::ResourceLimitError);
  rosbags::AnyReader any({path.string()}); any.open();
  CHECK_THROWS_AS(any.read_decoded({},registry,"test",[](const auto&,const auto&) {},rosbags::UnknownTypePolicy::Error,{},limits),rosbags::ResourceLimitError);
}

TEST_CASE("ROS1 lengths and counts fail before untrusted allocations" * doctest::test_suite("project")) {
  const auto path=temp_path("_resource.bag"); write_ros1_bag(path,{{{42,u32(7)}}});
  for (int budget=0;budget!=3;++budget) {
    rosbags::ReaderOptions options;
    if (budget==0) options.max_rosbag1_header_bytes=1;
    if (budget==1) options.max_connection_body_bytes=1;
    if (budget==2) options.max_index_bytes=1;
    rosbags::Reader reader(path.string(),options);
    CHECK_THROWS_AS(reader.open(),rosbags::ResourceLimitError);
    CHECK_FALSE(reader.is_open());
  }
  rosbags::ReaderOptions options; options.max_message_bytes=3;
  rosbags::Reader reader(path.string(),options); reader.open();
  rosbags::Message message;
  CHECK_THROWS_AS(reader.messages().next(message),rosbags::ResourceLimitError);
  reader.close();
  // A tiny file claiming a 4 GiB header must report a format error, not allocate it.
  std::ofstream output(path,std::ios::binary|std::ios::trunc);
  output << "#ROSBAG V2.0\n";
  const auto giant=u32(0xffffffffU);
  output.write(reinterpret_cast<const char*>(giant.data()),giant.size()); output.close();
  options={}; options.max_rosbag1_header_bytes=std::numeric_limits<std::size_t>::max();
  rosbags::Reader malformed(path.string(),options);
  CHECK_THROWS_AS(malformed.open(),rosbags::FormatError);
  CHECK_FALSE(malformed.is_open());
}

TEST_CASE("codegen rejects unsupported or unresolved layouts explicitly" * doctest::test_suite("project")) {
  const auto package=temp_path("_invalid_codegen"); std::filesystem::create_directories(package/"msg");
  rosbags::codegen::GenerateOptions options;
  options.profile="ros2_humble"; options.inputs={package.string()}; options.output_directory=(package/"output").string();
  for (const std::string source : {"struct Invalid { @key uint32 data; };", "module x { struct Invalid { uint32 data; }; };",
                                 "struct Invalid { const uint32 x = 3; uint32 data; };", "struct Invalid { malformed; };"}) {
    std::ofstream file(package/"msg"/"Invalid.idl"); file << source; file.close();
    CHECK_THROWS_AS(rosbags::codegen::generate(options),rosbags::RosbagsError);
  }
  std::filesystem::remove(package/"msg"/"Invalid.idl");
  for (const std::string source : {"Missing data\n", "uint8[<=bad] data\n", "uint8[4]junk data\n", "uint8 data\nuint8 data\n"}) {
    std::ofstream file(package/"msg"/"Invalid.msg"); file << source; file.close();
    CHECK_THROWS_AS(rosbags::codegen::generate(options),rosbags::RosbagsError);
  }
}
