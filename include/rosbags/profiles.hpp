#pragma once

#include "rosbags/rosbags.hpp"

#include <array>
#include <string>

namespace rosbags::profiles {

struct Time {
  std::int32_t sec = 0;
  std::uint32_t nanosec = 0;
};
struct Duration {
  std::int32_t sec = 0;
  std::uint32_t nanosec = 0;
};
struct String {
  std::string data;
};
struct Empty {};
struct Header {
  Time stamp;
  std::string frame_id;
};
struct Vector3 {
  double x = 0;
  double y = 0;
  double z = 0;
};
struct Quaternion {
  double x = 0;
  double y = 0;
  double z = 0;
  double w = 0;
};
struct Imu {
  Header header;
  Quaternion orientation;
  std::array<double, 9> orientation_covariance{};
  Vector3 angular_velocity;
  std::array<double, 9> angular_velocity_covariance{};
  Vector3 linear_acceleration;
  std::array<double, 9> linear_acceleration_covariance{};
};

void register_builtin_types(TypeRegistry& registry, std::string_view profile);
void register_all_builtin_profiles(TypeRegistry& registry);

}  // namespace rosbags::profiles
