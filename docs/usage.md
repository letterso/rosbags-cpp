# rosbags-cpp 使用文档

本文档面向使用者，给出从构建、查看 bag、读取原始消息，到 C++ 类型解码和自定义消息生成的完整流程。

## 1. 环境和构建

### 1.1 依赖

必须依赖：

- C++17 编译器；
- CMake 3.16 或更高版本；
- SQLite3、yaml-cpp、liblz4、libzstd 和 pkg-config 开发文件。

可选依赖：

- BZip2：启用 ROS1 BZip2 chunk 解压；
- MCAP：`ROSBAGS_ENABLE_MCAP=ON` 时默认启用。自动 FetchContent 路径要求 CMake 3.22.1 或更高版本，并需要 Git/网络访问，或预先准备可用的 FetchContent 缓存。

### 1.2 常用构建

默认构建会启用 MCAP：

```bash
cmake -S . -B build \
  -DROSBAGS_BUILD_TESTS=ON \
  -DROSBAGS_BUILD_TOOLS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

没有网络或不需要 MCAP 时：

```bash
cmake -S . -B build-no-mcap \
  -DROSBAGS_ENABLE_MCAP=OFF \
  -DROSBAGS_BUILD_TESTS=ON
cmake --build build-no-mcap -j2
```

工程 C++ 测试使用 vendored 的 doctest v2.5.3 single-header，不链接额外测试库。

```bash
build-no-mcap/rosbags_cpp_tests
```

使用已有 MCAP SDK 时，前缀需要包含 `include/mcap_vendor/mcap/reader.hpp` 和 `lib/libmcap.so`：

```bash
cmake -S . -B build-mcap \
  -DROSBAGS_MCAP_ROOT=/path/to/mcap-prefix
```

自动下载的仓库和版本可覆盖：

```bash
cmake -S . -B build-mcap \
  -DROSBAGS_MCAP_BUILDER_REPOSITORY=https://github.com/olympus-robotics/mcap_builder.git \
  -DROSBAGS_MCAP_BUILDER_TAG=main
```

`mcap_builder` 使用 CMake config package 查找 LZ4 和 zstd，而 Ubuntu 的
开发包通常仅提供 pkg-config 文件。MCAP 自动构建时，本项目会在构建目录
生成私有的 `lz4Config.cmake` 和 `zstdConfig.cmake` 适配包，分别导出
`LZ4::lz4` 与 `zstd::libzstd` 并链接到 `PkgConfig::LZ4`、
`PkgConfig::ZSTD`；无需向系统安装额外的 CMake 包文件。

### 1.3 安装和下游工程

```bash
cmake --install build --prefix /opt/rosbags_cpp
```

下游项目使用导出的 CMake target：

```cmake
cmake_minimum_required(VERSION 3.16)
project(example LANGUAGES CXX)

find_package(rosbags_cpp CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE rosbags_cpp::rosbags_cpp)
```

MCAP 由本项目自动构建并安装时，安装包同时导出 `mcap` 依赖；下游不需要再初始化 ROS。

## 2. 输入路径和格式判断

`Reader` 根据输入路径选择后端：

| 输入 | 后端 |
| --- | --- |
| 带 `metadata.yaml` 的目录 | ROS2 目录后端，根据 `storage_identifier` 创建 SQLite3 或 MCAP 子后端。 |
| `.bag` 文件 | ROS1 bag v2.0 后端。 |
| `.mcap` 文件 | MCAP 后端。 |
| 其他文件（例如 `.db3`、`.dbs`） | SQLite3 后端。 |

ROS2 目录中的 `relative_file_paths` 必须指向目录内实际存在的存储文件。目录级连接信息来自 `metadata.yaml`，消息仍由底层 SQLite3/MCAP 后端读取。

## 3. CLI 使用

所有 CLI 都是一次性命令；参数错误或格式错误会输出 `error: ...` 并返回非零退出码。当前 CLI 没有独立的 `--help` 选项，参数形式如下。

### 3.1 查看摘要：`rosbags-info`

```bash
build/rosbags-info PATH
```

示例：

```bash
build/rosbags-info /data/run_01
build/rosbags-info /data/legacy.bag
build/rosbags-info /data/run_01/rosbag2_0.mcap
```

输出包括：

- `storage`：`rosbag1`、`sqlite3` 或 `mcap`；
- `start_ns`、`end_ns`、`duration_ns`；
- 总消息数；
- 每个连接的 topic、规范化类型、消息数、序列化格式和定义格式。

库使用半开时间区间 `[start_ns, end_ns)`，因此有消息时 `end_ns` 是最后一条消息时间戳加 1。

### 3.2 读取原始消息：`rosbags-read`

```bash
build/rosbags-read PATH [TOPIC ...] [--decode PROFILE]
```

不指定 topic 时读取所有 topic；可以指定多个 topic：

```bash
build/rosbags-read /data/run_01 /imu0 /camera/image_raw
```

原始模式每行输出：

```text
TIMESTAMP_NS TOPIC PAYLOAD_BYTES
```

对于大文件需要快速抽样时，可以由 shell 截断输出。截断只代表短读，不代表全文件扫描完成：

```bash
build/rosbags-read /data/run_01 /imu0 | head -n 5
```

### 3.3 内置类型解码

`--decode PROFILE` 会注册项目内置类型并尝试解码。可用 profile：

- `ros1_noetic`；
- `ros2_foxy`；
- `ros2_humble`；
- `ros2_jazzy`。

示例：

```bash
build/rosbags-read /data/legacy.bag /imu0 --decode ros1_noetic
build/rosbags-read /data/run_01 /imu0 --decode ros2_humble
```

CLI 默认使用 `WarnAndSkip`：未注册类型会写 warning 并跳过，不会伪装成已解码对象。当前内置 profile 覆盖 `std_msgs`、`geometry_msgs`、`sensor_msgs` 和 `nav_msgs` 的直接消息类型，并提供 `rosbags::profiles::<package>::<Type>` C++ 类型。为兼容旧代码，`String`、`Empty`、`Header`、`Vector3`、`Quaternion` 和 `Imu` 仍可通过 `rosbags::profiles` 的扁平别名访问。

ROS1 `ros1_noetic` 的 `std_msgs/Header` 包含 `seq`；ROS2 CDR 的 Header 按官方定义仅包含 `stamp` 和 `frame_id`。`sensor_msgs/CameraInfo` 同样保留 ROS1 的大写 `D/K/R/P` 与 ROS2 的小写 `d/k/r/p` 字段，读取器会按 profile 选择对应 wire layout。ROS2 Humble/Jazzy 还注册 `geometry_msgs/PolygonInstance`、`PolygonInstanceStamped` 和 `VelocityStamped`；Foxy 不注册这些较新的类型。

### 3.4 生成自定义类型：`rosbags-gen`

```bash
build/rosbags-gen \
  --profile PROFILE \
  --input DIR_OR_FILE \
  --output OUTPUT_DIR
```

`--input` 可以重复，用于合并多个定义目录或文件。输入目录递归搜索 `.msg` 和 `.idl`；输出目录会生成：

- `<profile>_messages.hpp`；
- `<profile>_registry.hpp`。

例如：

```bash
build/rosbags-gen \
  --profile ros2_humble \
  --input definitions/test_msgs \
  --output build/generated/test_msgs
```

## 4. C++ 程序调用

### 4.1 读取元数据和连接

```cpp
#include <rosbags/rosbags.hpp>

#include <iostream>

int main() {
  rosbags::Reader reader("/data/run_01");
  reader.open();

  const auto& metadata = reader.metadata();
  std::cout << rosbags::storage_kind_name(metadata.storage) << '\n'
            << metadata.message_count << '\n';
  for (const auto& connection : reader.connections()) {
    std::cout << connection.topic << " " << connection.type << '\n';
  }
}
```

`metadata()`、`connections()` 和 `read_raw()` 应在 `open()` 成功后调用。`Connection` 指针和引用只在对应 reader 保持打开期间有效。

### 4.2 原始读取和过滤

```cpp
#include <rosbags/rosbags.hpp>

int main() {
  rosbags::Reader reader("/data/run_01");
  reader.open();

  rosbags::ReadFilter filter;
  filter.topics = {"/imu0", "/camera/image_raw"};
  filter.start = 1403637130549000000ULL;
  filter.stop = 1403637130550000000ULL;

  reader.read_raw(filter, [](const rosbags::Message& message) {
    // message.bytes 是共享的原始序列化 payload。
    const auto size = message.bytes ? message.bytes->size() : 0;
    (void)size;
  });
}
```

过滤规则为：topic 规范化后匹配，`start` 包含，`stop` 不包含。`connection_ids` 也可以直接指定连接 ID；通常优先使用 topic 过滤。

### 4.3 内置类型解码

```cpp
#include <rosbags/profiles.hpp>
#include <rosbags/rosbags.hpp>

int main() {
  rosbags::TypeRegistry registry;
  rosbags::profiles::register_builtin_types(registry, "ros2_humble");

  rosbags::Reader reader("/data/run_01");
  reader.open();
  reader.read_decoded(
      {}, registry, "ros2_humble",
      [](const rosbags::Message& message, const rosbags::DecodedMessage& decoded) {
        if (message.connection->type == "sensor_msgs/msg/Imu") {
          const auto& imu = decoded.as<rosbags::profiles::Imu>();
          (void)imu.header.stamp.sec;
        }
      },
      rosbags::UnknownTypePolicy::Error);
}
```

ROS1 输入使用 `ros1_noetic`，ROS2 CDR 输入使用对应的 `ros2_*` profile。profile 参与 registry key；类型已注册但 profile 不匹配时仍会被视为未知类型。

### 4.4 未知类型策略

```cpp
const auto decoded = rosbags::decode(
    message, registry, "ros2_humble",
    rosbags::UnknownTypePolicy::WarnAndRaw,
    [](std::string_view warning) { std::cerr << warning << '\n'; });

if (decoded && decoded->raw_only()) {
  // decoded->source 指向原始消息；没有可用的 C++ 类型对象。
}
```

三种策略的含义：

| 策略 | 结果 |
| --- | --- |
| `WarnAndSkip` | warning 后返回空值；`read_decoded()` 不调用用户回调。 |
| `WarnAndRaw` | warning 后返回 `raw_only()`，可以继续使用 `source->bytes`。 |
| `Error` | 立即抛出 `DecodeError`。 |

### 4.5 多文件读取：`AnyReader`

```cpp
rosbags::AnyReader reader({
    "/data/segment_0.db3",
    "/data/segment_1.db3",
});
reader.open();
reader.read_raw({}, [](const rosbags::Message& message) {
  // 消息按 timestamp 稳定排序。
});
```

所有输入必须属于同一个 ROS 世代。ROS1 与 ROS2 混用会抛出 `RosbagsError`。`AnyReader` 为全局排序暂存消息，输入很多或消息很大时应考虑分批处理。

## 5. 自定义消息类型

### 5.1 推荐目录布局

```text
definitions/
└── demo_msgs/
    ├── msg/
    │   └── Telemetry.msg
    └── idl/
        └── Status.idl
```

包名由 `msg`/`idl` 的父目录推导。单文件输入也应放在标准目录中，例如 `definitions/demo_msgs/msg/Telemetry.msg`。

### 5.2 `.msg` 示例

`definitions/demo_msgs/msg/Telemetry.msg`：

```text
int32 sequence
string frame_id
float64[3] position
float32[] samples
```

支持基本数值类型、`string`、定长数组、无界/有界序列和嵌套消息。ROS1 和 ROS2 生成器都会生成对应读取函数；实际读取时由消息连接的 `serialization_format` 选择 ROS1 或 CDR decoder。

### 5.3 基础 IDL 示例

`definitions/demo_msgs/idl/Status.idl`：

```text
struct Status {
  long code;
  string text;
  sequence<double, 4> values;
};
```

当前 IDL 解析器面向基础 `struct`、字段数组、`sequence<T>` 和常见 primitive 别名。完整 ROS IDL module、注解、常量以及 action/service 结构不应直接假设可以生成，需要先用实际定义做验证。

### 5.4 生成和接入

```bash
build/rosbags-gen \
  --profile ros2_humble \
  --input definitions/demo_msgs \
  --output build/generated/demo_msgs
```

生成文件中的类型命名空间为：

```text
rosbags::generated::<profile>::<package>::msg::<Type>
```

下游 CMake 示例：

```cmake
target_include_directories(my_reader PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/generated/demo_msgs")
target_link_libraries(my_reader PRIVATE rosbags_cpp::rosbags_cpp)
```

C++ 注册并读取：

```cpp
#include "ros2_humble_registry.hpp"
#include <rosbags/rosbags.hpp>

int main() {
  rosbags::TypeRegistry registry;
  rosbags::generated::ros2_humble::register_types(registry);

  rosbags::Reader reader("/data/run_01");
  reader.open();
  rosbags::ReadFilter filter;
  filter.topics = {"/telemetry"};
  reader.read_decoded(
      filter, registry, "ros2_humble",
      [](const rosbags::Message&, const rosbags::DecodedMessage& decoded) {
        using Telemetry = rosbags::generated::ros2_humble::demo_msgs::msg::Telemetry;
        const auto& value = decoded.as<Telemetry>();
        (void)value.sequence;
      },
      rosbags::UnknownTypePolicy::Error);
}
```

生成器会同时生成 ROS1 和 CDR 读取函数；因此同一组定义可以分别用于 `ros1_noetic` 或 `ros2_*` profile，但定义字段必须与录包时的 wire layout 一致。

### 5.5 自定义消息的两条路径

| 路径 | 适用类型 | 注册方式 | 取舍 |
| --- | --- | --- | --- |
| 生成器路径 | 不属于 ROS 标准发行版的消息，以及项目或设备自定义消息 | 用 `rosbags-gen` 生成 `<profile>_messages.hpp` 和 `<profile>_registry.hpp`，调用生成的 `register_types()` | 不修改库内置 profile；定义和 wire layout 随应用版本管理，可同时生成 ROS1/CDR decoder |
| 内置 profile 路径 | 需要长期作为库公共 API 的稳定 ROS 标准消息；把自定义消息手写进库也属于此路径 | 在 `profiles.hpp/.cpp` 增加结构体、ROS1/CDR decoder 和 `register_builtin_types()` 注册 | 使用方便，但会扩大库的固定 API 和维护面，不能覆盖任意包名或设备私有定义 |

建议：凡是非 ROS 标准消息类型，统一走生成器路径。只有在消息属于稳定的
ROS 标准接口、且希望作为所有应用的通用依赖时，才考虑扩展内置 profile；不要为
单个 bag 或设备把自定义类型硬编码进 profile。

## 6. 错误处理和排查

| 现象 | 优先检查 |
| --- | --- |
| `path does not exist` | 输入路径和 ROS2 目录中的 `metadata.yaml`/相对文件名。 |
| `unsupported rosbag2 storage identifier` | `storage_identifier` 是否为 `sqlite3` 或 `mcap`。 |
| `MCAP support is disabled` | 是否启用 `ROSBAGS_ENABLE_MCAP`，或是否提供了可用 SDK/网络缓存。 |
| `unregistered message type` | profile、canonical type 和生成 registry 是否匹配。 |
| `serialized message is truncated/trailing bytes` | 消息定义、ROS1/CDR 选择和自定义字段顺序是否与录包一致。 |
| `unsupported ... compression` | 当前构建是否带 BZip2，ROS2 压缩模式是否为支持的 zstd file/message。 |

异常类型可按以下方式区分：`FormatError` 表示文件格式或结构损坏，`UnsupportedFeature` 表示当前构建/后端没有该能力，`DecodeError` 表示类型或序列化 payload 无法解码，`RosbagsError` 表示通用状态或 I/O 错误。

## 7. 验证建议

完成新环境构建后至少执行：

```bash
ctest --test-dir build --output-on-failure
build/rosbags-info /path/to/bag
build/rosbags-read /path/to/bag /known_topic | head -n 5
```

对于自定义消息，再执行一次 `rosbags-gen`，编译一个注册该生成头文件的最小 C++ 程序，并用 `UnknownTypePolicy::Error` 读取目标 topic。这样可以分别验证存储索引、原始 payload 和类型解码三条链路。
