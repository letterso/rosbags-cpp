#include "rosbags/profiles.hpp"

#include "rosbags/serialization.hpp"

#include <memory>

namespace rosbags::profiles {
namespace {

using serialization::CdrReader;
using serialization::Ros1Reader;

template <typename T>
void read_time(T& reader, Time& value) { value.sec = reader.i32(); value.nanosec = reader.u32(); }
template <typename T>
void read_time(T& reader, Duration& value) { value.sec = reader.i32(); value.nanosec = reader.u32(); }
template <typename T>
void read_header(T& reader, Header& value) { read_time(reader, value.stamp); value.frame_id = reader.string(); }
void read_header(Ros1Reader& reader, Header& value) {
  (void)reader.u32();  // ROS1 std_msgs/Header includes the legacy sequence number.
  read_time(reader, value.stamp);
  value.frame_id = reader.string();
}
template <typename T>
void read_vector(T& reader, Vector3& value) { value.x = reader.f64(); value.y = reader.f64(); value.z = reader.f64(); }
template <typename T>
void read_quaternion(T& reader, Quaternion& value) { value.x = reader.f64(); value.y = reader.f64(); value.z = reader.f64(); value.w = reader.f64(); }
template <typename T>
void read_imu(T& reader, Imu& value) {
  read_header(reader, value.header);
  read_quaternion(reader, value.orientation);
  for (auto& item : value.orientation_covariance) item = reader.f64();
  read_vector(reader, value.angular_velocity);
  for (auto& item : value.angular_velocity_covariance) item = reader.f64();
  read_vector(reader, value.linear_acceleration);
  for (auto& item : value.linear_acceleration_covariance) item = reader.f64();
}
template <typename T>
String read_string(T& reader) { return String{reader.string()}; }
template <typename T>
Empty read_empty(T&) { return {}; }

template <typename T, typename Decoder>
std::shared_ptr<const TypeSupport<T>> support(std::string name, std::string profile, Decoder decoder) {
  return std::make_shared<TypeSupport<T>>(std::move(name), std::move(profile), decoder, decoder,
                                          [](const T&) { return std::string{}; });
}

}  // namespace

void register_builtin_types(TypeRegistry& registry, std::string_view profile) {
  const auto name = std::string(profile);
  registry.register_type(support<String>("std_msgs/msg/String", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); auto value = read_string(reader); reader.finish(); return value; }));
  registry.register_type(support<Empty>("std_msgs/msg/Empty", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); auto value = read_empty(reader); reader.finish(); return value; }));
  registry.register_type(support<Time>("builtin_interfaces/msg/Time", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Time value; read_time(reader, value); reader.finish(); return value; }));
  registry.register_type(support<Duration>("builtin_interfaces/msg/Duration", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Duration value; read_time(reader, value); reader.finish(); return value; }));
  registry.register_type(support<Vector3>("geometry_msgs/msg/Vector3", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Vector3 value; read_vector(reader, value); reader.finish(); return value; }));
  registry.register_type(support<Quaternion>("geometry_msgs/msg/Quaternion", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Quaternion value; read_quaternion(reader, value); reader.finish(); return value; }));
  registry.register_type(support<Imu>("sensor_msgs/msg/Imu", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Imu value; read_imu(reader, value); reader.finish(); return value; }));

  // Replace the ROS1 decoder with the CDR decoder for ROS2 wire data.
  registry.register_type(std::make_shared<TypeSupport<Header>>("std_msgs/msg/Header", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Header value; read_header(reader, value); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); Header value; read_header(reader, value); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<Time>>("builtin_interfaces/msg/Time", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Time value; read_time(reader, value); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); Time value; read_time(reader, value); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<Duration>>("builtin_interfaces/msg/Duration", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Duration value; read_time(reader, value); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); Duration value; read_time(reader, value); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<String>>("std_msgs/msg/String", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); auto value = read_string(reader); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); auto value = read_string(reader); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<Empty>>("std_msgs/msg/Empty", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); auto value = read_empty(reader); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); auto value = read_empty(reader); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<Vector3>>("geometry_msgs/msg/Vector3", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Vector3 value; read_vector(reader, value); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); Vector3 value; read_vector(reader, value); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<Quaternion>>("geometry_msgs/msg/Quaternion", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Quaternion value; read_quaternion(reader, value); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); Quaternion value; read_quaternion(reader, value); reader.finish(); return value; }));
  registry.register_type(std::make_shared<TypeSupport<Imu>>("sensor_msgs/msg/Imu", name,
      [](ByteView bytes) { Ros1Reader reader(bytes); Imu value; read_imu(reader, value); reader.finish(); return value; },
      [](ByteView bytes) { CdrReader reader(bytes); Imu value; read_imu(reader, value); reader.finish(); return value; }));
}

void register_all_builtin_profiles(TypeRegistry& registry) {
  for (const auto profile : {"ros1_noetic", "ros2_foxy", "ros2_humble", "ros2_jazzy"}) register_builtin_types(registry, profile);
}

}  // namespace rosbags::profiles
