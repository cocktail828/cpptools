# Progress Bar Library

C++11/14 线程安全的进度条库，支持两种显示模式：

## 功能

- **Docker pull 风格** — 多行并发进度条，每个任务独立显示
- **APT 风格** — 单行底部进度条 + 上方滚动日志

## 特性

- Header-only，无外部依赖
- 线程安全（per-bar mutex + 专用渲染线程）
- 支持终端宽度自适应
- 自动降级为非 TTY 模式
- 支持颜色和不同样式字符

## 构建 (Linux)

```bash
cmake -B build
cmake --build build
./build/examples/docker_pull
./build/examples/apt_style
```

启用测试：
```bash
cmake -B build -DPROGRESSBAR_BUILD_TESTS=ON
cmake --build build
./build/tests/test_bar
```

## 使用示例

### Docker pull 风格

```cpp
#include <progressbar/progressbar.hpp>

pbar::MultiDisplay display;
display.start();

auto bar1 = display.add_bar("layer1: Downloading", 1000);
auto bar2 = display.add_bar("layer2: Downloading", 500);

// 任意线程中更新
std::thread([&bar1] {
    for (int i = 0; i <= 1000; ++i) {
        bar1->set_progress(i);
        bar1->set_status_text(std::to_string(i) + "/1000");
    }
    bar1->set_status(pbar::BarStatus::Complete);
}).detach();

display.stop();
```

### APT 风格

```cpp
#include <progressbar/progressbar.hpp>

pbar::SingleDisplay display;
display.start();

display.log("Reading package lists...");
display.set_progress(1, 10);

display.log("Unpacking libfoo...");
display.set_progress(5, 10);

display.stop();
```

## API

### MultiDisplay

- `add_bar(label, total)` — 添加进度条，返回 shared_ptr
- `remove_bar(id)` — 移除进度条
- `start()` / `stop()` — 启动/停止渲染线程

### SingleDisplay

- `set_progress(current, total)` — 更新进度
- `log(message)` — 添加滚动日志行
- `set_label(label)` — 设置进度条标签
- `start()` / `stop()` — 启动/停止渲染线程

### Bar

- `set_progress(current)` — 设置当前值
- `set_total(total)` — 设置总量
- `set_status(BarStatus)` — 设置状态 (Waiting/InProgress/Complete/Error)
- `set_status_text(text)` — 设置附加文本
- `tick(amount)` — 增量更新

## 许可

MIT
