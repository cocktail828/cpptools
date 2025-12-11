# MockFS - 文件系统 Mock 共享库

MockFS 是一个用于模拟文件系统操作的共享库（.so）。它通过拦截系统调用（如 open、write、read 和 lseek），将匹配特定规则的文件操作重定向到 /dev/null，从而实现文件操作的 Mock。
## 功能
- Mock 文件写操作：根据环境变量 MOCKFS 中定义的规则，匹配文件路径。如果文件路径匹配规则，则将写操作重定向到 /dev/null。
- 保持读和 seek 操作正常：读操作和 lseek 操作不会被 Mock，确保程序正常运行。

使用方法
1. 设置环境变量
设置环境变量 MOCKFS，定义需要 Mock 的文件路径规则。规则支持 glob 模式匹配。
```
export MOCKFS="/tmp/mock_file*"
```

2. 编译共享库
将源代码编译为共享库：
```
gcc -shared -fPIC -o libmockfs.so mockfs.c -ldl
```

3. 加载共享库
使用 LD_PRELOAD 加载共享库，运行目标程序。例如：
```
LD_PRELOAD=./libmockfs.so your_program
```

4. 示例
假设有一个测试程序 test.c：
```
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/tmp/mock_file.txt", O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "Hello, World!", 13);
    close(fd);

    fd = open("/tmp/mock_file.txt", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    char buf[14] = {0};
    read(fd, buf, 13);
    printf("Read: %s\n", buf);
    close(fd);

    return 0;
}
```

编译并运行测试：

```
gcc -o test test.c
MOCKFS=/tmp/mock_file* LD_PRELOAD=./libmockfs.so ./test
```