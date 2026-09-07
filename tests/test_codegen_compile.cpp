#include "ros2_humble_registry.hpp"
#include <doctest/doctest.h>

TEST_CASE("generated nested types compile and register" * doctest::test_suite("codegen")) {
  using Envelope = rosbags::generated::ros2_humble::dependencies::msg::AEnvelope;
  Envelope value{};
  value.payload.value = 1;
  value.payload.class_ = 4;
  value.fixed[0].value = 2;
  value.sequence.push_back({3});

  rosbags::TypeRegistry registry;
  rosbags::generated::ros2_humble::register_types(registry);
  CHECK(registry.find("ros2_humble", "dependencies/msg/AEnvelope") != nullptr);
  CHECK(registry.find("ros2_humble", "dependencies/msg/ZPayload") != nullptr);
}
