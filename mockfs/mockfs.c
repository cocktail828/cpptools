#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <glob.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int (*original_open)(const char *, int, mode_t) = NULL;
static ssize_t (*original_write)(int, const void *, size_t) = NULL;
static ssize_t (*original_read)(int, void *, size_t) = NULL;
static off_t (*original_lseek)(int, off_t, int) = NULL;

static glob_t globbuf;
static int glob_initialized = 0;

__attribute__((constructor)) void init() {
    original_open = dlsym(RTLD_NEXT, "open");
    original_write = dlsym(RTLD_NEXT, "write");
    original_read = dlsym(RTLD_NEXT, "read");
    original_lseek = dlsym(RTLD_NEXT, "lseek");

    if (!original_open || !original_write || !original_read || !original_lseek) {
        fprintf(stderr, "Error: Failed to load original functions\n");
        exit(1);
    }

    char *mock_rules = getenv("MOCKFS");
    if (mock_rules) {
        if (glob(mock_rules, 0, NULL, &globbuf) == 0) {
            glob_initialized = 1;
        }
    }
}

static int should_mock(const char *path) {
    if (!glob_initialized) {
        return 0;
    }

    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        if (strcmp(path, globbuf.gl_pathv[i]) == 0) {
            // fprintf(stderr, "mockfs: '%s' is mocked\n", path);
            return 1;
        }
    }
    return 0;
}

int open(const char *pathname, int flags, ...) {
    if (should_mock(pathname)) {
        int null_fd = original_open("/dev/null", O_RDWR, 0);
        if (null_fd >= 0) {
            return null_fd;
        }
    }

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    return original_open(pathname, flags, mode);
}

ssize_t write(int fd, const void *buf, size_t count) { return original_write(fd, buf, count); }

ssize_t read(int fd, void *buf, size_t count) { return original_read(fd, buf, count); }

off_t lseek(int fd, off_t offset, int whence) { return original_lseek(fd, offset, whence); }