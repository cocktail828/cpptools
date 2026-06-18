# cpptools

一组实用的 C++ / C 工具库集合，header-only 设计为主，适用于 Linux 系统级开发场景。

## 模块一览

| 模块 | 语言 | 说明 |
|------|------|------|
| [net/uri.h](net/uri.h) | C++20 | RFC 3986 URI 解析器，支持多主机（etcd/MongoDB 连接串）、IPv6、百分号编解码 |
| [slog/slog.h](slog/slog.h) | C++17 | 基于 spdlog 的日志封装，支持滚动文件 + 彩色终端输出，提供 glog 风格的 `LOG(INFO)` / `CHECK()` 宏 |
| [utilities/scope_guard.h](utilities/scope_guard.h) | C++17 | RAII scope guard，支持 `ON_SCOPE_EXIT([]{...})` 语法 |
| [utilities/fs.h](utilities/fs.h) | C++17 | `mkdirall` — 递归创建目录 |
| [utilities/longterm_checker.h](utilities/longterm_checker.h) | C++17 | 定时任务线程 + 计时器 recorder |
| [hardware/clock.h](hardware/clock.h) | C++17 | 读取 CMOS RTC 时钟、TPM2 时钟（可选 TSS2） |
| [progressbar/](progressbar/) | C++11 | 线程安全终端进度条，支持 Docker pull 多行并发 / APT 单行滚动两种风格 |
| [mockfs/](mockfs/) | C | LD_PRELOAD 文件系统 mock 库，将匹配路径的写操作重定向到 /dev/null |

## 依赖

- CMake >= 3.16
- GCC / Clang（C++20）
- [spdlog](https://github.com/gabime/spdlog)（slog 模块）
- [Google Test](https://github.com/google/googletest)（测试）
- libtss2-esys（hardware 模块，可选，通过 `ENABLE_TSS2` 开关）

## 构建与测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

禁用 TPM2 支持：

```bash
cmake -B build -DENABLE_TSS2=OFF
```

构建 progressbar 示例：

```bash
cd progressbar
cmake -B build
cmake --build build
./build/examples/docker_pull
./build/examples/apt_style
```

## 快速使用

### URI 解析

```cpp
#include "uri.h"

auto uri = cpptools::net::URI("mongodb://user:pass@host1:27017,host2:27018/mydb?w=1");
uri.scheme();       // "mongodb"
uri.User();         // {"user", "pass"}
uri.hosts();        // [{host1, 27017}, {host2, 27018}]
uri.path();         // "/mydb"
uri.query();        // "w=1"
```

### 日志

```cpp
#include "slog.h"

cpptools::slog::LogConfig cfg;
cfg.progname = "myapp";
cfg.log_file = "./logs/myapp.log";
cpptools::slog::InitLoggingCompat(cfg);

LOG(INFO) << "server started on port " << 8080;
CHECK(ptr != nullptr);
```

### Scope Guard

```cpp
#include "scope_guard.h"

void foo() {
    auto fd = open("/tmp/x", O_RDONLY);
    ON_SCOPE_EXIT([&] { close(fd); });
    // fd 在函数退出时自动关闭
}
```

### 定时检查器

```cpp
#include "longterm_checker.h"
using namespace std::chrono_literals;

cpptools::utilities::longterm_checker checker(5s, [] {
    // 每 5 秒执行一次的健康检查
});
checker.start();
// ...
checker.stop();
checker.join();
```

## 项目结构

```
cpptools/
├── CMakeLists.txt          # 顶层构建
├── hardware/clock.h        # 硬件时钟
├── mockfs/                 # 文件系统 mock (C)
├── net/uri.h               # URI 解析器
├── progressbar/            # 进度条库（独立子项目）
├── scripts/                # 工具脚本
├── slog/slog.h             # 日志
├── tests/                  # 单元测试
└── utilities/              # 通用工具
```

## License

[MIT](LICENSE)
