#pragma once

#include <rosbags/profiles.hpp>
#include <tuple>

// Conservative wire lower bounds exclude CDR alignment padding.
// Update these together with the corresponding profile readers.
namespace rosbags::serialization {
template <> struct WireMinimum<::rosbags::profiles::Time> {
  static constexpr std::size_t ros1 = 4 + 4;
  static constexpr std::size_t cdr = 4 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::Duration> {
  static constexpr std::size_t ros1 = 4 + 4;
  static constexpr std::size_t cdr = 4 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Vector3> {
  static constexpr std::size_t ros1 = 8 + 8 + 8;
  static constexpr std::size_t cdr = 8 + 8 + 8;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Accel> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Accel{}.linear)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Accel{}.angular)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Accel{}.linear)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Accel{}.angular)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Header> {
  static constexpr std::size_t ros1 = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Header{}.stamp)>::ros1;
  static constexpr std::size_t cdr = 5 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Header{}.stamp)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::AccelStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelStamped{}.accel)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelStamped{}.accel)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::AccelWithCovariance> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovariance{}.covariance)>::value + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovariance{}.accel)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovariance{}.covariance)>::value + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovariance{}.accel)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped{}.accel)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped{}.accel)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Inertia> {
  static constexpr std::size_t ros1 = 8 + 8 + 8 + 8 + 8 + 8 + 8 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Inertia{}.com)>::ros1;
  static constexpr std::size_t cdr = 8 + 8 + 8 + 8 + 8 + 8 + 8 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Inertia{}.com)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::InertiaStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::InertiaStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::InertiaStamped{}.inertia)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::InertiaStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::InertiaStamped{}.inertia)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Point> {
  static constexpr std::size_t ros1 = 8 + 8 + 8;
  static constexpr std::size_t cdr = 8 + 8 + 8;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Point32> {
  static constexpr std::size_t ros1 = 4 + 4 + 4;
  static constexpr std::size_t cdr = 4 + 4 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PointStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PointStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PointStamped{}.point)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PointStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PointStamped{}.point)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Polygon> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 4;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PolygonInstance> {
  static constexpr std::size_t ros1 = 8 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonInstance{}.polygon)>::ros1;
  static constexpr std::size_t cdr = 8 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonInstance{}.polygon)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PolygonInstanceStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonInstanceStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonInstanceStamped{}.polygon)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonInstanceStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonInstanceStamped{}.polygon)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PolygonStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonStamped{}.polygon)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PolygonStamped{}.polygon)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Quaternion> {
  static constexpr std::size_t ros1 = 8 + 8 + 8 + 8;
  static constexpr std::size_t cdr = 8 + 8 + 8 + 8;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Pose> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Pose{}.position)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Pose{}.orientation)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Pose{}.position)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Pose{}.orientation)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Pose2D> {
  static constexpr std::size_t ros1 = 8 + 8 + 8;
  static constexpr std::size_t cdr = 8 + 8 + 8;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PoseArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseArray{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseArray{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PoseStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseStamped{}.pose)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseStamped{}.pose)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PoseWithCovariance> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovariance{}.covariance)>::value + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovariance{}.pose)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovariance{}.covariance)>::value + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovariance{}.pose)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped{}.pose)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped{}.pose)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::QuaternionStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::QuaternionStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::QuaternionStamped{}.quaternion)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::QuaternionStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::QuaternionStamped{}.quaternion)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Transform> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Transform{}.translation)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Transform{}.rotation)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Transform{}.translation)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Transform{}.rotation)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::TransformStamped> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TransformStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TransformStamped{}.transform)>::ros1;
  static constexpr std::size_t cdr = 5 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TransformStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TransformStamped{}.transform)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Twist> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Twist{}.linear)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Twist{}.angular)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Twist{}.linear)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Twist{}.angular)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::TwistStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistStamped{}.twist)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistStamped{}.twist)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::TwistWithCovariance> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovariance{}.covariance)>::value + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovariance{}.twist)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovariance{}.covariance)>::value + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovariance{}.twist)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped{}.twist)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped{}.twist)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Vector3Stamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Vector3Stamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Vector3Stamped{}.vector)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Vector3Stamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Vector3Stamped{}.vector)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::VelocityStamped> {
  static constexpr std::size_t ros1 = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::VelocityStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::VelocityStamped{}.velocity)>::ros1;
  static constexpr std::size_t cdr = 5 + 5 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::VelocityStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::VelocityStamped{}.velocity)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::Wrench> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Wrench{}.force)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Wrench{}.torque)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Wrench{}.force)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::Wrench{}.torque)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::geometry_msgs::WrenchStamped> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::WrenchStamped{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::WrenchStamped{}.wrench)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::geometry_msgs::WrenchStamped{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::geometry_msgs::WrenchStamped{}.wrench)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::nav_msgs::GridCells> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::GridCells{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::GridCells{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::nav_msgs::MapMetaData> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::MapMetaData{}.map_load_time)>::ros1 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::MapMetaData{}.origin)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::MapMetaData{}.map_load_time)>::cdr + WireMinimum<decltype(::rosbags::profiles::nav_msgs::MapMetaData{}.origin)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::nav_msgs::OccupancyGrid> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::OccupancyGrid{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::OccupancyGrid{}.info)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::OccupancyGrid{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::nav_msgs::OccupancyGrid{}.info)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::nav_msgs::Odometry> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Odometry{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Odometry{}.pose)>::ros1 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Odometry{}.twist)>::ros1;
  static constexpr std::size_t cdr = 5 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Odometry{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Odometry{}.pose)>::cdr + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Odometry{}.twist)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::nav_msgs::Path> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Path{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::nav_msgs::Path{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::BatteryState> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4 + 4 + 4 + 4 + 1 + 1 + 1 + 1 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::BatteryState{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4 + 4 + 4 + 4 + 1 + 1 + 1 + 1 + 4 + 4 + 5 + 5 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::BatteryState{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::RegionOfInterest> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4 + 1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4 + 1;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::CameraInfo> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.K)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.R)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.P)>::value + 4 + 4 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.roi)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.k)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.r)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.p)>::value + 4 + 4 + 5 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::CameraInfo{}.roi)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::ChannelFloat32> {
  static constexpr std::size_t ros1 = 4 + 4;
  static constexpr std::size_t cdr = 5 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::CompressedImage> {
  static constexpr std::size_t ros1 = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::CompressedImage{}.header)>::ros1;
  static constexpr std::size_t cdr = 5 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::CompressedImage{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::FluidPressure> {
  static constexpr std::size_t ros1 = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::FluidPressure{}.header)>::ros1;
  static constexpr std::size_t cdr = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::FluidPressure{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::Illuminance> {
  static constexpr std::size_t ros1 = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Illuminance{}.header)>::ros1;
  static constexpr std::size_t cdr = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Illuminance{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::Image> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 1 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Image{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 5 + 1 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Image{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::Imu> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::Imu{}.orientation_covariance)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::Imu{}.angular_velocity_covariance)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::Imu{}.linear_acceleration_covariance)>::value + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.orientation)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.angular_velocity)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.linear_acceleration)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::Imu{}.orientation_covariance)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::Imu{}.angular_velocity_covariance)>::value + 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::Imu{}.linear_acceleration_covariance)>::value + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.orientation)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.angular_velocity)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Imu{}.linear_acceleration)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::JointState> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::JointState{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::JointState{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::Joy> {
  static constexpr std::size_t ros1 = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Joy{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Joy{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::JoyFeedback> {
  static constexpr std::size_t ros1 = 1 + 1 + 4;
  static constexpr std::size_t cdr = 1 + 1 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::JoyFeedbackArray> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 4;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::LaserEcho> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 4;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::LaserScan> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::LaserScan{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::LaserScan{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::MagneticField> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::MagneticField{}.magnetic_field_covariance)>::value + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MagneticField{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MagneticField{}.magnetic_field)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::MagneticField{}.magnetic_field_covariance)>::value + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MagneticField{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MagneticField{}.magnetic_field)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::MultiDOFJointState> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MultiDOFJointState{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MultiDOFJointState{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::MultiEchoLaserScan> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MultiEchoLaserScan{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::MultiEchoLaserScan{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::NavSatStatus> {
  static constexpr std::size_t ros1 = 1 + 2;
  static constexpr std::size_t cdr = 1 + 2;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::NavSatFix> {
  static constexpr std::size_t ros1 = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::NavSatFix{}.position_covariance)>::value + 8 + 8 + 8 + 1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::NavSatFix{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::NavSatFix{}.status)>::ros1;
  static constexpr std::size_t cdr = 8 * std::tuple_size<decltype(::rosbags::profiles::sensor_msgs::NavSatFix{}.position_covariance)>::value + 8 + 8 + 8 + 1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::NavSatFix{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::NavSatFix{}.status)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::PointCloud> {
  static constexpr std::size_t ros1 = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::PointCloud{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::PointCloud{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::PointField> {
  static constexpr std::size_t ros1 = 4 + 4 + 1 + 4;
  static constexpr std::size_t cdr = 5 + 4 + 1 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::PointCloud2> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 1 + 4 + 4 + 4 + 1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::PointCloud2{}.header)>::ros1;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 1 + 4 + 4 + 4 + 1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::PointCloud2{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::Range> {
  static constexpr std::size_t ros1 = 1 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Range{}.header)>::ros1;
  static constexpr std::size_t cdr = 1 + 4 + 4 + 4 + 4 + 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Range{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::RelativeHumidity> {
  static constexpr std::size_t ros1 = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::RelativeHumidity{}.header)>::ros1;
  static constexpr std::size_t cdr = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::RelativeHumidity{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::Temperature> {
  static constexpr std::size_t ros1 = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Temperature{}.header)>::ros1;
  static constexpr std::size_t cdr = 8 + 8 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::Temperature{}.header)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::sensor_msgs::TimeReference> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::TimeReference{}.header)>::ros1 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::TimeReference{}.time_ref)>::ros1;
  static constexpr std::size_t cdr = 5 + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::TimeReference{}.header)>::cdr + WireMinimum<decltype(::rosbags::profiles::sensor_msgs::TimeReference{}.time_ref)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Bool> {
  static constexpr std::size_t ros1 = 1;
  static constexpr std::size_t cdr = 1;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Byte> {
  static constexpr std::size_t ros1 = 1;
  static constexpr std::size_t cdr = 1;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::MultiArrayDimension> {
  static constexpr std::size_t ros1 = 4 + 4 + 4;
  static constexpr std::size_t cdr = 5 + 4 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::MultiArrayLayout> {
  static constexpr std::size_t ros1 = 4 + 4;
  static constexpr std::size_t cdr = 4 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::ByteMultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::ByteMultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::ByteMultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Char> {
  static constexpr std::size_t ros1 = 1;
  static constexpr std::size_t cdr = 1;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::ColorRGBA> {
  static constexpr std::size_t ros1 = 4 + 4 + 4 + 4;
  static constexpr std::size_t cdr = 4 + 4 + 4 + 4;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Duration> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::std_msgs::Duration{}.data)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::std_msgs::Duration{}.data)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Empty> {
  static constexpr std::size_t ros1 = 0;
  static constexpr std::size_t cdr = 0;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Float32> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 4;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Float32MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Float32MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Float32MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Float64> {
  static constexpr std::size_t ros1 = 8;
  static constexpr std::size_t cdr = 8;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Float64MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Float64MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Float64MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int16> {
  static constexpr std::size_t ros1 = 2;
  static constexpr std::size_t cdr = 2;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int16MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int16MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int16MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int32> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 4;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int32MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int32MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int32MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int64> {
  static constexpr std::size_t ros1 = 8;
  static constexpr std::size_t cdr = 8;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int64MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int64MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int64MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int8> {
  static constexpr std::size_t ros1 = 1;
  static constexpr std::size_t cdr = 1;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Int8MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int8MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::Int8MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::String> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 5;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::Time> {
  static constexpr std::size_t ros1 = WireMinimum<decltype(::rosbags::profiles::std_msgs::Time{}.data)>::ros1;
  static constexpr std::size_t cdr = WireMinimum<decltype(::rosbags::profiles::std_msgs::Time{}.data)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt16> {
  static constexpr std::size_t ros1 = 2;
  static constexpr std::size_t cdr = 2;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt16MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt16MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt16MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt32> {
  static constexpr std::size_t ros1 = 4;
  static constexpr std::size_t cdr = 4;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt32MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt32MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt32MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt64> {
  static constexpr std::size_t ros1 = 8;
  static constexpr std::size_t cdr = 8;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt64MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt64MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt64MultiArray{}.layout)>::cdr;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt8> {
  static constexpr std::size_t ros1 = 1;
  static constexpr std::size_t cdr = 1;
};
template <> struct WireMinimum<::rosbags::profiles::std_msgs::UInt8MultiArray> {
  static constexpr std::size_t ros1 = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt8MultiArray{}.layout)>::ros1;
  static constexpr std::size_t cdr = 4 + WireMinimum<decltype(::rosbags::profiles::std_msgs::UInt8MultiArray{}.layout)>::cdr;
};
}  // namespace rosbags::serialization
