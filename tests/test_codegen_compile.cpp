#include "ros2_humble_registry.hpp"

int main() {
  using Envelope = rosbags::generated::ros2_humble::dependencies::msg::AEnvelope;
  Envelope value{};
  value.payload.value = 1;
  value.payload.class_ = 4;
  value.fixed[0].value = 2;
  value.sequence.push_back({3});

  rosbags::TypeRegistry registry;
  rosbags::generated::ros2_humble::register_types(registry);
  return registry.find("ros2_humble", "dependencies/msg/AEnvelope") ? 0 : 1;
}
