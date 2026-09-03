# Changelog

本文件记录 `rosbags-cpp` 的版本更新。

## [0.1.1] - 2026-09-03

### 新增

- 增加 Livox ROS 驱动消息定义：`livox_ros_driver/CustomMsg` 和
  `livox_ros_driver/CustomPoint`，可直接交给 `rosbags-gen` 生成类型支持。

### 修复

- 修复嵌套消息代码生成：生成前置声明，并为 `std_msgs/msg/Header` 提供
  ROS1/ROS2 各自的 wire reader，使生成的嵌套消息注册表可以编译。

### 测试

- 将 C++ 测试迁移到仓库内置的 doctest v2.5.3 single-header，并拆分项目测试
  入口，继续覆盖 SQLite、ROS2 目录、ROS1 bag、解码和代码生成路径。

### 文档

- 将主 README 和消息定义说明统一为中文。
- 补充自定义消息的生成器路径与内置 profile 路径说明，以及 doctest 测试运行方式。

## [0.1.0] - 2026-09-03

首个公开版本，提供不依赖 ROS 运行时的只读 C++17 bag 读取能力。

### 核心读取能力

- 支持 ROS1 bag v2.0、ROS2 SQLite3、ROS2 bag 目录和 MCAP。
- 提供 `Reader` 与 `AnyReader`，统一暴露元数据、连接信息、原始 payload、
  topic/连接 ID/时间窗口过滤，以及跨输入按时间戳合并读取。
- 支持 ROS2 `metadata.yaml`、多文件 bag、SQLite schema 1-4、zstd 文件/消息压缩，
  以及 ROS1 `none`/`lz4`/可选 `bz2` chunk 压缩。
- 增加边界检查的 ROS1 与 CDR 序列化读取器、topic/类型名称规范化和明确的格式、
  能力及解码错误类型。

### 类型与解码

- 引入 `TypeRegistry`、`TypeSupport` 和 `DecodedMessage`，支持按 profile 注册
  ROS1/CDR 解码器，并提供未知类型的跳过、原始返回或报错策略。
- 内置 `String`、`Empty`、`Time`、`Duration`、`Vector3`、`Quaternion` 和 `Imu`
  等常见消息 profile。
- 增加 `rosbags-gen` 代码生成器：解析常见 `.msg` 和基础 IDL，生成 C++ 结构体、
  ROS1/CDR 读取函数及 profile 注册表。

### 工具与构建

- 增加 `rosbags-info`、`rosbags-read` 和 `rosbags-gen` 命令行工具。
- CMake 提供库安装、导出 target 和下游 `find_package` 接入；MCAP 默认启用，支持
  使用已有 SDK 或通过 `mcap_builder` 自动获取，也可关闭以进行离线构建。
- 增加 reader、序列化解码、ROS2 目录和代码生成的基础测试覆盖。

### 文档与工程维护

- 增加架构、使用和消息定义输入文档。
- 增加提交信息校验辅助脚本 `.githook.sh`。
