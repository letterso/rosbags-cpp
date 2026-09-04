#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "rosbags/rosbags.hpp"
#include "rosbags/serialization.hpp"
namespace rosbags::profiles {
struct Time { std::int32_t sec = 0; std::uint32_t nanosec = 0; };
struct Duration { std::int32_t sec = 0; std::uint32_t nanosec = 0; };
namespace std_msgs {
struct Header;
struct Bool;
struct Byte;
struct MultiArrayDimension;
struct MultiArrayLayout;
struct ByteMultiArray;
struct Char;
struct ColorRGBA;
struct Duration;
struct Empty;
struct Float32;
struct Float32MultiArray;
struct Float64;
struct Float64MultiArray;
struct Int16;
struct Int16MultiArray;
struct Int32;
struct Int32MultiArray;
struct Int64;
struct Int64MultiArray;
struct Int8;
struct Int8MultiArray;
struct String;
struct Time;
struct UInt16;
struct UInt16MultiArray;
struct UInt32;
struct UInt32MultiArray;
struct UInt64;
struct UInt64MultiArray;
struct UInt8;
struct UInt8MultiArray;
}
namespace geometry_msgs {
struct Vector3;
struct Accel;
struct AccelStamped;
struct AccelWithCovariance;
struct AccelWithCovarianceStamped;
struct Inertia;
struct InertiaStamped;
struct Point;
struct Point32;
struct PointStamped;
struct Polygon;
struct PolygonInstance;
struct PolygonInstanceStamped;
struct PolygonStamped;
struct Quaternion;
struct Pose;
struct Pose2D;
struct PoseArray;
struct PoseStamped;
struct PoseWithCovariance;
struct PoseWithCovarianceStamped;
struct QuaternionStamped;
struct Transform;
struct TransformStamped;
struct Twist;
struct TwistStamped;
struct TwistWithCovariance;
struct TwistWithCovarianceStamped;
struct Vector3Stamped;
struct VelocityStamped;
struct Wrench;
struct WrenchStamped;
}
namespace sensor_msgs {
struct BatteryState;
struct RegionOfInterest;
struct CameraInfo;
struct ChannelFloat32;
struct CompressedImage;
struct FluidPressure;
struct Illuminance;
struct Image;
struct Imu;
struct JointState;
struct Joy;
struct JoyFeedback;
struct JoyFeedbackArray;
struct LaserEcho;
struct LaserScan;
struct MagneticField;
struct MultiDOFJointState;
struct MultiEchoLaserScan;
struct NavSatStatus;
struct NavSatFix;
struct PointCloud;
struct PointField;
struct PointCloud2;
struct Range;
struct RelativeHumidity;
struct Temperature;
struct TimeReference;
}
namespace nav_msgs {
struct GridCells;
struct MapMetaData;
struct OccupancyGrid;
struct Odometry;
struct Path;
}
namespace std_msgs {
struct Header {
  std::uint32_t seq{};
  ::rosbags::profiles::Time stamp{};
  std::string frame_id{};
  Header() = default;
  Header(::rosbags::profiles::Time stamp_value, std::string frame_id_value)
      : stamp(stamp_value), frame_id(frame_id_value) {}
  Header(std::uint32_t seq_value, ::rosbags::profiles::Time stamp_value, std::string frame_id_value)
      : seq(seq_value), stamp(stamp_value), frame_id(frame_id_value) {}
};

struct Bool {
  bool data{};
};

struct Byte {
  std::uint8_t data{};
};

struct MultiArrayDimension {
  std::string label{};
  std::uint32_t size{};
  std::uint32_t stride{};
};

struct MultiArrayLayout {
  std::vector<::rosbags::profiles::std_msgs::MultiArrayDimension> dim{};
  std::uint32_t data_offset{};
};

struct ByteMultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::uint8_t> data{};
};

struct Char {
  std::uint8_t data{};
};

struct ColorRGBA {
  float r{};
  float g{};
  float b{};
  float a{};
};

struct Duration {
  ::rosbags::profiles::Duration data{};
};

struct Empty {
};

struct Float32 {
  float data{};
};

struct Float32MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<float> data{};
};

struct Float64 {
  double data{};
};

struct Float64MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<double> data{};
};

struct Int16 {
  std::int16_t data{};
};

struct Int16MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::int16_t> data{};
};

struct Int32 {
  std::int32_t data{};
};

struct Int32MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::int32_t> data{};
};

struct Int64 {
  std::int64_t data{};
};

struct Int64MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::int64_t> data{};
};

struct Int8 {
  std::int8_t data{};
};

struct Int8MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::int8_t> data{};
};

struct String {
  std::string data{};
};

struct Time {
  ::rosbags::profiles::Time data{};
};

struct UInt16 {
  std::uint16_t data{};
};

struct UInt16MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::uint16_t> data{};
};

struct UInt32 {
  std::uint32_t data{};
};

struct UInt32MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::uint32_t> data{};
};

struct UInt64 {
  std::uint64_t data{};
};

struct UInt64MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::uint64_t> data{};
};

struct UInt8 {
  std::uint8_t data{};
};

struct UInt8MultiArray {
  ::rosbags::profiles::std_msgs::MultiArrayLayout layout{};
  std::vector<std::uint8_t> data{};
};

}
namespace geometry_msgs {
struct Vector3 {
  double x{};
  double y{};
  double z{};
};

struct Accel {
  ::rosbags::profiles::geometry_msgs::Vector3 linear{};
  ::rosbags::profiles::geometry_msgs::Vector3 angular{};
};

struct AccelStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Accel accel{};
};

struct AccelWithCovariance {
  ::rosbags::profiles::geometry_msgs::Accel accel{};
  std::array<double, 36> covariance{};
};

struct AccelWithCovarianceStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::AccelWithCovariance accel{};
};

struct Inertia {
  double m{};
  ::rosbags::profiles::geometry_msgs::Vector3 com{};
  double ixx{};
  double ixy{};
  double ixz{};
  double iyy{};
  double iyz{};
  double izz{};
};

struct InertiaStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Inertia inertia{};
};

struct Point {
  double x{};
  double y{};
  double z{};
};

struct Point32 {
  float x{};
  float y{};
  float z{};
};

struct PointStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Point point{};
};

struct Polygon {
  std::vector<::rosbags::profiles::geometry_msgs::Point32> points{};
};

struct PolygonInstance {
  ::rosbags::profiles::geometry_msgs::Polygon polygon{};
  std::int64_t id{};
};

struct PolygonInstanceStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::PolygonInstance polygon{};
};

struct PolygonStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Polygon polygon{};
};

struct Quaternion {
  double x{};
  double y{};
  double z{};
  double w{};
};

struct Pose {
  ::rosbags::profiles::geometry_msgs::Point position{};
  ::rosbags::profiles::geometry_msgs::Quaternion orientation{};
};

struct Pose2D {
  double x{};
  double y{};
  double theta{};
};

struct PoseArray {
  ::rosbags::profiles::std_msgs::Header header{};
  std::vector<::rosbags::profiles::geometry_msgs::Pose> poses{};
};

struct PoseStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Pose pose{};
};

struct PoseWithCovariance {
  ::rosbags::profiles::geometry_msgs::Pose pose{};
  std::array<double, 36> covariance{};
};

struct PoseWithCovarianceStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::PoseWithCovariance pose{};
};

struct QuaternionStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Quaternion quaternion{};
};

struct Transform {
  ::rosbags::profiles::geometry_msgs::Vector3 translation{};
  ::rosbags::profiles::geometry_msgs::Quaternion rotation{};
};

struct TransformStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  std::string child_frame_id{};
  ::rosbags::profiles::geometry_msgs::Transform transform{};
};

struct Twist {
  ::rosbags::profiles::geometry_msgs::Vector3 linear{};
  ::rosbags::profiles::geometry_msgs::Vector3 angular{};
};

struct TwistStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Twist twist{};
};

struct TwistWithCovariance {
  ::rosbags::profiles::geometry_msgs::Twist twist{};
  std::array<double, 36> covariance{};
};

struct TwistWithCovarianceStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::TwistWithCovariance twist{};
};

struct Vector3Stamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Vector3 vector{};
};

struct VelocityStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  std::string body_frame_id{};
  std::string reference_frame_id{};
  ::rosbags::profiles::geometry_msgs::Twist velocity{};
};

struct Wrench {
  ::rosbags::profiles::geometry_msgs::Vector3 force{};
  ::rosbags::profiles::geometry_msgs::Vector3 torque{};
};

struct WrenchStamped {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Wrench wrench{};
};

}
namespace sensor_msgs {
struct BatteryState {
  static constexpr std::uint8_t POWER_SUPPLY_STATUS_UNKNOWN = 0;
  static constexpr std::uint8_t POWER_SUPPLY_STATUS_CHARGING = 1;
  static constexpr std::uint8_t POWER_SUPPLY_STATUS_DISCHARGING = 2;
  static constexpr std::uint8_t POWER_SUPPLY_STATUS_NOT_CHARGING = 3;
  static constexpr std::uint8_t POWER_SUPPLY_STATUS_FULL = 4;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_UNKNOWN = 0;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_GOOD = 1;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_OVERHEAT = 2;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_DEAD = 3;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_OVERVOLTAGE = 4;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_UNSPEC_FAILURE = 5;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_COLD = 6;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE = 7;
  static constexpr std::uint8_t POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE = 8;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_UNKNOWN = 0;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_NIMH = 1;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_LION = 2;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_LIPO = 3;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_LIFE = 4;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_NICD = 5;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_LIMN = 6;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_TERNARY = 7;
  static constexpr std::uint8_t POWER_SUPPLY_TECHNOLOGY_VRLA = 8;
  ::rosbags::profiles::std_msgs::Header header{};
  float voltage{};
  float temperature{};
  float current{};
  float charge{};
  float capacity{};
  float design_capacity{};
  float percentage{};
  std::uint8_t power_supply_status{};
  std::uint8_t power_supply_health{};
  std::uint8_t power_supply_technology{};
  bool present{};
  std::vector<float> cell_voltage{};
  std::vector<float> cell_temperature{};
  std::string location{};
  std::string serial_number{};
};

struct RegionOfInterest {
  std::uint32_t x_offset{};
  std::uint32_t y_offset{};
  std::uint32_t height{};
  std::uint32_t width{};
  bool do_rectify{};
};

struct CameraInfo {
  ::rosbags::profiles::std_msgs::Header header{};
  std::uint32_t height{};
  std::uint32_t width{};
  std::string distortion_model{};
  std::vector<double> D{};
  std::array<double, 9> K{};
  std::array<double, 9> R{};
  std::array<double, 12> P{};
  std::uint32_t binning_x{};
  std::uint32_t binning_y{};
  ::rosbags::profiles::sensor_msgs::RegionOfInterest roi{};
  std::vector<double> d{};
  std::array<double, 9> k{};
  std::array<double, 9> r{};
  std::array<double, 12> p{};
};

struct ChannelFloat32 {
  std::string name{};
  std::vector<float> values{};
};

struct CompressedImage {
  ::rosbags::profiles::std_msgs::Header header{};
  std::string format{};
  std::vector<std::uint8_t> data{};
};

struct FluidPressure {
  ::rosbags::profiles::std_msgs::Header header{};
  double fluid_pressure{};
  double variance{};
};

struct Illuminance {
  ::rosbags::profiles::std_msgs::Header header{};
  double illuminance{};
  double variance{};
};

struct Image {
  ::rosbags::profiles::std_msgs::Header header{};
  std::uint32_t height{};
  std::uint32_t width{};
  std::string encoding{};
  std::uint8_t is_bigendian{};
  std::uint32_t step{};
  std::vector<std::uint8_t> data{};
};

struct Imu {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Quaternion orientation{};
  std::array<double, 9> orientation_covariance{};
  ::rosbags::profiles::geometry_msgs::Vector3 angular_velocity{};
  std::array<double, 9> angular_velocity_covariance{};
  ::rosbags::profiles::geometry_msgs::Vector3 linear_acceleration{};
  std::array<double, 9> linear_acceleration_covariance{};
};

struct JointState {
  ::rosbags::profiles::std_msgs::Header header{};
  std::vector<std::string> name{};
  std::vector<double> position{};
  std::vector<double> velocity{};
  std::vector<double> effort{};
};

struct Joy {
  ::rosbags::profiles::std_msgs::Header header{};
  std::vector<float> axes{};
  std::vector<std::int32_t> buttons{};
};

struct JoyFeedback {
  static constexpr std::uint8_t TYPE_LED = 0;
  static constexpr std::uint8_t TYPE_RUMBLE = 1;
  static constexpr std::uint8_t TYPE_BUZZER = 2;
  std::uint8_t type{};
  std::uint8_t id{};
  float intensity{};
};

struct JoyFeedbackArray {
  std::vector<::rosbags::profiles::sensor_msgs::JoyFeedback> array{};
};

struct LaserEcho {
  std::vector<float> echoes{};
};

struct LaserScan {
  ::rosbags::profiles::std_msgs::Header header{};
  float angle_min{};
  float angle_max{};
  float angle_increment{};
  float time_increment{};
  float scan_time{};
  float range_min{};
  float range_max{};
  std::vector<float> ranges{};
  std::vector<float> intensities{};
};

struct MagneticField {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::geometry_msgs::Vector3 magnetic_field{};
  std::array<double, 9> magnetic_field_covariance{};
};

struct MultiDOFJointState {
  ::rosbags::profiles::std_msgs::Header header{};
  std::vector<std::string> joint_names{};
  std::vector<::rosbags::profiles::geometry_msgs::Transform> transforms{};
  std::vector<::rosbags::profiles::geometry_msgs::Twist> twist{};
  std::vector<::rosbags::profiles::geometry_msgs::Wrench> wrench{};
};

struct MultiEchoLaserScan {
  ::rosbags::profiles::std_msgs::Header header{};
  float angle_min{};
  float angle_max{};
  float angle_increment{};
  float time_increment{};
  float scan_time{};
  float range_min{};
  float range_max{};
  std::vector<::rosbags::profiles::sensor_msgs::LaserEcho> ranges{};
  std::vector<::rosbags::profiles::sensor_msgs::LaserEcho> intensities{};
};

struct NavSatStatus {
  static constexpr std::int8_t STATUS_NO_FIX = -1;
  static constexpr std::int8_t STATUS_FIX = 0;
  static constexpr std::int8_t STATUS_SBAS_FIX = 1;
  static constexpr std::int8_t STATUS_GBAS_FIX = 2;
  static constexpr std::uint16_t SERVICE_GPS = 1;
  static constexpr std::uint16_t SERVICE_GLONASS = 2;
  static constexpr std::uint16_t SERVICE_COMPASS = 4;
  static constexpr std::uint16_t SERVICE_GALILEO = 8;
  std::int8_t status{};
  std::uint16_t service{};
};

struct NavSatFix {
  static constexpr std::uint8_t COVARIANCE_TYPE_UNKNOWN = 0;
  static constexpr std::uint8_t COVARIANCE_TYPE_APPROXIMATED = 1;
  static constexpr std::uint8_t COVARIANCE_TYPE_DIAGONAL_KNOWN = 2;
  static constexpr std::uint8_t COVARIANCE_TYPE_KNOWN = 3;
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::sensor_msgs::NavSatStatus status{};
  double latitude{};
  double longitude{};
  double altitude{};
  std::array<double, 9> position_covariance{};
  std::uint8_t position_covariance_type{};
};

struct PointCloud {
  ::rosbags::profiles::std_msgs::Header header{};
  std::vector<::rosbags::profiles::geometry_msgs::Point32> points{};
  std::vector<::rosbags::profiles::sensor_msgs::ChannelFloat32> channels{};
};

struct PointField {
  static constexpr std::uint8_t INT8 = 1;
  static constexpr std::uint8_t UINT8 = 2;
  static constexpr std::uint8_t INT16 = 3;
  static constexpr std::uint8_t UINT16 = 4;
  static constexpr std::uint8_t INT32 = 5;
  static constexpr std::uint8_t UINT32 = 6;
  static constexpr std::uint8_t FLOAT32 = 7;
  static constexpr std::uint8_t FLOAT64 = 8;
  std::string name{};
  std::uint32_t offset{};
  std::uint8_t datatype{};
  std::uint32_t count{};
};

struct PointCloud2 {
  ::rosbags::profiles::std_msgs::Header header{};
  std::uint32_t height{};
  std::uint32_t width{};
  std::vector<::rosbags::profiles::sensor_msgs::PointField> fields{};
  bool is_bigendian{};
  std::uint32_t point_step{};
  std::uint32_t row_step{};
  std::vector<std::uint8_t> data{};
  bool is_dense{};
};

struct Range {
  static constexpr std::uint8_t ULTRASOUND = 0;
  static constexpr std::uint8_t INFRARED = 1;
  ::rosbags::profiles::std_msgs::Header header{};
  std::uint8_t radiation_type{};
  float field_of_view{};
  float min_range{};
  float max_range{};
  float range{};
  float variance{};
};

struct RelativeHumidity {
  ::rosbags::profiles::std_msgs::Header header{};
  double relative_humidity{};
  double variance{};
};

struct Temperature {
  ::rosbags::profiles::std_msgs::Header header{};
  double temperature{};
  double variance{};
};

struct TimeReference {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::Time time_ref{};
  std::string source{};
};

}
namespace nav_msgs {
struct GridCells {
  ::rosbags::profiles::std_msgs::Header header{};
  float cell_width{};
  float cell_height{};
  std::vector<::rosbags::profiles::geometry_msgs::Point> cells{};
};

struct MapMetaData {
  ::rosbags::profiles::Time map_load_time{};
  float resolution{};
  std::uint32_t width{};
  std::uint32_t height{};
  ::rosbags::profiles::geometry_msgs::Pose origin{};
};

struct OccupancyGrid {
  ::rosbags::profiles::std_msgs::Header header{};
  ::rosbags::profiles::nav_msgs::MapMetaData info{};
  std::vector<std::int8_t> data{};
};

struct Odometry {
  ::rosbags::profiles::std_msgs::Header header{};
  std::string child_frame_id{};
  ::rosbags::profiles::geometry_msgs::PoseWithCovariance pose{};
  ::rosbags::profiles::geometry_msgs::TwistWithCovariance twist{};
};

struct Path {
  ::rosbags::profiles::std_msgs::Header header{};
  std::vector<::rosbags::profiles::geometry_msgs::PoseStamped> poses{};
};

}
namespace builtin_interfaces { using Time = ::rosbags::profiles::Time; using Duration = ::rosbags::profiles::Duration; }
using String = std_msgs::String;
using Empty = std_msgs::Empty;
using Header = std_msgs::Header;
using Vector3 = geometry_msgs::Vector3;
using Quaternion = geometry_msgs::Quaternion;
using Imu = sensor_msgs::Imu;
using StdTime = std_msgs::Time;
using StdDuration = std_msgs::Duration;
namespace detail { using serialization::CdrReader; using serialization::Ros1Reader;
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Vector3&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Vector3&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Accel&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Accel&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Header&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Header&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::AccelStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::AccelStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::AccelWithCovariance&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::AccelWithCovariance&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::AccelWithCovarianceStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Inertia&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Inertia&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::InertiaStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::InertiaStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Point&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Point&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Point32&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Point32&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PointStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PointStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Polygon&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Polygon&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PolygonInstance&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PolygonInstance&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PolygonInstanceStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PolygonInstanceStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PolygonStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PolygonStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Quaternion&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Quaternion&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Pose&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Pose&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Pose2D&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Pose2D&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PoseArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PoseArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PoseStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PoseStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PoseWithCovariance&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PoseWithCovariance&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::PoseWithCovarianceStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::QuaternionStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::QuaternionStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Transform&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Transform&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::TransformStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::TransformStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Twist&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Twist&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::TwistStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::TwistStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::TwistWithCovariance&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::TwistWithCovariance&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::TwistWithCovarianceStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Vector3Stamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Vector3Stamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::VelocityStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::VelocityStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::Wrench&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::Wrench&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::geometry_msgs::WrenchStamped&);
void read_cdr(CdrReader&, ::rosbags::profiles::geometry_msgs::WrenchStamped&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::nav_msgs::GridCells&);
void read_cdr(CdrReader&, ::rosbags::profiles::nav_msgs::GridCells&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::nav_msgs::MapMetaData&);
void read_cdr(CdrReader&, ::rosbags::profiles::nav_msgs::MapMetaData&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::nav_msgs::OccupancyGrid&);
void read_cdr(CdrReader&, ::rosbags::profiles::nav_msgs::OccupancyGrid&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::nav_msgs::Odometry&);
void read_cdr(CdrReader&, ::rosbags::profiles::nav_msgs::Odometry&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::nav_msgs::Path&);
void read_cdr(CdrReader&, ::rosbags::profiles::nav_msgs::Path&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::BatteryState&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::BatteryState&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::RegionOfInterest&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::RegionOfInterest&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::CameraInfo&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::CameraInfo&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::ChannelFloat32&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::ChannelFloat32&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::CompressedImage&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::CompressedImage&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::FluidPressure&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::FluidPressure&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::Illuminance&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::Illuminance&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::Image&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::Image&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::Imu&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::Imu&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::JointState&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::JointState&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::Joy&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::Joy&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::JoyFeedback&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::JoyFeedback&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::JoyFeedbackArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::JoyFeedbackArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::LaserEcho&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::LaserEcho&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::LaserScan&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::LaserScan&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::MagneticField&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::MagneticField&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::MultiDOFJointState&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::MultiDOFJointState&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::MultiEchoLaserScan&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::MultiEchoLaserScan&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::NavSatStatus&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::NavSatStatus&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::NavSatFix&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::NavSatFix&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::PointCloud&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::PointCloud&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::PointField&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::PointField&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::PointCloud2&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::PointCloud2&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::Range&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::Range&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::RelativeHumidity&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::RelativeHumidity&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::Temperature&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::Temperature&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::sensor_msgs::TimeReference&);
void read_cdr(CdrReader&, ::rosbags::profiles::sensor_msgs::TimeReference&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Bool&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Bool&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Byte&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Byte&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::MultiArrayDimension&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::MultiArrayDimension&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::MultiArrayLayout&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::MultiArrayLayout&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::ByteMultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::ByteMultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Char&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Char&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::ColorRGBA&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::ColorRGBA&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Duration&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Duration&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Empty&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Empty&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Float32&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Float32&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Float32MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Float32MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Float64&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Float64&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Float64MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Float64MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int16&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int16&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int16MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int16MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int32&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int32&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int32MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int32MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int64&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int64&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int64MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int64MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int8&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int8&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Int8MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Int8MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::String&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::String&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::Time&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::Time&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt16&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt16&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt16MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt16MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt32&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt32&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt32MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt32MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt64&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt64&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt64MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt64MultiArray&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt8&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt8&);
void read_ros1(Ros1Reader&, ::rosbags::profiles::std_msgs::UInt8MultiArray&);
void read_cdr(CdrReader&, ::rosbags::profiles::std_msgs::UInt8MultiArray&);
void read_ros1(Ros1Reader&, Time&);
void read_cdr(CdrReader&, Time&);
void read_ros1(Ros1Reader&, Duration&);
void read_cdr(CdrReader&, Duration&);
}

void register_builtin_types(TypeRegistry&, std::string_view profile);
void register_all_builtin_profiles(TypeRegistry&);
}  // namespace rosbags::profiles
