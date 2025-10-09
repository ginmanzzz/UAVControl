#!/bin/bash

################################################################################
# Drawing Demo - Kyrin OS 2.0 SP1 一键安装脚本
#
# 前置条件：
#   - 系统：Kyrin OS 2.0 SP1
#   - Qt6 已存在于 ~/Qt6/6.7.3/gcc_64/
#   - MapLibre 已存在于 ~/projects/maplibre-native-qt/install/
#
# 功能：
#   1. 安装系统依赖
#   2. 安装 CMake 3.29.6
#   3. 编译安装 OpenSSL 3.0.14
#   4. 配置环境变量
#   5. 克隆并编译项目
#   6. 运行程序
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

if [ ! -d "$HOME/Qt6/6.7.3/gcc_64" ]; then
    print_error "未找到 Qt6，请确保 Qt6 安装在 $HOME/Qt6/6.7.3/gcc_64/"
    exit 1
fi
print_info "✓ Qt6 已找到: $HOME/Qt6/6.7.3/gcc_64"

if [ ! -d "$HOME/projects/maplibre-native-qt/install" ]; then
    print_error "未找到 MapLibre，请确保 MapLibre 安装在 $HOME/projects/maplibre-native-qt/install/"
    exit 1
fi
print_info "✓ MapLibre 已找到: $HOME/projects/maplibre-native-qt/install"

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
ln -sf $OPENSSL_DIR/lib/libssl.so.3    $HOME/Qt6/6.7.3/gcc_64/lib/libssl.so.3
ln -sf $OPENSSL_DIR/lib/libcrypto.so.3 $HOME/Qt6/6.7.3/gcc_64/lib/libcrypto.so.3

# 验证 Qt6 识别到 OpenSSL 3.0
print_info "验证 Qt6 识别 OpenSSL..."
$HOME/Qt6/6.7.3/gcc_64/bin/qtdiag | grep -A3 SSL || print_warn "qtdiag 未能显示 SSL 信息（可能正常）"

################################################################################
# 步骤 5: 配置环境变量（写入 ~/.bashrc）
################################################################################
print_step "步骤 5/6: 配置环境变量"

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
export PATH="$HOME/Qt6/6.7.3/gcc_64/bin:$PATH"
export CMAKE_PREFIX_PATH="$HOME/Qt6/6.7.3/gcc_64:$CMAKE_PREFIX_PATH"

################################################################################
# 步骤 6: 克隆并编译项目
################################################################################
print_step "步骤 6/6: 克隆并编译项目"

PROJECT_DIR="$HOME/projects/drawing-demo"
REPO_URL="git@github.com:ginmanzzz/UAVControl.git"

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
