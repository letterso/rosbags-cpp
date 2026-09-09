#include <rosbags/rosbags.hpp>
#ifdef WITH_GENERATOR
#include "application_registry.hpp"
#endif

int main() {
  if (rosbags::normalize_type("example/Envelope") != "example/msg/Envelope") return 1;
#ifdef WITH_GENERATOR
  rosbags::TypeRegistry registry;
  rosbags::generated::application::register_types(registry);
  using Envelope = rosbags::generated::application::example::msg::Envelope;
  rosbags::Connection connection;
  connection.type = "example/msg/Envelope";
  connection.serialization_format = "ros1";
  // ROS1 Header(seq, sec, nsec, empty frame_id), sequence length, uint32.
  auto bytes = std::make_shared<rosbags::Bytes>(16, 0);
  bytes->insert(bytes->end(), {1, 0, 0, 0, 42, 0, 0, 0});
#ifdef ADDED_FIELD
  bytes->insert(bytes->end(), {7, 0, 0, 0});
#endif
  rosbags::Message message{bytes, 123, &connection, "synthetic"};
  auto decoded = rosbags::decode(message, registry, "application", rosbags::UnknownTypePolicy::Error);
  if (!decoded || decoded->as<Envelope>().points.at(0).value != 42) return 2;
#ifdef ADDED_FIELD
  if (decoded->as<Envelope>().points.at(0).extra != 7) return 3;
#endif
  connection.serialization_format = "cdr";
  bytes = std::make_shared<rosbags::Bytes>(rosbags::Bytes{0, 1, 0, 0});
  bytes->resize(12, 0);  // Header stamp; ROS2 omits the ROS1 sequence number.
  bytes->insert(bytes->end(), {1, 0, 0, 0, 0, 0, 0, 0});  // empty frame_id and padding
  bytes->insert(bytes->end(), {1, 0, 0, 0, 42, 0, 0, 0});
#ifdef ADDED_FIELD
  bytes->insert(bytes->end(), {7, 0, 0, 0});
#endif
  message.bytes = bytes;
  decoded = rosbags::decode(message, registry, "application", rosbags::UnknownTypePolicy::Error);
  if (!decoded || decoded->as<Envelope>().points.at(0).value != 42) return 4;
#endif
  return 0;
}
