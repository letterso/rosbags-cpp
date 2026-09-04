# 消息定义

此目录用于存放 `rosbags-gen` 使用的 ROS 消息定义输入。消息定义与读取器代码保持分离，应用可以只构建所需的 profile。内置运行时注册表 `include/rosbags/profiles.hpp` 已覆盖 `std_msgs`、`geometry_msgs`、`sensor_msgs` 和 `nav_msgs` 的直接消息；项目或其他 ROS 接口包仍可放在 profile 目录下并生成：

```text
rosbags-gen --profile ros2_humble --input definitions/ros2_humble --output build/generated/ros2_humble
```
