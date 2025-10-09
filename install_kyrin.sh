#!/bin/bash

################################################################################
# Drawing Demo - Kyrin OS 2.0 SP1 一键安装脚本
#
# 前置条件：
#   - 系统：Kyrin OS 2.0 SP1
#   - Qt6 已存在于 ~/Qt6/6.7.3/gcc_64/
#   - MapLibre 源码已存在于 ~/maplibre-native-qt/
#
# 功能：
#   1. 检查前置条件（Qt6、MapLibre 源码）
#   2. 安装系统依赖
#   3. 安装 CMake 3.29.6
#   4. 编译安装 OpenSSL 3.0.14
#   5. 检查 MapLibre 兼容性（不兼容则自动重新编译）
#   6. 配置环境变量
#   7. 克隆并编译项目
#   8. 运行程序
################################################################################

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印函数
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "\n${GREEN}======================================${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${GREEN}======================================${NC}\n"
}

################################################################################
# 步骤 1: 检查前置条件
################################################################################
print_step "步骤 1/6: 检查前置条件"

QT6_PATH="$HOME/Qt6/6.7.3/gcc_64"
MAPLIBRE_SRC="$HOME/maplibre-native-qt"
MAPLIBRE_PATH="$HOME/maplibre-native-qt/install"

# 检查 Qt6
if [ ! -d "$QT6_PATH" ]; then
    print_error "未找到 Qt6，请确保 Qt6 安装在 $QT6_PATH"
    exit 1
fi
print_info "✓ Qt6 已找到: $QT6_PATH"

# 检查 MapLibre 源码
if [ ! -d "$MAPLIBRE_SRC" ]; then
    print_error "未找到 MapLibre 源码，请确保源码在 $MAPLIBRE_SRC"
    exit 1
fi
print_info "✓ MapLibre 源码已找到: $MAPLIBRE_SRC"

# 检查 MapLibre install 目录
if [ -d "$MAPLIBRE_PATH" ]; then
    print_info "✓ MapLibre install 目录存在: $MAPLIBRE_PATH（稍后会检查兼容性）"
else
    print_warn "MapLibre install 目录不存在，稍后会自动编译"
fi

################################################################################
# 步骤 2: 安装系统依赖
################################################################################
print_step "步骤 2/6: 安装系统依赖"

print_info "更新软件包列表..."
sudo apt update

print_info "安装基础构建工具..."
sudo apt install -y build-essential git wget

print_info "安装 Qt6 运行依赖..."
sudo apt install -y \
    libxcb-xinerama0 libxcb-cursor0 libxcb-icccm4 \
    libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
    libxcb-render-util0 libxcb-shape0 libxcb-xfixes0 \
    libxcb-xkb1 libxkbcommon-x11-0 libfontconfig1 \
    libdbus-1-3 libgl1 libglib2.0-0

print_info "安装 MapLibre 运行依赖..."
sudo apt install -y \
    libglx-dev libgl1-mesa-dev libicu-dev \
    libcurl4-openssl-dev libuv1-dev zlib1g-dev

print_info "✓ 系统依赖安装完成"

################################################################################
# 步骤 3: 安装 CMake 3.29.6
################################################################################
print_step "步骤 3/6: 安装 CMake 3.29.6"

CMAKE_VERSION="3.29.6"
CMAKE_DIR="$HOME/tools/cmake-$CMAKE_VERSION-linux-x86_64"

if [ -f "$CMAKE_DIR/bin/cmake" ]; then
    print_info "CMake $CMAKE_VERSION 已存在，跳过安装"
else
    print_info "下载并安装 CMake $CMAKE_VERSION..."
    mkdir -p "$HOME/tools"
    cd "$HOME/tools"

    if [ ! -f "cmake-$CMAKE_VERSION-linux-x86_64.tar.gz" ]; then
        wget https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-linux-x86_64.tar.gz
    fi

    tar -xzf cmake-$CMAKE_VERSION-linux-x86_64.tar.gz
    print_info "✓ CMake $CMAKE_VERSION 安装完成"
fi

# 临时添加到 PATH（本次会话使用）
export PATH="$CMAKE_DIR/bin:$PATH"
cmake --version

################################################################################
# 步骤 4: 编译安装 OpenSSL 3.0.14
################################################################################
print_step "步骤 4/6: 编译安装 OpenSSL 3.0.14"

OPENSSL_VERSION="3.0.14"
OPENSSL_DIR="$HOME/.local/openssl-$OPENSSL_VERSION"

if [ -f "$OPENSSL_DIR/lib/libssl.so.3" ]; then
    print_info "OpenSSL $OPENSSL_VERSION 已存在，跳过编译"
else
    print_info "下载 OpenSSL $OPENSSL_VERSION 源码..."
    mkdir -p "$HOME/src"
    cd "$HOME/src"

    if [ ! -f "openssl-$OPENSSL_VERSION.tar.gz" ]; then
        wget https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz
    fi

    tar -xzf openssl-$OPENSSL_VERSION.tar.gz
    cd openssl-$OPENSSL_VERSION

    print_info "编译 OpenSSL（可能需要几分钟）..."
    ./Configure --prefix=$OPENSSL_DIR --libdir=lib shared
    make -j$(nproc)
    make install

    print_info "✓ OpenSSL $OPENSSL_VERSION 编译完成"
fi

# 创建软链接到 Qt6 lib 目录
print_info "创建软链接到 Qt6 库目录..."
ln -sf $OPENSSL_DIR/lib/libssl.so.3    $QT6_PATH/lib/libssl.so.3
ln -sf $OPENSSL_DIR/lib/libcrypto.so.3 $QT6_PATH/lib/libcrypto.so.3

# 验证 Qt6 识别到 OpenSSL 3.0
print_info "验证 Qt6 识别 OpenSSL..."
$QT6_PATH/bin/qtdiag | grep -A3 SSL || print_warn "qtdiag 未能显示 SSL 信息（可能正常）"

################################################################################
# 步骤 5: 检查并编译 MapLibre（如果需要）
################################################################################
print_step "步骤 5/7: 检查 MapLibre 兼容性"

# 检查 MapLibre 是否需要重新编译
print_info "检查 MapLibre 库兼容性..."

# 尝试检查 MapLibre 是否可用
MAPLIBRE_NEEDS_REBUILD=false

if ldd $MAPLIBRE_PATH/lib/libQMapLibreWidgets.so.3 2>&1 | grep -q "not found"; then
    print_warn "MapLibre 库依赖缺失，需要重新编译"
    MAPLIBRE_NEEDS_REBUILD=true
else
    # 尝试简单的符号检查
    if readelf -s $MAPLIBRE_PATH/lib/libQMapLibre.so.3.0.0 2>&1 | grep -q "GLIBC_2.3[89]"; then
        print_warn "MapLibre 需要更新的 glibc 版本，建议重新编译"
        read -p "是否重新编译 MapLibre？(Y/n): " rebuild_choice
        if [[ ! "$rebuild_choice" =~ ^[Nn]$ ]]; then
            MAPLIBRE_NEEDS_REBUILD=true
        fi
    else
        print_info "✓ MapLibre 库兼容性检查通过，跳过编译"
    fi
fi

if [ "$MAPLIBRE_NEEDS_REBUILD" = true ]; then
    print_info "开始编译 MapLibre（可能需要 10-20 分钟）..."

    # 安装编译依赖
    print_info "安装 MapLibre 编译依赖..."
    sudo apt install -y \
        pkg-config ninja-build ccache \
        libxkbcommon0 libxkbcommon-dev \
        libxkbcommon-x11-0 libxkbcommon-x11-dev \
        xkb-data libx11-xcb-dev libxcb1-dev \
        libxcb-xkb-dev libxcb-keysyms1-dev

    # 备份旧的 install 目录
    if [ -d "$MAPLIBRE_PATH" ]; then
        print_info "备份旧的 MapLibre install 目录..."
        mv "$MAPLIBRE_PATH" "${MAPLIBRE_PATH}.backup.$(date +%s)"
    fi

    # 使用已有的 MapLibre 源码（已在步骤1检查过）
    print_info "使用 MapLibre 源码: $MAPLIBRE_SRC"

    cd "$MAPLIBRE_SRC"

    # 清理旧的构建
    rm -rf build install
    mkdir build && cd build

    # 配置并编译
    print_info "配置 CMake..."
    cmake .. \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$MAPLIBRE_PATH" \
        -DCMAKE_PREFIX_PATH="$QT6_PATH"

    print_info "开始编译（使用 $(nproc) 核心）..."
    ninja -j$(nproc)

    print_info "安装到 $MAPLIBRE_PATH..."
    ninja install

    print_info "✓ MapLibre 编译完成！"
fi

################################################################################
# 步骤 6: 配置环境变量（写入 ~/.bashrc）
################################################################################
print_step "步骤 6/7: 配置环境变量"

BASHRC="$HOME/.bashrc"
MARKER="# ====== drawing-demo 项目配置 ======"

if grep -q "$MARKER" "$BASHRC"; then
    print_info "环境变量已配置，跳过写入"
else
    print_info "写入环境变量到 ~/.bashrc..."
    cat >> "$BASHRC" << EOF

$MARKER

# CMake 3.29.6（Kyrin OS 系统版本过低）
export PATH="$CMAKE_DIR/bin:\$PATH"

# Qt6 6.7.3
export PATH="\$HOME/Qt6/6.7.3/gcc_64/bin:\$PATH"
export CMAKE_PREFIX_PATH="\$HOME/Qt6/6.7.3/gcc_64:\$CMAKE_PREFIX_PATH"

# ====== 配置结束 ======
EOF
    print_info "✓ 环境变量已写入 ~/.bashrc"
fi

# 应用环境变量（本次会话）
export PATH="$QT6_PATH/bin:$PATH"
export CMAKE_PREFIX_PATH="$QT6_PATH:$CMAKE_PREFIX_PATH"

################################################################################
# 步骤 7: 克隆并编译项目
################################################################################
print_step "步骤 7/7: 克隆并编译项目"

PROJECT_DIR="$HOME/projects/drawing-demo"
REPO_URL="https://github.com/ginmanzzz/UAVControl.git"

if [ -d "$PROJECT_DIR" ]; then
    print_warn "项目目录已存在: $PROJECT_DIR"
    read -p "是否删除并重新克隆？(y/N): " confirm
    if [[ "$confirm" =~ ^[Yy]$ ]]; then
        rm -rf "$PROJECT_DIR"
    else
        print_info "保留现有项目目录"
    fi
fi

if [ ! -d "$PROJECT_DIR" ]; then
    print_info "克隆项目仓库..."
    mkdir -p "$HOME/projects"
    cd "$HOME/projects"
    git clone "$REPO_URL" drawing-demo
fi

cd "$PROJECT_DIR"

print_info "编译项目..."
mkdir -p build
cd build

# 清理之前的构建（如果存在）
if [ -f "CMakeCache.txt" ]; then
    print_info "清理旧的 CMake 缓存..."
    rm -f CMakeCache.txt
fi

cmake ..
cmake --build . -j$(nproc)

print_info "✓ 项目编译完成！"

################################################################################
# 完成
################################################################################
print_step "安装完成！"

echo -e "${GREEN}所有步骤已完成！${NC}\n"
echo -e "运行程序："
echo -e "  ${YELLOW}cd $PROJECT_DIR/build${NC}"
echo -e "  ${YELLOW}./drawing-demo${NC}\n"

# 询问是否立即运行
read -p "是否立即运行程序？(Y/n): " run_now
if [[ ! "$run_now" =~ ^[Nn]$ ]]; then
    print_info "启动程序..."
    cd "$PROJECT_DIR/build"
    ./drawing-demo
else
    print_info "稍后可手动运行程序"
fi
