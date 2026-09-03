# 消息定义

此目录用于存放 `rosbags-gen` 使用的 ROS 消息定义输入。消息定义与读取器代码保持分离，应用可以只构建所需的 profile。内置运行时注册表`include/rosbags/profiles.hpp` 覆盖了 smoke test 使用的常见消息；其他 ROS1 Noetic 以及 ROS2 Foxy/Humble/Jazzy 接口包可以放在 profile 目录下并生成：

```text
rosbags-gen --profile ros2_humble --input definitions/ros2_humble --output build/generated/ros2_humble
```
