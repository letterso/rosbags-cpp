# rosbags-cpp

[中文文档](README_CN.md)

`rosbags-cpp` is a read-only C++17 reader for ROS1 bag v2.0, ROS2 SQLite3,
ROS2 bag directories, and MCAP. It does not initialize or depend on a ROS
runtime, so it can be embedded in a plain CMake/C++ application.

MCAP support is enabled by default. The build can use an installed SDK or
fetch and build `mcap_builder`; it can also be disabled for an offline build.

## Documentation

- [落盘方案与程序架构](docs/architecture.md)
- [具体使用文档：CLI、C++ API 和自定义消息](docs/usage.md)
- [消息定义输入说明](definitions/README.md)

## Quick build

Required dependencies are SQLite3, yaml-cpp, liblz4, libzstd, and pkg-config.
See the [usage document](docs/usage.md) for CMake versions, optional BZip2,
MCAP configuration, installation, and downstream integration.

```bash
cmake -S . -B build -DROSBAGS_BUILD_TESTS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The command-line tools are built under `build/` when
`ROSBAGS_BUILD_TOOLS=ON`:

```bash
build/rosbags-info PATH
build/rosbags-read PATH [TOPIC ...] [--decode PROFILE]
build/rosbags-gen --profile PROFILE --input DIR_OR_FILE --output DIR
```
