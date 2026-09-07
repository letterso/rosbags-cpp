# C++ 测试

全部 C++ 测试使用仓库内置 doctest，由 CTest 统一运行：

```bash
cmake -S . -B build -DROSBAGS_BUILD_TESTS=ON -DROSBAGS_BUILD_TOOLS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

三个测试目标共用 `test_main.cpp` 和 CMake 的 `rosbags_add_test()`：

| 可执行文件 | 测试内容 |
| --- | --- |
| `rosbags_cpp_tests` | 库读取、解码、profile 和代码生成行为 |
| `rosbags_cli_tests` | 启动真实 CLI，检查退出码、输出、参数和生成文件 |
| `rosbags_codegen_compile_test` | 编译生成的头文件，并验证类型注册 |

关闭 `ROSBAGS_BUILD_TOOLS` 时仅构建库测试。生成代码测试仍通过构建期
`rosbags-gen` 命令生成头文件，编译失败会直接使构建失败。

可直接使用 doctest 参数筛选用例，例如：

```bash
build/rosbags_cli_tests --test-suite=cli
build/rosbags_cpp_tests --list-test-cases
```

新增测试使用 `TEST_CASE`、`CHECK` 和 `REQUIRE`，不要另写 `main()`。
`support/temp_directory.hpp` 提供独占创建、自动清理的临时目录。
`support/process.hpp` 使用 POSIX `posix_spawn` 传递 argv，分别捕获 stdout/stderr，
不经过 shell；目前 CLI 进程测试适用于 POSIX 平台。CTest 为每个测试目标设置
60 秒超时，直接运行测试程序时没有此超时限制。
