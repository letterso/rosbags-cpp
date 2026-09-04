#include "rosbags/profiles.hpp"
#include <memory>
#include <utility>
namespace rosbags::profiles::detail {
void read_ros1(Ros1Reader& reader, Time& value) { value.sec = reader.i32(); value.nanosec = reader.u32(); }
void read_cdr(CdrReader& reader, Time& value) { value.sec = reader.i32(); value.nanosec = reader.u32(); }
void read_ros1(Ros1Reader& reader, Duration& value) { value.sec = reader.i32(); value.nanosec = reader.u32(); }
void read_cdr(CdrReader& reader, Duration& value) { value.sec = reader.i32(); value.nanosec = reader.u32(); }
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Vector3& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.z = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Vector3& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.z = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Accel& message) {
  read_ros1(reader, message.linear);
  read_ros1(reader, message.angular);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Accel& message) {
  read_cdr(reader, message.linear);
  read_cdr(reader, message.angular);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Header& message) {
  message.seq = reader.u32();
  read_ros1(reader, message.stamp);
  message.frame_id = reader.string();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Header& message) {
  read_cdr(reader, message.stamp);
  message.frame_id = reader.string();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::AccelStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.accel);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::AccelStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.accel);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::AccelWithCovariance& message) {
  read_ros1(reader, message.accel);
  for (auto& value : message.covariance) {
    value = reader.f64();
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::AccelWithCovariance& message) {
  read_cdr(reader, message.accel);
  for (auto& value : message.covariance) {
    value = reader.f64();
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.accel);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.accel);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Inertia& message) {
  message.m = reader.f64();
  read_ros1(reader, message.com);
  message.ixx = reader.f64();
  message.ixy = reader.f64();
  message.ixz = reader.f64();
  message.iyy = reader.f64();
  message.iyz = reader.f64();
  message.izz = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Inertia& message) {
  message.m = reader.f64();
  read_cdr(reader, message.com);
  message.ixx = reader.f64();
  message.ixy = reader.f64();
  message.ixz = reader.f64();
  message.iyy = reader.f64();
  message.iyz = reader.f64();
  message.izz = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::InertiaStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.inertia);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::InertiaStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.inertia);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Point& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.z = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Point& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.z = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Point32& message) {
  message.x = reader.f32();
  message.y = reader.f32();
  message.z = reader.f32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Point32& message) {
  message.x = reader.f32();
  message.y = reader.f32();
  message.z = reader.f32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PointStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.point);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PointStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.point);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Polygon& message) {
  const auto size_points = reader.u32();
  if (size_points > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.points.clear(); message.points.reserve(size_points);
  for (std::uint32_t i = 0; i < size_points; ++i) {
    ::rosbags::profiles::geometry_msgs::Point32 value{}; read_ros1(reader, value); message.points.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Polygon& message) {
  const auto size_points = reader.u32();
  if (size_points > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.points.clear(); message.points.reserve(size_points);
  for (std::uint32_t i = 0; i < size_points; ++i) {
    ::rosbags::profiles::geometry_msgs::Point32 value{}; read_cdr(reader, value); message.points.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PolygonInstance& message) {
  read_ros1(reader, message.polygon);
  message.id = reader.i64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PolygonInstance& message) {
  read_cdr(reader, message.polygon);
  message.id = reader.i64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PolygonInstanceStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.polygon);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PolygonInstanceStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.polygon);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PolygonStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.polygon);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PolygonStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.polygon);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Quaternion& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.z = reader.f64();
  message.w = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Quaternion& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.z = reader.f64();
  message.w = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Pose& message) {
  read_ros1(reader, message.position);
  read_ros1(reader, message.orientation);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Pose& message) {
  read_cdr(reader, message.position);
  read_cdr(reader, message.orientation);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Pose2D& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.theta = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Pose2D& message) {
  message.x = reader.f64();
  message.y = reader.f64();
  message.theta = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PoseArray& message) {
  read_ros1(reader, message.header);
  const auto size_poses = reader.u32();
  if (size_poses > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.poses.clear(); message.poses.reserve(size_poses);
  for (std::uint32_t i = 0; i < size_poses; ++i) {
    ::rosbags::profiles::geometry_msgs::Pose value{}; read_ros1(reader, value); message.poses.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PoseArray& message) {
  read_cdr(reader, message.header);
  const auto size_poses = reader.u32();
  if (size_poses > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.poses.clear(); message.poses.reserve(size_poses);
  for (std::uint32_t i = 0; i < size_poses; ++i) {
    ::rosbags::profiles::geometry_msgs::Pose value{}; read_cdr(reader, value); message.poses.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PoseStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.pose);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PoseStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.pose);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PoseWithCovariance& message) {
  read_ros1(reader, message.pose);
  for (auto& value : message.covariance) {
    value = reader.f64();
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PoseWithCovariance& message) {
  read_cdr(reader, message.pose);
  for (auto& value : message.covariance) {
    value = reader.f64();
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.pose);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.pose);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::QuaternionStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.quaternion);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::QuaternionStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.quaternion);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Transform& message) {
  read_ros1(reader, message.translation);
  read_ros1(reader, message.rotation);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Transform& message) {
  read_cdr(reader, message.translation);
  read_cdr(reader, message.rotation);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::TransformStamped& message) {
  read_ros1(reader, message.header);
  message.child_frame_id = reader.string();
  read_ros1(reader, message.transform);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::TransformStamped& message) {
  read_cdr(reader, message.header);
  message.child_frame_id = reader.string();
  read_cdr(reader, message.transform);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Twist& message) {
  read_ros1(reader, message.linear);
  read_ros1(reader, message.angular);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Twist& message) {
  read_cdr(reader, message.linear);
  read_cdr(reader, message.angular);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::TwistStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.twist);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::TwistStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.twist);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::TwistWithCovariance& message) {
  read_ros1(reader, message.twist);
  for (auto& value : message.covariance) {
    value = reader.f64();
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::TwistWithCovariance& message) {
  read_cdr(reader, message.twist);
  for (auto& value : message.covariance) {
    value = reader.f64();
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.twist);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.twist);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Vector3Stamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.vector);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Vector3Stamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.vector);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::VelocityStamped& message) {
  read_ros1(reader, message.header);
  message.body_frame_id = reader.string();
  message.reference_frame_id = reader.string();
  read_ros1(reader, message.velocity);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::VelocityStamped& message) {
  read_cdr(reader, message.header);
  message.body_frame_id = reader.string();
  message.reference_frame_id = reader.string();
  read_cdr(reader, message.velocity);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::Wrench& message) {
  read_ros1(reader, message.force);
  read_ros1(reader, message.torque);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::Wrench& message) {
  read_cdr(reader, message.force);
  read_cdr(reader, message.torque);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::geometry_msgs::WrenchStamped& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.wrench);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::geometry_msgs::WrenchStamped& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.wrench);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::nav_msgs::GridCells& message) {
  read_ros1(reader, message.header);
  message.cell_width = reader.f32();
  message.cell_height = reader.f32();
  const auto size_cells = reader.u32();
  if (size_cells > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.cells.clear(); message.cells.reserve(size_cells);
  for (std::uint32_t i = 0; i < size_cells; ++i) {
    ::rosbags::profiles::geometry_msgs::Point value{}; read_ros1(reader, value); message.cells.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::nav_msgs::GridCells& message) {
  read_cdr(reader, message.header);
  message.cell_width = reader.f32();
  message.cell_height = reader.f32();
  const auto size_cells = reader.u32();
  if (size_cells > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.cells.clear(); message.cells.reserve(size_cells);
  for (std::uint32_t i = 0; i < size_cells; ++i) {
    ::rosbags::profiles::geometry_msgs::Point value{}; read_cdr(reader, value); message.cells.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::nav_msgs::MapMetaData& message) {
  read_ros1(reader, message.map_load_time);
  message.resolution = reader.f32();
  message.width = reader.u32();
  message.height = reader.u32();
  read_ros1(reader, message.origin);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::nav_msgs::MapMetaData& message) {
  read_cdr(reader, message.map_load_time);
  message.resolution = reader.f32();
  message.width = reader.u32();
  message.height = reader.u32();
  read_cdr(reader, message.origin);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::nav_msgs::OccupancyGrid& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.info);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i8());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::nav_msgs::OccupancyGrid& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.info);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i8());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::nav_msgs::Odometry& message) {
  read_ros1(reader, message.header);
  message.child_frame_id = reader.string();
  read_ros1(reader, message.pose);
  read_ros1(reader, message.twist);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::nav_msgs::Odometry& message) {
  read_cdr(reader, message.header);
  message.child_frame_id = reader.string();
  read_cdr(reader, message.pose);
  read_cdr(reader, message.twist);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::nav_msgs::Path& message) {
  read_ros1(reader, message.header);
  const auto size_poses = reader.u32();
  if (size_poses > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.poses.clear(); message.poses.reserve(size_poses);
  for (std::uint32_t i = 0; i < size_poses; ++i) {
    ::rosbags::profiles::geometry_msgs::PoseStamped value{}; read_ros1(reader, value); message.poses.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::nav_msgs::Path& message) {
  read_cdr(reader, message.header);
  const auto size_poses = reader.u32();
  if (size_poses > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.poses.clear(); message.poses.reserve(size_poses);
  for (std::uint32_t i = 0; i < size_poses; ++i) {
    ::rosbags::profiles::geometry_msgs::PoseStamped value{}; read_cdr(reader, value); message.poses.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::BatteryState& message) {
  read_ros1(reader, message.header);
  message.voltage = reader.f32();
  message.temperature = reader.f32();
  message.current = reader.f32();
  message.charge = reader.f32();
  message.capacity = reader.f32();
  message.design_capacity = reader.f32();
  message.percentage = reader.f32();
  message.power_supply_status = reader.u8();
  message.power_supply_health = reader.u8();
  message.power_supply_technology = reader.u8();
  message.present = reader.boolean();
  const auto size_cell_voltage = reader.u32();
  if (size_cell_voltage > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.cell_voltage.clear(); message.cell_voltage.reserve(size_cell_voltage);
  for (std::uint32_t i = 0; i < size_cell_voltage; ++i) {
    message.cell_voltage.push_back(reader.f32());
  }
  const auto size_cell_temperature = reader.u32();
  if (size_cell_temperature > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.cell_temperature.clear(); message.cell_temperature.reserve(size_cell_temperature);
  for (std::uint32_t i = 0; i < size_cell_temperature; ++i) {
    message.cell_temperature.push_back(reader.f32());
  }
  message.location = reader.string();
  message.serial_number = reader.string();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::BatteryState& message) {
  read_cdr(reader, message.header);
  message.voltage = reader.f32();
  message.temperature = reader.f32();
  message.current = reader.f32();
  message.charge = reader.f32();
  message.capacity = reader.f32();
  message.design_capacity = reader.f32();
  message.percentage = reader.f32();
  message.power_supply_status = reader.u8();
  message.power_supply_health = reader.u8();
  message.power_supply_technology = reader.u8();
  message.present = reader.boolean();
  const auto size_cell_voltage = reader.u32();
  if (size_cell_voltage > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.cell_voltage.clear(); message.cell_voltage.reserve(size_cell_voltage);
  for (std::uint32_t i = 0; i < size_cell_voltage; ++i) {
    message.cell_voltage.push_back(reader.f32());
  }
  const auto size_cell_temperature = reader.u32();
  if (size_cell_temperature > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.cell_temperature.clear(); message.cell_temperature.reserve(size_cell_temperature);
  for (std::uint32_t i = 0; i < size_cell_temperature; ++i) {
    message.cell_temperature.push_back(reader.f32());
  }
  message.location = reader.string();
  message.serial_number = reader.string();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::RegionOfInterest& message) {
  message.x_offset = reader.u32();
  message.y_offset = reader.u32();
  message.height = reader.u32();
  message.width = reader.u32();
  message.do_rectify = reader.boolean();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::RegionOfInterest& message) {
  message.x_offset = reader.u32();
  message.y_offset = reader.u32();
  message.height = reader.u32();
  message.width = reader.u32();
  message.do_rectify = reader.boolean();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::CameraInfo& message) {
  read_ros1(reader, message.header);
  message.height = reader.u32();
  message.width = reader.u32();
  message.distortion_model = reader.string();
  const auto size_D = reader.u32();
  if (size_D > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.D.clear(); message.D.reserve(size_D);
  for (std::uint32_t i = 0; i < size_D; ++i) {
    message.D.push_back(reader.f64());
  }
  for (auto& value : message.K) {
    value = reader.f64();
  }
  for (auto& value : message.R) {
    value = reader.f64();
  }
  for (auto& value : message.P) {
    value = reader.f64();
  }
  message.binning_x = reader.u32();
  message.binning_y = reader.u32();
  read_ros1(reader, message.roi);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::CameraInfo& message) {
  read_cdr(reader, message.header);
  message.height = reader.u32();
  message.width = reader.u32();
  message.distortion_model = reader.string();
  const auto size_d = reader.u32();
  if (size_d > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.d.clear(); message.d.reserve(size_d);
  for (std::uint32_t i = 0; i < size_d; ++i) {
    message.d.push_back(reader.f64());
  }
  for (auto& value : message.k) {
    value = reader.f64();
  }
  for (auto& value : message.r) {
    value = reader.f64();
  }
  for (auto& value : message.p) {
    value = reader.f64();
  }
  message.binning_x = reader.u32();
  message.binning_y = reader.u32();
  read_cdr(reader, message.roi);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::ChannelFloat32& message) {
  message.name = reader.string();
  const auto size_values = reader.u32();
  if (size_values > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.values.clear(); message.values.reserve(size_values);
  for (std::uint32_t i = 0; i < size_values; ++i) {
    message.values.push_back(reader.f32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::ChannelFloat32& message) {
  message.name = reader.string();
  const auto size_values = reader.u32();
  if (size_values > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.values.clear(); message.values.reserve(size_values);
  for (std::uint32_t i = 0; i < size_values; ++i) {
    message.values.push_back(reader.f32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::CompressedImage& message) {
  read_ros1(reader, message.header);
  message.format = reader.string();
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::CompressedImage& message) {
  read_cdr(reader, message.header);
  message.format = reader.string();
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::FluidPressure& message) {
  read_ros1(reader, message.header);
  message.fluid_pressure = reader.f64();
  message.variance = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::FluidPressure& message) {
  read_cdr(reader, message.header);
  message.fluid_pressure = reader.f64();
  message.variance = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::Illuminance& message) {
  read_ros1(reader, message.header);
  message.illuminance = reader.f64();
  message.variance = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::Illuminance& message) {
  read_cdr(reader, message.header);
  message.illuminance = reader.f64();
  message.variance = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::Image& message) {
  read_ros1(reader, message.header);
  message.height = reader.u32();
  message.width = reader.u32();
  message.encoding = reader.string();
  message.is_bigendian = reader.u8();
  message.step = reader.u32();
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::Image& message) {
  read_cdr(reader, message.header);
  message.height = reader.u32();
  message.width = reader.u32();
  message.encoding = reader.string();
  message.is_bigendian = reader.u8();
  message.step = reader.u32();
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::Imu& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.orientation);
  for (auto& value : message.orientation_covariance) {
    value = reader.f64();
  }
  read_ros1(reader, message.angular_velocity);
  for (auto& value : message.angular_velocity_covariance) {
    value = reader.f64();
  }
  read_ros1(reader, message.linear_acceleration);
  for (auto& value : message.linear_acceleration_covariance) {
    value = reader.f64();
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::Imu& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.orientation);
  for (auto& value : message.orientation_covariance) {
    value = reader.f64();
  }
  read_cdr(reader, message.angular_velocity);
  for (auto& value : message.angular_velocity_covariance) {
    value = reader.f64();
  }
  read_cdr(reader, message.linear_acceleration);
  for (auto& value : message.linear_acceleration_covariance) {
    value = reader.f64();
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::JointState& message) {
  read_ros1(reader, message.header);
  const auto size_name = reader.u32();
  if (size_name > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.name.clear(); message.name.reserve(size_name);
  for (std::uint32_t i = 0; i < size_name; ++i) {
    message.name.push_back(reader.string());
  }
  const auto size_position = reader.u32();
  if (size_position > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.position.clear(); message.position.reserve(size_position);
  for (std::uint32_t i = 0; i < size_position; ++i) {
    message.position.push_back(reader.f64());
  }
  const auto size_velocity = reader.u32();
  if (size_velocity > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.velocity.clear(); message.velocity.reserve(size_velocity);
  for (std::uint32_t i = 0; i < size_velocity; ++i) {
    message.velocity.push_back(reader.f64());
  }
  const auto size_effort = reader.u32();
  if (size_effort > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.effort.clear(); message.effort.reserve(size_effort);
  for (std::uint32_t i = 0; i < size_effort; ++i) {
    message.effort.push_back(reader.f64());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::JointState& message) {
  read_cdr(reader, message.header);
  const auto size_name = reader.u32();
  if (size_name > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.name.clear(); message.name.reserve(size_name);
  for (std::uint32_t i = 0; i < size_name; ++i) {
    message.name.push_back(reader.string());
  }
  const auto size_position = reader.u32();
  if (size_position > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.position.clear(); message.position.reserve(size_position);
  for (std::uint32_t i = 0; i < size_position; ++i) {
    message.position.push_back(reader.f64());
  }
  const auto size_velocity = reader.u32();
  if (size_velocity > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.velocity.clear(); message.velocity.reserve(size_velocity);
  for (std::uint32_t i = 0; i < size_velocity; ++i) {
    message.velocity.push_back(reader.f64());
  }
  const auto size_effort = reader.u32();
  if (size_effort > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.effort.clear(); message.effort.reserve(size_effort);
  for (std::uint32_t i = 0; i < size_effort; ++i) {
    message.effort.push_back(reader.f64());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::Joy& message) {
  read_ros1(reader, message.header);
  const auto size_axes = reader.u32();
  if (size_axes > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.axes.clear(); message.axes.reserve(size_axes);
  for (std::uint32_t i = 0; i < size_axes; ++i) {
    message.axes.push_back(reader.f32());
  }
  const auto size_buttons = reader.u32();
  if (size_buttons > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.buttons.clear(); message.buttons.reserve(size_buttons);
  for (std::uint32_t i = 0; i < size_buttons; ++i) {
    message.buttons.push_back(reader.i32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::Joy& message) {
  read_cdr(reader, message.header);
  const auto size_axes = reader.u32();
  if (size_axes > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.axes.clear(); message.axes.reserve(size_axes);
  for (std::uint32_t i = 0; i < size_axes; ++i) {
    message.axes.push_back(reader.f32());
  }
  const auto size_buttons = reader.u32();
  if (size_buttons > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.buttons.clear(); message.buttons.reserve(size_buttons);
  for (std::uint32_t i = 0; i < size_buttons; ++i) {
    message.buttons.push_back(reader.i32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::JoyFeedback& message) {
  message.type = reader.u8();
  message.id = reader.u8();
  message.intensity = reader.f32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::JoyFeedback& message) {
  message.type = reader.u8();
  message.id = reader.u8();
  message.intensity = reader.f32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::JoyFeedbackArray& message) {
  const auto size_array = reader.u32();
  if (size_array > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.array.clear(); message.array.reserve(size_array);
  for (std::uint32_t i = 0; i < size_array; ++i) {
    ::rosbags::profiles::sensor_msgs::JoyFeedback value{}; read_ros1(reader, value); message.array.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::JoyFeedbackArray& message) {
  const auto size_array = reader.u32();
  if (size_array > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.array.clear(); message.array.reserve(size_array);
  for (std::uint32_t i = 0; i < size_array; ++i) {
    ::rosbags::profiles::sensor_msgs::JoyFeedback value{}; read_cdr(reader, value); message.array.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::LaserEcho& message) {
  const auto size_echoes = reader.u32();
  if (size_echoes > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.echoes.clear(); message.echoes.reserve(size_echoes);
  for (std::uint32_t i = 0; i < size_echoes; ++i) {
    message.echoes.push_back(reader.f32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::LaserEcho& message) {
  const auto size_echoes = reader.u32();
  if (size_echoes > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.echoes.clear(); message.echoes.reserve(size_echoes);
  for (std::uint32_t i = 0; i < size_echoes; ++i) {
    message.echoes.push_back(reader.f32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::LaserScan& message) {
  read_ros1(reader, message.header);
  message.angle_min = reader.f32();
  message.angle_max = reader.f32();
  message.angle_increment = reader.f32();
  message.time_increment = reader.f32();
  message.scan_time = reader.f32();
  message.range_min = reader.f32();
  message.range_max = reader.f32();
  const auto size_ranges = reader.u32();
  if (size_ranges > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.ranges.clear(); message.ranges.reserve(size_ranges);
  for (std::uint32_t i = 0; i < size_ranges; ++i) {
    message.ranges.push_back(reader.f32());
  }
  const auto size_intensities = reader.u32();
  if (size_intensities > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.intensities.clear(); message.intensities.reserve(size_intensities);
  for (std::uint32_t i = 0; i < size_intensities; ++i) {
    message.intensities.push_back(reader.f32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::LaserScan& message) {
  read_cdr(reader, message.header);
  message.angle_min = reader.f32();
  message.angle_max = reader.f32();
  message.angle_increment = reader.f32();
  message.time_increment = reader.f32();
  message.scan_time = reader.f32();
  message.range_min = reader.f32();
  message.range_max = reader.f32();
  const auto size_ranges = reader.u32();
  if (size_ranges > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.ranges.clear(); message.ranges.reserve(size_ranges);
  for (std::uint32_t i = 0; i < size_ranges; ++i) {
    message.ranges.push_back(reader.f32());
  }
  const auto size_intensities = reader.u32();
  if (size_intensities > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.intensities.clear(); message.intensities.reserve(size_intensities);
  for (std::uint32_t i = 0; i < size_intensities; ++i) {
    message.intensities.push_back(reader.f32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::MagneticField& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.magnetic_field);
  for (auto& value : message.magnetic_field_covariance) {
    value = reader.f64();
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::MagneticField& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.magnetic_field);
  for (auto& value : message.magnetic_field_covariance) {
    value = reader.f64();
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::MultiDOFJointState& message) {
  read_ros1(reader, message.header);
  const auto size_joint_names = reader.u32();
  if (size_joint_names > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.joint_names.clear(); message.joint_names.reserve(size_joint_names);
  for (std::uint32_t i = 0; i < size_joint_names; ++i) {
    message.joint_names.push_back(reader.string());
  }
  const auto size_transforms = reader.u32();
  if (size_transforms > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.transforms.clear(); message.transforms.reserve(size_transforms);
  for (std::uint32_t i = 0; i < size_transforms; ++i) {
    ::rosbags::profiles::geometry_msgs::Transform value{}; read_ros1(reader, value); message.transforms.push_back(std::move(value));
  }
  const auto size_twist = reader.u32();
  if (size_twist > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.twist.clear(); message.twist.reserve(size_twist);
  for (std::uint32_t i = 0; i < size_twist; ++i) {
    ::rosbags::profiles::geometry_msgs::Twist value{}; read_ros1(reader, value); message.twist.push_back(std::move(value));
  }
  const auto size_wrench = reader.u32();
  if (size_wrench > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.wrench.clear(); message.wrench.reserve(size_wrench);
  for (std::uint32_t i = 0; i < size_wrench; ++i) {
    ::rosbags::profiles::geometry_msgs::Wrench value{}; read_ros1(reader, value); message.wrench.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::MultiDOFJointState& message) {
  read_cdr(reader, message.header);
  const auto size_joint_names = reader.u32();
  if (size_joint_names > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.joint_names.clear(); message.joint_names.reserve(size_joint_names);
  for (std::uint32_t i = 0; i < size_joint_names; ++i) {
    message.joint_names.push_back(reader.string());
  }
  const auto size_transforms = reader.u32();
  if (size_transforms > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.transforms.clear(); message.transforms.reserve(size_transforms);
  for (std::uint32_t i = 0; i < size_transforms; ++i) {
    ::rosbags::profiles::geometry_msgs::Transform value{}; read_cdr(reader, value); message.transforms.push_back(std::move(value));
  }
  const auto size_twist = reader.u32();
  if (size_twist > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.twist.clear(); message.twist.reserve(size_twist);
  for (std::uint32_t i = 0; i < size_twist; ++i) {
    ::rosbags::profiles::geometry_msgs::Twist value{}; read_cdr(reader, value); message.twist.push_back(std::move(value));
  }
  const auto size_wrench = reader.u32();
  if (size_wrench > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.wrench.clear(); message.wrench.reserve(size_wrench);
  for (std::uint32_t i = 0; i < size_wrench; ++i) {
    ::rosbags::profiles::geometry_msgs::Wrench value{}; read_cdr(reader, value); message.wrench.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::MultiEchoLaserScan& message) {
  read_ros1(reader, message.header);
  message.angle_min = reader.f32();
  message.angle_max = reader.f32();
  message.angle_increment = reader.f32();
  message.time_increment = reader.f32();
  message.scan_time = reader.f32();
  message.range_min = reader.f32();
  message.range_max = reader.f32();
  const auto size_ranges = reader.u32();
  if (size_ranges > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.ranges.clear(); message.ranges.reserve(size_ranges);
  for (std::uint32_t i = 0; i < size_ranges; ++i) {
    ::rosbags::profiles::sensor_msgs::LaserEcho value{}; read_ros1(reader, value); message.ranges.push_back(std::move(value));
  }
  const auto size_intensities = reader.u32();
  if (size_intensities > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.intensities.clear(); message.intensities.reserve(size_intensities);
  for (std::uint32_t i = 0; i < size_intensities; ++i) {
    ::rosbags::profiles::sensor_msgs::LaserEcho value{}; read_ros1(reader, value); message.intensities.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::MultiEchoLaserScan& message) {
  read_cdr(reader, message.header);
  message.angle_min = reader.f32();
  message.angle_max = reader.f32();
  message.angle_increment = reader.f32();
  message.time_increment = reader.f32();
  message.scan_time = reader.f32();
  message.range_min = reader.f32();
  message.range_max = reader.f32();
  const auto size_ranges = reader.u32();
  if (size_ranges > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.ranges.clear(); message.ranges.reserve(size_ranges);
  for (std::uint32_t i = 0; i < size_ranges; ++i) {
    ::rosbags::profiles::sensor_msgs::LaserEcho value{}; read_cdr(reader, value); message.ranges.push_back(std::move(value));
  }
  const auto size_intensities = reader.u32();
  if (size_intensities > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.intensities.clear(); message.intensities.reserve(size_intensities);
  for (std::uint32_t i = 0; i < size_intensities; ++i) {
    ::rosbags::profiles::sensor_msgs::LaserEcho value{}; read_cdr(reader, value); message.intensities.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::NavSatStatus& message) {
  message.status = reader.i8();
  message.service = reader.u16();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::NavSatStatus& message) {
  message.status = reader.i8();
  message.service = reader.u16();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::NavSatFix& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.status);
  message.latitude = reader.f64();
  message.longitude = reader.f64();
  message.altitude = reader.f64();
  for (auto& value : message.position_covariance) {
    value = reader.f64();
  }
  message.position_covariance_type = reader.u8();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::NavSatFix& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.status);
  message.latitude = reader.f64();
  message.longitude = reader.f64();
  message.altitude = reader.f64();
  for (auto& value : message.position_covariance) {
    value = reader.f64();
  }
  message.position_covariance_type = reader.u8();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::PointCloud& message) {
  read_ros1(reader, message.header);
  const auto size_points = reader.u32();
  if (size_points > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.points.clear(); message.points.reserve(size_points);
  for (std::uint32_t i = 0; i < size_points; ++i) {
    ::rosbags::profiles::geometry_msgs::Point32 value{}; read_ros1(reader, value); message.points.push_back(std::move(value));
  }
  const auto size_channels = reader.u32();
  if (size_channels > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.channels.clear(); message.channels.reserve(size_channels);
  for (std::uint32_t i = 0; i < size_channels; ++i) {
    ::rosbags::profiles::sensor_msgs::ChannelFloat32 value{}; read_ros1(reader, value); message.channels.push_back(std::move(value));
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::PointCloud& message) {
  read_cdr(reader, message.header);
  const auto size_points = reader.u32();
  if (size_points > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.points.clear(); message.points.reserve(size_points);
  for (std::uint32_t i = 0; i < size_points; ++i) {
    ::rosbags::profiles::geometry_msgs::Point32 value{}; read_cdr(reader, value); message.points.push_back(std::move(value));
  }
  const auto size_channels = reader.u32();
  if (size_channels > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.channels.clear(); message.channels.reserve(size_channels);
  for (std::uint32_t i = 0; i < size_channels; ++i) {
    ::rosbags::profiles::sensor_msgs::ChannelFloat32 value{}; read_cdr(reader, value); message.channels.push_back(std::move(value));
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::PointField& message) {
  message.name = reader.string();
  message.offset = reader.u32();
  message.datatype = reader.u8();
  message.count = reader.u32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::PointField& message) {
  message.name = reader.string();
  message.offset = reader.u32();
  message.datatype = reader.u8();
  message.count = reader.u32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::PointCloud2& message) {
  read_ros1(reader, message.header);
  message.height = reader.u32();
  message.width = reader.u32();
  const auto size_fields = reader.u32();
  if (size_fields > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.fields.clear(); message.fields.reserve(size_fields);
  for (std::uint32_t i = 0; i < size_fields; ++i) {
    ::rosbags::profiles::sensor_msgs::PointField value{}; read_ros1(reader, value); message.fields.push_back(std::move(value));
  }
  message.is_bigendian = reader.boolean();
  message.point_step = reader.u32();
  message.row_step = reader.u32();
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
  message.is_dense = reader.boolean();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::PointCloud2& message) {
  read_cdr(reader, message.header);
  message.height = reader.u32();
  message.width = reader.u32();
  const auto size_fields = reader.u32();
  if (size_fields > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.fields.clear(); message.fields.reserve(size_fields);
  for (std::uint32_t i = 0; i < size_fields; ++i) {
    ::rosbags::profiles::sensor_msgs::PointField value{}; read_cdr(reader, value); message.fields.push_back(std::move(value));
  }
  message.is_bigendian = reader.boolean();
  message.point_step = reader.u32();
  message.row_step = reader.u32();
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
  message.is_dense = reader.boolean();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::Range& message) {
  read_ros1(reader, message.header);
  message.radiation_type = reader.u8();
  message.field_of_view = reader.f32();
  message.min_range = reader.f32();
  message.max_range = reader.f32();
  message.range = reader.f32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::Range& message) {
  read_cdr(reader, message.header);
  message.radiation_type = reader.u8();
  message.field_of_view = reader.f32();
  message.min_range = reader.f32();
  message.max_range = reader.f32();
  message.range = reader.f32();
  message.variance = reader.f32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::RelativeHumidity& message) {
  read_ros1(reader, message.header);
  message.relative_humidity = reader.f64();
  message.variance = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::RelativeHumidity& message) {
  read_cdr(reader, message.header);
  message.relative_humidity = reader.f64();
  message.variance = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::Temperature& message) {
  read_ros1(reader, message.header);
  message.temperature = reader.f64();
  message.variance = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::Temperature& message) {
  read_cdr(reader, message.header);
  message.temperature = reader.f64();
  message.variance = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::sensor_msgs::TimeReference& message) {
  read_ros1(reader, message.header);
  read_ros1(reader, message.time_ref);
  message.source = reader.string();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::sensor_msgs::TimeReference& message) {
  read_cdr(reader, message.header);
  read_cdr(reader, message.time_ref);
  message.source = reader.string();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Bool& message) {
  message.data = reader.boolean();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Bool& message) {
  message.data = reader.boolean();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Byte& message) {
  message.data = reader.u8();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Byte& message) {
  message.data = reader.u8();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::MultiArrayDimension& message) {
  message.label = reader.string();
  message.size = reader.u32();
  message.stride = reader.u32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::MultiArrayDimension& message) {
  message.label = reader.string();
  message.size = reader.u32();
  message.stride = reader.u32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::MultiArrayLayout& message) {
  const auto size_dim = reader.u32();
  if (size_dim > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.dim.clear(); message.dim.reserve(size_dim);
  for (std::uint32_t i = 0; i < size_dim; ++i) {
    ::rosbags::profiles::std_msgs::MultiArrayDimension value{}; read_ros1(reader, value); message.dim.push_back(std::move(value));
  }
  message.data_offset = reader.u32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::MultiArrayLayout& message) {
  const auto size_dim = reader.u32();
  if (size_dim > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.dim.clear(); message.dim.reserve(size_dim);
  for (std::uint32_t i = 0; i < size_dim; ++i) {
    ::rosbags::profiles::std_msgs::MultiArrayDimension value{}; read_cdr(reader, value); message.dim.push_back(std::move(value));
  }
  message.data_offset = reader.u32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::ByteMultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::ByteMultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Char& message) {
  message.data = reader.u8();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Char& message) {
  message.data = reader.u8();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::ColorRGBA& message) {
  message.r = reader.f32();
  message.g = reader.f32();
  message.b = reader.f32();
  message.a = reader.f32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::ColorRGBA& message) {
  message.r = reader.f32();
  message.g = reader.f32();
  message.b = reader.f32();
  message.a = reader.f32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Duration& message) {
  read_ros1(reader, message.data);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Duration& message) {
  read_cdr(reader, message.data);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Empty& message) {
  (void)reader;
  (void)message;
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Empty& message) {
  (void)reader;
  (void)message;
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Float32& message) {
  message.data = reader.f32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Float32& message) {
  message.data = reader.f32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Float32MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.f32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Float32MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.f32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Float64& message) {
  message.data = reader.f64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Float64& message) {
  message.data = reader.f64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Float64MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.f64());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Float64MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.f64());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int16& message) {
  message.data = reader.i16();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int16& message) {
  message.data = reader.i16();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int16MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i16());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int16MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i16());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int32& message) {
  message.data = reader.i32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int32& message) {
  message.data = reader.i32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int32MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int32MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int64& message) {
  message.data = reader.i64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int64& message) {
  message.data = reader.i64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int64MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i64());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int64MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i64());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int8& message) {
  message.data = reader.i8();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int8& message) {
  message.data = reader.i8();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Int8MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i8());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Int8MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.i8());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::String& message) {
  message.data = reader.string();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::String& message) {
  message.data = reader.string();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::Time& message) {
  read_ros1(reader, message.data);
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::Time& message) {
  read_cdr(reader, message.data);
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt16& message) {
  message.data = reader.u16();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt16& message) {
  message.data = reader.u16();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt16MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u16());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt16MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u16());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt32& message) {
  message.data = reader.u32();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt32& message) {
  message.data = reader.u32();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt32MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u32());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt32MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u32());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt64& message) {
  message.data = reader.u64();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt64& message) {
  message.data = reader.u64();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt64MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u64());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt64MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u64());
  }
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt8& message) {
  message.data = reader.u8();
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt8& message) {
  message.data = reader.u8();
}
void read_ros1(Ros1Reader& reader, ::rosbags::profiles::std_msgs::UInt8MultiArray& message) {
  read_ros1(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
void read_cdr(CdrReader& reader, ::rosbags::profiles::std_msgs::UInt8MultiArray& message) {
  read_cdr(reader, message.layout);
  const auto size_data = reader.u32();
  if (size_data > reader.remaining()) throw DecodeError("sequence length exceeds serialized payload");
  message.data.clear(); message.data.reserve(size_data);
  for (std::uint32_t i = 0; i < size_data; ++i) {
    message.data.push_back(reader.u8());
  }
}
}  // namespace rosbags::profiles::detail
namespace rosbags::profiles {
namespace {
template <typename T>
std::shared_ptr<const TypeSupport<T>> make_support(std::string name, std::string profile, void (*ros1)(serialization::Ros1Reader&, T&), void (*cdr)(serialization::CdrReader&, T&)) {
  return std::make_shared<TypeSupport<T>>(std::move(name), std::move(profile),
      [ros1](ByteView bytes) { serialization::Ros1Reader reader(bytes); T value{}; ros1(reader, value); reader.finish(); return value; },
      [cdr](ByteView bytes) { serialization::CdrReader reader(bytes); T value{}; cdr(reader, value); reader.finish(); return value; });
}
}  // namespace
void register_builtin_types(TypeRegistry& registry, std::string_view profile) {
  const auto name = std::string(profile);
  const auto is_unknown_profile = profile != "ros1_noetic" && profile != "ros2_foxy" &&
                                  profile != "ros2_humble" && profile != "ros2_jazzy";
  registry.register_type(make_support<geometry_msgs::Vector3>("geometry_msgs/msg/Vector3", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Accel>("geometry_msgs/msg/Accel", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Header>("std_msgs/msg/Header", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::AccelStamped>("geometry_msgs/msg/AccelStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::AccelWithCovariance>("geometry_msgs/msg/AccelWithCovariance", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::AccelWithCovarianceStamped>("geometry_msgs/msg/AccelWithCovarianceStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Inertia>("geometry_msgs/msg/Inertia", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::InertiaStamped>("geometry_msgs/msg/InertiaStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Point>("geometry_msgs/msg/Point", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Point32>("geometry_msgs/msg/Point32", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::PointStamped>("geometry_msgs/msg/PointStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Polygon>("geometry_msgs/msg/Polygon", name, &detail::read_ros1, &detail::read_cdr));
  if (profile == "ros2_humble" || profile == "ros2_jazzy" || is_unknown_profile) {
  registry.register_type(make_support<geometry_msgs::PolygonInstance>("geometry_msgs/msg/PolygonInstance", name, &detail::read_ros1, &detail::read_cdr));
  }
  if (profile == "ros2_humble" || profile == "ros2_jazzy" || is_unknown_profile) {
  registry.register_type(make_support<geometry_msgs::PolygonInstanceStamped>("geometry_msgs/msg/PolygonInstanceStamped", name, &detail::read_ros1, &detail::read_cdr));
  }
  registry.register_type(make_support<geometry_msgs::PolygonStamped>("geometry_msgs/msg/PolygonStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Quaternion>("geometry_msgs/msg/Quaternion", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Pose>("geometry_msgs/msg/Pose", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Pose2D>("geometry_msgs/msg/Pose2D", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::PoseArray>("geometry_msgs/msg/PoseArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::PoseStamped>("geometry_msgs/msg/PoseStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::PoseWithCovariance>("geometry_msgs/msg/PoseWithCovariance", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::PoseWithCovarianceStamped>("geometry_msgs/msg/PoseWithCovarianceStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::QuaternionStamped>("geometry_msgs/msg/QuaternionStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Transform>("geometry_msgs/msg/Transform", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::TransformStamped>("geometry_msgs/msg/TransformStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Twist>("geometry_msgs/msg/Twist", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::TwistStamped>("geometry_msgs/msg/TwistStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::TwistWithCovariance>("geometry_msgs/msg/TwistWithCovariance", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::TwistWithCovarianceStamped>("geometry_msgs/msg/TwistWithCovarianceStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::Vector3Stamped>("geometry_msgs/msg/Vector3Stamped", name, &detail::read_ros1, &detail::read_cdr));
  if (profile == "ros2_humble" || profile == "ros2_jazzy" || is_unknown_profile) {
  registry.register_type(make_support<geometry_msgs::VelocityStamped>("geometry_msgs/msg/VelocityStamped", name, &detail::read_ros1, &detail::read_cdr));
  }
  registry.register_type(make_support<geometry_msgs::Wrench>("geometry_msgs/msg/Wrench", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<geometry_msgs::WrenchStamped>("geometry_msgs/msg/WrenchStamped", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<nav_msgs::GridCells>("nav_msgs/msg/GridCells", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<nav_msgs::MapMetaData>("nav_msgs/msg/MapMetaData", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<nav_msgs::OccupancyGrid>("nav_msgs/msg/OccupancyGrid", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<nav_msgs::Odometry>("nav_msgs/msg/Odometry", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<nav_msgs::Path>("nav_msgs/msg/Path", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::BatteryState>("sensor_msgs/msg/BatteryState", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::RegionOfInterest>("sensor_msgs/msg/RegionOfInterest", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::CameraInfo>("sensor_msgs/msg/CameraInfo", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::ChannelFloat32>("sensor_msgs/msg/ChannelFloat32", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::CompressedImage>("sensor_msgs/msg/CompressedImage", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::FluidPressure>("sensor_msgs/msg/FluidPressure", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::Illuminance>("sensor_msgs/msg/Illuminance", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::Image>("sensor_msgs/msg/Image", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::Imu>("sensor_msgs/msg/Imu", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::JointState>("sensor_msgs/msg/JointState", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::Joy>("sensor_msgs/msg/Joy", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::JoyFeedback>("sensor_msgs/msg/JoyFeedback", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::JoyFeedbackArray>("sensor_msgs/msg/JoyFeedbackArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::LaserEcho>("sensor_msgs/msg/LaserEcho", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::LaserScan>("sensor_msgs/msg/LaserScan", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::MagneticField>("sensor_msgs/msg/MagneticField", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::MultiDOFJointState>("sensor_msgs/msg/MultiDOFJointState", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::MultiEchoLaserScan>("sensor_msgs/msg/MultiEchoLaserScan", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::NavSatStatus>("sensor_msgs/msg/NavSatStatus", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::NavSatFix>("sensor_msgs/msg/NavSatFix", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::PointCloud>("sensor_msgs/msg/PointCloud", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::PointField>("sensor_msgs/msg/PointField", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::PointCloud2>("sensor_msgs/msg/PointCloud2", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::Range>("sensor_msgs/msg/Range", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::RelativeHumidity>("sensor_msgs/msg/RelativeHumidity", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::Temperature>("sensor_msgs/msg/Temperature", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<sensor_msgs::TimeReference>("sensor_msgs/msg/TimeReference", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Bool>("std_msgs/msg/Bool", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Byte>("std_msgs/msg/Byte", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::MultiArrayDimension>("std_msgs/msg/MultiArrayDimension", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::MultiArrayLayout>("std_msgs/msg/MultiArrayLayout", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::ByteMultiArray>("std_msgs/msg/ByteMultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Char>("std_msgs/msg/Char", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::ColorRGBA>("std_msgs/msg/ColorRGBA", name, &detail::read_ros1, &detail::read_cdr));
  if (profile == "ros1_noetic" || is_unknown_profile) {
  registry.register_type(make_support<std_msgs::Duration>("std_msgs/msg/Duration", name, &detail::read_ros1, &detail::read_cdr));
  }
  registry.register_type(make_support<std_msgs::Empty>("std_msgs/msg/Empty", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Float32>("std_msgs/msg/Float32", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Float32MultiArray>("std_msgs/msg/Float32MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Float64>("std_msgs/msg/Float64", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Float64MultiArray>("std_msgs/msg/Float64MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int16>("std_msgs/msg/Int16", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int16MultiArray>("std_msgs/msg/Int16MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int32>("std_msgs/msg/Int32", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int32MultiArray>("std_msgs/msg/Int32MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int64>("std_msgs/msg/Int64", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int64MultiArray>("std_msgs/msg/Int64MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int8>("std_msgs/msg/Int8", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::Int8MultiArray>("std_msgs/msg/Int8MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::String>("std_msgs/msg/String", name, &detail::read_ros1, &detail::read_cdr));
  if (profile == "ros1_noetic" || is_unknown_profile) {
  registry.register_type(make_support<std_msgs::Time>("std_msgs/msg/Time", name, &detail::read_ros1, &detail::read_cdr));
  }
  registry.register_type(make_support<std_msgs::UInt16>("std_msgs/msg/UInt16", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt16MultiArray>("std_msgs/msg/UInt16MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt32>("std_msgs/msg/UInt32", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt32MultiArray>("std_msgs/msg/UInt32MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt64>("std_msgs/msg/UInt64", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt64MultiArray>("std_msgs/msg/UInt64MultiArray", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt8>("std_msgs/msg/UInt8", name, &detail::read_ros1, &detail::read_cdr));
  registry.register_type(make_support<std_msgs::UInt8MultiArray>("std_msgs/msg/UInt8MultiArray", name, &detail::read_ros1, &detail::read_cdr));
}
void register_all_builtin_profiles(TypeRegistry& registry) {
  for (const auto profile : {"ros1_noetic", "ros2_foxy", "ros2_humble", "ros2_jazzy"}) register_builtin_types(registry, profile);
}
}  // namespace rosbags::profiles
