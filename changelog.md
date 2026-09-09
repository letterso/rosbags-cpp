# Changelog

本文件记录 `rosbags-cpp` 的版本更新。

## [0.1.4] - 2026-09-09

### 新增

- 增加结构化错误上下文 `ErrorContext`，支持通过 `RosbagsError::context()` 获取源文件、连接、topic、类型、序列化格式、时间戳和字节偏移；`reason()` 保留原始原因，`what()` 提供包含上下文的可读诊断信息。
- 为 `Message` 增加来源文件信息，为 `Connection` 保留存储中的原始 topic 拼写，并为解码和读取错误提供跨 ROS1、ROS2 SQLite、ROS2 目录及 MCAP 后端的来源定位。
- 增加 `TopicMatchPolicy` 和 `topics_match()`，支持严格匹配、忽略首个斜杠以及基于绝对命名空间解析相对 topic；增加时间戳纳秒/微秒转换函数及范围校验。
- 导出可安装的 CMake package、`rosbags-info`、`rosbags-read` 和 `rosbags-gen`，并增加 `rosbags_generate_messages()`，支持 `.msg`/`.idl` 定义、嵌套依赖、配置变更追踪和交叉编译时指定宿主机生成器。

### 改进

- 完善解码异常处理：保留 `RosbagsError` 派生类型，为截断、尾随数据和自定义解码器异常附加消息上下文及字节偏移，并继续保留原始错误原因。
- 改进已安装包对 SQLite、yaml-cpp、LZ4、zstd、BZip2 和外部 MCAP SDK 的依赖导出、动态库运行时路径及可重定位安装目录的支持。

### 测试与工程维护

- 增加已安装 CMake package 的端到端下游测试，覆盖重定位安装、重复 `find_package()`、已有压缩依赖 target、消息定义新增/修改/删除后的重新生成、ROS1/CDR 解码、交叉编译约束和非法 profile 校验。
- 增加 GitHub Actions CMake 矩阵，覆盖 Ubuntu 22.04/24.04、MCAP 开关和静态/动态库构建；补充构建变体忽略规则。

### 文档

- 更新使用文档，补充 topic 匹配策略、时间戳边界、结构化诊断信息、公共 API 兼容性和已安装 CMake 消息生成集成说明。
- 扩充 C++ 测试说明，记录 CTest 测试目标、安装验收、问题定位方式以及 CI 配置覆盖范围。

## [0.1.3] - 2026-09-07

### 新增

- 为 `rosbags-info`、`rosbags-read` 和 `rosbags-gen` 引入私有 vendored `cxxopts` 单头文件参数解析器，支持 `-h`/`--help`、`--key value` 与 `--key=value` 形式，以及重复的 `--input` 和多个 topic 参数。
- 增加 CLI 集成测试，启动真实命令行程序验证帮助信息、参数错误、退出码、字面路径、多个输入、topic 过滤和消息解码。
- 增加 C++ 测试说明文档，记录 CTest 测试目标、用例筛选方式和测试辅助工具的使用方法。

### 改进

- 统一 CLI 参数和错误处理：帮助请求返回 0，参数错误返回 2，读取或生成失败返回 1，并输出明确的错误信息和帮助文本。
- 将 CLI 测试、库测试和生成代码编译测试统一纳入 CTest 调度，并为每个测试目标设置 60 秒超时。

### 文档

- 更新 README、CLI 使用文档和测试说明，补充新的参数行为、测试组织及运行方式。

## [0.1.2] - 2026-09-04

### 新增

- 增加 `std_msgs`、`geometry_msgs`、`sensor_msgs` 和 `nav_msgs` 内置 profile，覆盖常用基础、几何、传感器和导航消息的 ROS1/ROS2 wire 解码。
- 为 `Reader` 和 `AnyReader` 增加可移动的增量 `MessageCursor` API，支持应用自行暂停、限速和分批读取。
- 增加 `ReaderOptions::max_rosbag1_chunk_bytes` 与 `ResourceLimitError`，限制 ROS1 单个 chunk 解压后的内存占用，默认上限为 256 MiB。

### 修复

- 将 ROS1、ROS2 SQLite、MCAP 和 ROS2 目录后端改为基于 cursor 的增量读取；`AnyReader` 使用堆合并各输入，保持稳定的时间顺序，避免全量缓存消息 payload。
- ROS1 改为按需定位并流式解压 chunk，支持 LZ4/BZip2 解压过程的边界、大小、截断和尾部数据校验，不再将整个 bag 文件载入内存。
- 完善 ROS1/ROS2 元数据、时间戳、SQLite payload、压缩帧和目录资源的边界检查及异常清理，减少 malformed input 导致的资源泄漏或无界分配。
- 强化消息生成器的字段标识符和输入校验，处理 C++ 关键字转义、内置类型引用、重复/冲突定义、循环依赖和生成文件写入失败，并按依赖顺序生成消息类型。
- 修复 MCAP 自动构建时无法通过 CMake 找到 LZ4 和 zstd 的问题，并固定 `mcap_builder` 的版本提交。

### 重构

- 抽取 `DirectoryBackend::close_impl()`，统一目录后端关闭和 `open()` 失败时的子资源清理路径。

### 测试与工程维护

- 增加 Cppcheck 静态检查目标 `rosbags_cpp_cppcheck`，并统一库、工具和测试目标的编译警告选项。
- 增加内置 profile、代码生成依赖排序、未知大小 zstd 帧、ROS1 LZ4 chunk、单 chunk 资源上限、cursor 生命周期及多输入稳定合并顺序的回归测试。
- 新增 `changelog.md`，记录项目版本更新。

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
