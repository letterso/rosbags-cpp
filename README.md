# rosbags-cpp

`rosbags-cpp` 是一个只读 C++17 bag 读取库，支持 ROS1 bag v2.0、ROS2 SQLite3 和 ROS2 MCAP。库编译运行不依赖任何ROS环境，可以嵌入普通 CMake/C++ 应用。

MCAP 默认启用；构建时可以使用已有 SDK，也可以自动下载并编译`mcap_builder`，还可以关闭 MCAP 进行离线构建。

## 快速构建

基础依赖为 SQLite3、yaml-cpp、liblz4、libzstd 和 pkg-config。CMake版本、BZip2/MCAP 配置、安装和下游工程接入请参阅[具体使用文档](docs/usage.md)。

```bash
cmake -S . -B build -DROSBAGS_BUILD_TESTS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

启用 `ROSBAGS_BUILD_TOOLS=ON` 时，命令行工具位于 `build/`：

```bash
build/rosbags-info PATH
build/rosbags-read PATH [TOPIC ...] [--decode PROFILE]
build/rosbags-gen --profile PROFILE --input DIR_OR_FILE --output DIR
```

## 文档

- [落盘方案与程序架构](docs/architecture.md)
- [具体使用文档：CLI、C++ API 和自定义消息](docs/usage.md)
- [消息定义输入说明](definitions/README.md)
- [更新说明](changelog.md)