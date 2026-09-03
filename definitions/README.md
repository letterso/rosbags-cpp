# Message definitions

This directory is reserved for ROS message definition inputs consumed by
`rosbags-gen`. Definitions are intentionally kept separate from the reader so
applications can build only the profiles they need. The bundled runtime
registry in `include/rosbags/profiles.hpp` covers the common messages used by
the smoke tests; additional official ROS1 Noetic and ROS2 Foxy/Humble/Jazzy
interface packages can be placed under a profile directory and generated with:

```text
rosbags-gen --profile ros2_humble --input definitions/ros2_humble --output build/generated/ros2_humble
```

Keep the upstream package license/NOTICE files next to any imported
definitions.
