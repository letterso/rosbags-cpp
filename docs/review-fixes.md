# 审查修复记录

本次按“查询与索引一致性 → 生成器和解码预算 → zstd/MCAP 后端 → 内存与性能”的顺序，修复原审查及补充审查中的 F1–F13。下表沿用审查编号，源码位置以文件名为准。

| 编号 | 修复结果 | 实现与回归 |
| --- | --- | --- |
| F1 | MCAP 完整索引使用 SDK 时间排序；缺消息索引、未分块、缺 summary 时建立受限排序索引及 payload 临时文件；缺统计记录通过独立扫描恢复；空文件正常 EOF。 | [mcap.cpp](../src/mcap.cpp)、MCAP variants / budgets 测试。 |
| F2 | 文件级 zstd 使用固定 64 KiB 输入/输出缓冲，直接写安全创建的临时文件；限制单文件输出、目录临时磁盘总量和解压窗口。 | [sqlite3.cpp](../src/sqlite3.cpp)、[temporary_file.hpp](../src/temporary_file.hpp)、拼接帧、截断、预算及清理测试。 |
| F3 | SQL 仅对实际出现的占位符绑定参数，覆盖无 topic、topic、connection 三种过滤组合。 | SQLite time filters 测试。 |
| F4 | `.msg` 常量从字段名后的 `=` 识别，保留类型中的 `<=`；有界字符串与序列执行实际界限校验。 | [codegen.cpp](../src/codegen.cpp)、生成头编译及 ROS1/CDR payload 测试。 |
| F5 | 分配前检查剩余 wire 字节、元素下限、独立元素数、对象存储与递归深度预算；ROS1 header/count 同时检查文件剩余长度。 | [serialization.hpp](../include/rosbags/serialization.hpp)、内置/生成解码器与 ROS1 资源限制测试。 |
| F6 | ROS1 游标保存预期 connection ID，并与 MSGDATA 校验，避免索引指向其他连接时交付错误消息。 | [rosbag1.cpp](../src/rosbag1.cpp)、错误连接索引测试。 |
| F7 | SQLite start 超过 INT64_MAX 直接返回空；stop 超过 INT64_MAX 省略 SQL 上界，保持 uint64 半开区间语义。 | SQLite time filters 的 0、INT64_MAX、UINT64_MAX 和反向区间回归。 |
| F8 | `byte` 在 ROS1 按 int8、CDR 按 uint8 读取；共享 C++ 字段采用 int16_t，数组同步调整。 | 内置及生成 byte、ByteMultiArray 的负数与无符号范围测试。 |
| F9 | ROS1 `time`/`duration` 引用映射到公共 Time/Duration，分别保留 ROS1 uint32 秒、int32 纳秒与 CDR 对应范围。 | Clock 生成头编译和真实 payload 解码、内置时间边界测试。 |
| F10 | ROS1 空消息元素允许零 wire 字节，通过独立元素数、存储和深度预算限制。 | Empty、Envelope、Node 生成及解码回归。 |
| F11 | 目录打开时验证分片 topic/type、编码以及双方已有的摘要/同格式定义，并建立连接映射；冲突明确失败。 | 目录 metadata 冲突、修正后重试与 connection 过滤测试。 |
| F12 | ROS1 加载新 chunk 前释放旧 chunk，减少替换时的瞬时驻留。 | ROS1 多 chunk、过滤、压缩和资源限制回归。 |
| F13 | 校验 CHUNK_INFO 总计数后，为每连接索引一次预留；四字节读取使用栈缓冲，移除每记录精确 reserve 和小型临时堆分配。 | ROS1 chunk/index 回归。 |

其他改进包括：ROS1/MCAP/目录的连接映射、内置与生成 uint8 序列的批量复制、SQLite statement RAII 和最终 step 状态检查、未支持 IDL 布局明确报错、yaml-cpp 新旧 CMake target 兼容，以及 sanitizer 安装消费者编译/链接参数传递。

## 验收方式

测试使用仓库内置的确定性样例，不依赖原 `/tmp/rosbags-cpp-review*` 目录。代码生成回归同时检查生成、编译和真实 ROS1/CDR 解码；MCAP 回归覆盖乱序写入、相同时间戳、多输入合并、七种 writer 配置与空文件。

最终验证采用 MCAP 开启的 Debug 静态构建、MCAP 关闭的 Release 共享构建，以及 MCAP 开启的 ASan/UBSan 构建。各配置执行完整 CTest，包含库、CLI、生成代码和安装后的独立消费者。

2026-10-28 从全新目录配置、构建和验收的结果：

| 配置 | CTest | 构建目录 |
| --- | --- | --- |
| Debug、静态库、MCAP/BZ2 开启 | 4/4 通过 | `/tmp/rosbags-cpp-final-mcap` |
| Release、共享库、MCAP 关闭、BZ2 开启 | 4/4 通过 | `/tmp/rosbags-cpp-final-shared` |
| Debug、静态库、MCAP/BZ2 开启、ASan/UBSan | 4/4 通过 | `/tmp/rosbags-cpp-final-asan` |

每个目录的 `final-ctest.log` 保存本次输出。构建和测试均未产生新的编译警告或 sanitizer 诊断，`git diff --check` 通过。

本地 sanitizer 验收设置 `ASAN_OPTIONS=detect_leaks=0`（此前 LeakSanitizer 曾报告 ptrace 限制）；本次未验证泄漏，外部 MCAP SDK 也没有重新以 sanitizer 编译。CI 的独立 sanitizer 作业关闭 MCAP，并启用泄漏检测。

## 兼容性与资源边界

- 公共 byte、ByteMultiArray、Time、Duration 布局以及类型支持接口发生变化。下游必须重新编译，并用新生成器重新生成消息头；依赖精确字段类型的代码需同步调整。
- `ReaderOptions` 限制各后端的指定缓冲、输出或临时存储；`DecodeLimits` 限制内置及新生成解码器的对象存储、元素数和嵌套深度。它们不是进程总 RSS 上限；旧单参数自定义 decoder 的内部资源由其实现负责。
- 无索引 MCAP 需要临时磁盘保存 payload，并在内存保留排序项；索引超限时明确失败，目前没有磁盘外部排序。多个子 reader 的资源预算分别生效。
- 文件级 zstd 仍在打开时处理全部分片；分片惰性解压、按字节预算的多 chunk LRU 和 SQLite 启动查询重构留待真实负载数据支持后实施。

资源选项、默认值和使用示例见 [使用文档](usage.md#44-增量游标和资源上限)，变更摘要见 [changelog](../changelog.md)。
