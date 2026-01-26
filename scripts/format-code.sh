#!/bin/bash

SUPPORTED_EXTENSIONS=(".c" ".h" ".cpp" ".hpp" ".cc" ".cxx" ".hxx")
# ===================== 固定参数 =====================
# 临时文件，存储待格式化的文件列表
TMP_FILE_LIST=$(mktemp)
# 格式化失败的文件计数
FAILED_COUNT=0
# 格式化成功的文件计数
SUCCESS_COUNT=0

# 函数：检查 clang-format 是否安装
check_clang_format() {
    if ! command -v clang-format &> /dev/null; then
        echo "❌ 错误：未安装 clang-format，请先安装！"
        echo "  Linux 安装：sudo apt install clang-format (Debian/Ubuntu) 或 sudo yum install clang-format (CentOS)"
        echo "  macOS 安装：brew install clang-format"
        exit 1
    fi
}

# 函数：检查是否在 Git 仓库中
check_git_repo() {
    if ! git rev-parse --is-inside-work-tree &> /dev/null; then
        echo "❌ 错误：当前目录不是 Git 仓库！"
        exit 1
    fi
}

# 函数：筛选 Git 变更的 C/C++ 文件（排除已删除文件）
get_changed_files() {
    echo "🔍 正在筛选 Git 变更的 C/C++ 文件..."
    # git diff 筛选：
    # --name-only：仅输出文件名
    # --diff-filter=d：排除已删除文件（d=deleted）
    # HEAD：对比工作区与最新提交（若需仅筛选暂存文件，替换为 --cached）
    git diff --name-only --diff-filter=d HEAD -- | while read -r file; do
        # 检查文件是否存在（避免处理已删除但未提交的文件）
        if [ -f "$file" ]; then
            # 检查文件后缀是否符合要求
            for ext in "${SUPPORTED_EXTENSIONS[@]}"; do
                if [[ "$file" == *"$ext" ]]; then
                    echo "$file" >> "$TMP_FILE_LIST"
                    break
                fi
            done
        fi
    done

    # 检查是否有需要格式化的文件
    if [ ! -s "$TMP_FILE_LIST" ]; then
        echo "✅ 无需要格式化的 C/C++ 变更文件"
        rm -f "$TMP_FILE_LIST"
        exit 0
    fi

    echo "📄 待格式化的文件列表："
    cat "$TMP_FILE_LIST"
    echo "========================================"
}

# 函数：执行 clang-format 格式化
format_files() {
    echo "🚀 开始使用 clang-format 格式化（风格：$CLANG_FORMAT_STYLE）..."
    while read -r file; do
        if clang-format -style=file -i "$file"; then
            echo "✅ 格式化成功：$file"
            ((SUCCESS_COUNT++))
        else
            echo "❌ 格式化失败：$file"
            ((FAILED_COUNT++))
        fi
    done < "$TMP_FILE_LIST"
}

# 函数：输出统计结果
print_summary() {
    echo "========================================"
    echo "📊 格式化完成！"
    echo "   成功：$SUCCESS_COUNT 个文件"
    echo "   失败：$FAILED_COUNT 个文件"
    if [ "$FAILED_COUNT" -eq 0 ]; then
        echo "🎉 所有变更文件格式化成功！"
    else
        echo "⚠️  部分文件格式化失败，请检查文件是否合法！"
        exit 1
    fi
}

check_clang_format
check_git_repo
get_changed_files
format_files
print_summary
# 清理临时文件
rm -f "$TMP_FILE_LIST"
