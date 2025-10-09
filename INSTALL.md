# 一键安装脚本使用说明

## 🚀 快速开始（Kyrin OS 2.0 SP1）

### 前置条件

确保你的系统已经有以下内容（假设已经复制到位）：

- ✅ Qt6 位于 `~/Qt6/6.7.3/gcc_64/`
- ✅ MapLibre 位于 `~/projects/maplibre-native-qt/install/`

### 一键安装

在全新的 Kyrin OS 系统上，只需运行以下命令：

```bash
# 1. 下载安装脚本
wget https://raw.githubusercontent.com/ginmanzzz/UAVControl/main/install_kyrin.sh

# 或者如果你已经克隆了仓库
cd ~/projects/drawing-demo

# 2. 添加执行权限
chmod +x install_kyrin.sh

# 3. 运行安装脚本（需要 sudo 权限安装系统依赖）
./install_kyrin.sh
```

### 脚本会自动完成：

1. ✅ **检查前置条件**（Qt6 和 MapLibre 是否存在）
2. ✅ **安装系统依赖**（build-essential、Qt6 运行库、MapLibre 运行库）
3. ✅ **安装 CMake 3.29.6**（到 `~/tools/`）
4. ✅ **编译 OpenSSL 3.0.14**（到 `~/.local/openssl-3.0.14/`）
5. ✅ **配置环境变量**（自动写入 `~/.bashrc`）
6. ✅ **克隆项目**（从 GitHub）
7. ✅ **编译项目**
8. ✅ **询问是否立即运行**

---

## 🎯 手动安装（参考 README.md）

如果你想了解每一步的详细过程，或者遇到问题需要排查，请参考：

- [README.md](README.md) - 完整的分步安装指南
- [KYRIN_OS_SETUP.md](KYRIN_OS_SETUP.md) - 配置说明和优化细节

---

## ⚠️ 常见问题

### Q: 脚本在哪一步失败了？

脚本使用 `set -e`，遇到错误会立即停止并显示错误信息。根据错误提示检查对应步骤。

### Q: 我已经安装过部分依赖，会重复安装吗？

不会。脚本会检查以下内容是否已存在：
- CMake 3.29.6 已安装 → 跳过
- OpenSSL 3.0.14 已编译 → 跳过
- 环境变量已配置 → 跳过
- 项目目录已存在 → 询问是否重新克隆

### Q: 脚本会修改我的 ~/.bashrc 吗？

会，但只会在末尾添加配置，并用注释标记：
```bash
# ====== drawing-demo 项目配置 ======
...
# ====== 配置结束 ======
```

如果检测到已配置（存在标记），则跳过。

### Q: 我想卸载怎么办？

手动删除以下内容：

```bash
# 1. 删除项目
rm -rf ~/projects/drawing-demo

# 2. 删除 CMake
rm -rf ~/tools/cmake-3.29.6-linux-x86_64

# 3. 删除 OpenSSL
rm -rf ~/.local/openssl-3.0.14
rm -f ~/Qt6/6.7.3/gcc_64/lib/libssl.so.3
rm -f ~/Qt6/6.7.3/gcc_64/lib/libcrypto.so.3

# 4. 删除 ~/.bashrc 中的配置（手动编辑，删除标记之间的内容）
nano ~/.bashrc
```

---

## 📋 脚本详细流程

```
[步骤 1/6] 检查前置条件
  ├─ 检查 Qt6 是否存在
  └─ 检查 MapLibre 是否存在

[步骤 2/6] 安装系统依赖
  ├─ apt update
  ├─ 安装 build-essential、git、wget
  ├─ 安装 Qt6 运行依赖（libxcb-*、libgl1 等）
  └─ 安装 MapLibre 运行依赖（libicu、libcurl 等）

[步骤 3/6] 安装 CMake 3.29.6
  ├─ 下载 cmake-3.29.6-linux-x86_64.tar.gz
  ├─ 解压到 ~/tools/
  └─ 临时添加到 PATH

[步骤 4/6] 编译 OpenSSL 3.0.14
  ├─ 下载 openssl-3.0.14.tar.gz
  ├─ 编译并安装到 ~/.local/openssl-3.0.14/
  ├─ 创建软链接到 Qt6 lib 目录
  └─ 验证 Qt6 识别 OpenSSL

[步骤 5/6] 配置环境变量
  ├─ 检查 ~/.bashrc 是否已配置
  ├─ 写入 CMake、Qt6 路径
  └─ 应用环境变量（当前会话）

[步骤 6/6] 克隆并编译项目
  ├─ 检查项目目录是否存在
  ├─ 克隆 git@github.com:ginmanzzz/UAVControl.git
  ├─ 运行 cmake 配置
  ├─ 运行 cmake --build 编译
  └─ 询问是否立即运行程序
```

---

## 📞 获取帮助

- GitHub Issues: https://github.com/ginmanzzz/UAVControl/issues
- 完整文档: [README.md](README.md)

---

**最后更新**: 2025-10-09
**适用系统**: Kyrin OS 2.0 SP1
