# C++ 测试

本目录测试库、命令行工具、代码生成和安装后的下游接入。测试使用仓库内置的
doctest v2.5.3 单头文件版；`test_main.cpp` 提供唯一的 `main()`，不链接额外的
doctest 库。CMake 通过 CTest 注册并运行全部测试。

## 构建和运行

常规开发配置会构建全部四项 CTest：

```bash
cmake -S . -B build \
  -DROSBAGS_BUILD_TESTS=ON \
  -DROSBAGS_BUILD_TOOLS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

不需要 MCAP，或当前环境无法获取 MCAP 依赖时，可关闭 MCAP：

```bash
cmake -S . -B build-no-mcap \
  -DROSBAGS_BUILD_TESTS=ON \
  -DROSBAGS_BUILD_TOOLS=ON \
  -DROSBAGS_ENABLE_MCAP=OFF
cmake --build build-no-mcap -j2
ctest --test-dir build-no-mcap --output-on-failure
```

关闭 `ROSBAGS_BUILD_TOOLS` 时，只注册库测试和安装验收测试；CLI 测试与生成代码
编译测试不会构建。这适合只验证库安装包的场景。

## CTest 项目

| CTest 名称 | 条件 | 可执行文件或脚本 | 覆盖内容 | 超时 |
| --- | --- | --- | --- | --- |
| `rosbags_cpp_project_tests` | `ROSBAGS_BUILD_TESTS=ON` | `rosbags_cpp_tests` | SQLite3、ROS2 目录、ROS1 chunk、压缩、`AnyReader`、内置 profile、类型生成、topic 匹配、时间转换、解码诊断；启用 MCAP 时还覆盖 MCAP 过滤和压缩消息。 | 60 秒 |
| `rosbags_cpp_cli_tests` | 同时开启测试和工具 | `rosbags_cli_tests` | 三个 CLI 的帮助和错误退出码、参数校验、重复 `--input`、包含空格和特殊字符的字面路径，以及真实 SQLite3 输入的 topic 过滤和解码。 | 60 秒 |
| `rosbags_cpp_codegen_compile_test` | 同时开启测试和工具 | `rosbags_codegen_compile_test` | 由构建期 `rosbags-gen` 生成嵌套消息头，编译生成类型并验证类型注册。生成失败会先使构建失败。 | 60 秒 |
| `rosbags_cpp_install_test` | `ROSBAGS_BUILD_TESTS=ON` | `install_test.cmake` | 安装、迁移安装前缀并配置独立下游工程；工具关闭时执行仅链接核心库的下游验收。 | 120 秒 |

`rosbags_cpp_project_tests` 中的 MCAP 用例受 `ROSBAGS_HAS_MCAP` 条件编译；关闭
MCAP 时，其余库测试仍会执行。

## 安装验收

`rosbags_cpp_install_test` 是端到端 CMake 集成测试，执行过程如下：

1. 将当前构建安装到临时前缀，再迁移到名称含空格的目录。
2. 清除 ROS、CMake 前缀和动态库搜索环境变量，避免误用构建机环境。
3. 使用安装后的头文件、库和 CMake 包配置、构建并运行
   `tests/install/` 中的独立消费者。
4. 验证重复 `find_package()` 和已有 `LZ4::lz4`、`zstd::libzstd` 目标不会被覆盖。
5. 开启工具时，验证 `rosbags_generate_messages()` 生成嵌套类型，修改、新增和删除
   定义后会重新生成；同一个 `application` profile 可解码 ROS1 和 CDR 数据。
6. 验证显式宿主生成器、交叉编译时缺少宿主生成器的拒绝路径，以及 C++ 关键字
   不能作为 `PROFILE`。

交叉编译部分只验证 CMake 配置约束，不执行真实异构平台上的程序。

## 目录和辅助设施

| 路径 | 用途 |
| --- | --- |
| `test_project.cpp` | 库的行为与回归测试。 |
| `test_cli.cpp` | 真实命令行程序的进程级测试。 |
| `test_codegen_compile.cpp` | 构建期生成的嵌套消息头的编译与注册测试。 |
| `install_test.cmake`、`install/` | 已安装 CMake package 的独立消费者验收。 |
| `data/` | 代码生成和 CLI 测试使用的小型确定性消息定义。 |
| `data/review/` | 有界字段、byte/time/duration、空元素与递归序列的生成、编译及真实 payload 回归。 |
| `support/temp_directory.hpp` | 随机独占、析构自动清理的临时目录。 |
| `support/process.hpp` | 通过 POSIX `posix_spawn` 直接传递 argv，并分别捕获 stdout/stderr 的进程启动器。 |
| `third_party/doctest/` | 随仓库引入的 doctest v2.5.3。 |

CLI 进程测试依赖 POSIX API。它不经过 shell，因此路径中的空格、引号或 `$` 会作为
普通参数传递。CTest 对库、CLI 和代码生成可执行文件设置 60 秒超时；直接运行这些
程序时不带该限制。

## 定位问题和新增用例

可用 CTest 名称运行单项测试：

```bash
ctest --test-dir build -R '^rosbags_cpp_project_tests$' --output-on-failure
ctest --test-dir build -R '^rosbags_cpp_install_test$' --output-on-failure
```

也可直接使用 doctest 筛选库或 CLI 用例：

```bash
build/rosbags_cpp_tests --test-suite=project
build/rosbags_cpp_tests --list-test-cases
build/rosbags_cli_tests --test-suite=cli
```

新增测试使用 `TEST_CASE`、`CHECK` 和 `REQUIRE`，不要另写 `main()`。行为变更应在
`test_project.cpp` 增加回归覆盖；涉及 CLI、生成器或安装导出时，同时补充相应的
进程、构建期或安装消费者测试。测试输入应保持小、确定且与用例放在同一目录结构中。

## CI 与配置覆盖

GitHub Actions 在 Ubuntu 24.04 上组合验证
`ROSBAGS_ENABLE_MCAP=ON/OFF` 与 `BUILD_SHARED_LIBS=ON/OFF`，每个组合都执行配置、
构建和 CTest。离线构建 MCAP 时，可将
`FETCHCONTENT_SOURCE_DIR_MCAP_BUILDER` 和 `FETCHCONTENT_SOURCE_DIR_MCAP` 指向已存在的
上游源码。外部 MCAP SDK 另行通过 `-DROSBAGS_MCAP_ROOT=/path/to/sdk` 或其 CMake 包的
`CMAKE_PREFIX_PATH` 进行验证。

新增回归覆盖无 topic 的 SQL 时间绑定、uint64 边界、ROS1 错误连接索引与分配前长度检查、目录编码冲突、独立解码预算，以及 zstd 拼接帧、截断和临时文件清理。MCAP 开启时覆盖 default/noMessageIndex/noChunking/noStatistics/noSummary/noRepeatedChannels/noRepeatedSchemas、空文件、乱序写入、时间相同的稳定顺序和 `AnyReader` 合并。

CI 另有 ASan/UBSan 作业，关闭 MCAP 以检测项目自身代码。可在本地按以下方式运行：

```bash
cmake -S . -B build-sanitizers -DCMAKE_BUILD_TYPE=Debug \
  -DROSBAGS_ENABLE_MCAP=OFF \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-sanitizers -j2
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-sanitizers --output-on-failure
```

若运行环境使用 ptrace，LeakSanitizer 无法工作，可在构建（生成器也会运行）和 CTest 前设置 `ASAN_OPTIONS=detect_leaks=0`；此时只验证 ASan/UBSan，不能据此宣称通过了泄漏检测。外部 MCAP SDK 的代码需单独以 sanitizer 构建，才能获得完整的上游检测覆盖。
