#!/bin/bash  # 确保使用bash而不是sh

# 获取脚本所在目录并切换到上级目录
pushd "$(dirname "$0")/.." || exit 1

# 执行 cmake 命令
cmake -B build/native -G "Unix Makefiles" . || {
    echo "Build Failed with Error $?"
    popd
    exit 1
}

# 执行 cmake 构建
cmake --build build/native || {
    echo "Build Failed with Error $?"
    popd
    exit 1
}

# 恢复原始目录
popd

exit 0
