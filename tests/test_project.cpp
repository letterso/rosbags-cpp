#include <doctest/doctest.h>

#include <rosbags/rosbags.hpp>
#include <rosbags/profiles.hpp>
#include <rosbags/codegen.hpp>

#include <sqlite3.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
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
  static std::uint32_t counter = 0;
  return std::filesystem::temp_directory_path() / ("rosbags_cpp_test_" + std::to_string(++counter) + suffix);
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
  reader.close();
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
  reader.read_raw({}, [&](const rosbags::Message& message) {
    CHECK(message.timestamp == 1000000002ULL);
    REQUIRE(message.bytes != nullptr);
    CHECK(message.bytes->size() == 4);
    ++count;
  });
  CHECK(count == 1);
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
