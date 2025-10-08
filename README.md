# MapLibre 地图标注与任务管理工具

这是一个基于 QMapLibre 的交互式地图标注与任务管理工具，支持多种地图元素的绘制、任务组织和交互查看。

## ✨ 主要功能

### 任务管理系统
- **创建/编辑/删除任务** - 组织和管理多个地图标注任务
- **任务切换** - 快速在不同任务间切换，查看对应的地图标记
- **任务可见性控制** - 显示/隐藏特定任务的所有标记

### 地图标注功能
1. **放置盘旋点** 📍 - 使用自定义图标标记地图位置
2. **放置无人机** 🛩️ - 支持多种颜色的无人机图标（黑、红、蓝、紫、绿、黄）
3. **绘制禁飞区域** 🚫 - 两次点击交互绘制圆形禁飞区，支持地形特征选择
4. **绘制多边形区域** 🔷 - 多点绘制任意多边形区域，支持地形特征选择
5. **元素详情查看** 💬 - 点击已放置的元素查看详细信息，支持内联编辑和删除
6. **实时坐标显示** 📊 - 右下角显示鼠标经纬度
7. **智能交互阈值** 🎯 - 根据地图缩放级别自动调整交互距离

### 地形特征系统
- **多边形/禁飞区地形类型** - 平原、丘陵、山地、高山地
- **内联编辑** - 在详情窗口直接修改地形类型
- **默认地形** - 新建区域默认为平原

## 📦 完整安装指南（Ubuntu 24.04）

### 1. 系统依赖安装

```bash
# 更新软件包列表
sudo apt update

# 安装基础构建工具
sudo apt install -y build-essential cmake git

# 安装 Qt6 依赖（在线安装器会用到）
sudo apt install -y \
    libxcb-xinerama0 \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xfixes0 \
    libxcb-xkb1 \
    libxkbcommon-x11-0 \
    libfontconfig1 \
    libdbus-1-3 \
    libgl1 \
    libglib2.0-0

# 安装 MapLibre 编译依赖
sudo apt install -y \
    libglx-dev \
    libgl1-mesa-dev \
    libicu-dev \
    libcurl4-openssl-dev \
    libuv1-dev \
    pkg-config \
    ninja-build \
    ccache
```

### 2. 安装 Qt6

#### 下载 Qt 在线安装器

```bash
cd ~/Downloads

# 下载在线安装器（官方地址）
wget https://d13lb3tujbc8s0.cloudfront.net/onlineinstallers/qt-unified-linux-x64-online.run

# 赋予执行权限
chmod +x qt-unified-linux-x64-online.run

# 运行安装器
./qt-unified-linux-x64-online.run
```

#### 安装器配置步骤

1. **登录** - 使用 Qt 账号登录（没有账号需要先注册）
2. **欢迎页面** - Next
3. **选择安装目录** - 推荐使用 `$HOME/Qt6`
4. **选择组件** - **重要！必须勾选以下组件**：
   ```
   Qt
   └── Qt 6.7.3
       ├── Desktop gcc 64-bit        ✅ 必选
       ├── Additional Libraries       ✅ 必选（展开并勾选全部）
       │   ├── Qt Quick 3D
       │   ├── Qt Charts
       │   ├── Qt Data Visualization
       │   ├── Qt Location
       │   └── ...（其他库）
       └── Developer and Designer Tools
           └── CMake                  ✅ 可选（如果系统已安装可不选）
   ```
   **注意**：一定要展开 "Additional Libraries" 并勾选，否则可能缺少必要的库文件

5. **许可协议** - 同意并 Next
6. **开始安装** - 等待安装完成（约需 10-30 分钟，取决于网速）

#### 设置环境变量（可选但推荐）

```bash
# 编辑 ~/.bashrc
echo 'export PATH="$HOME/Qt6/6.7.3/gcc_64/bin:$PATH"' >> ~/.bashrc
echo 'export CMAKE_PREFIX_PATH="$HOME/Qt6/6.7.3/gcc_64:$CMAKE_PREFIX_PATH"' >> ~/.bashrc

# 使配置生效
source ~/.bashrc
```

### 3. 编译安装 QMapLibre

```bash
# 创建项目目录
mkdir -p ~/projects
cd ~/projects

# 克隆 QMapLibre 仓库
git clone https://github.com/maplibre/maplibre-native-qt.git
cd maplibre-native-qt

# 创建构建目录
mkdir build && cd build

# 配置 CMake（指定 Qt6 和安装路径）
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_PREFIX_PATH="$HOME/Qt6/6.7.3/gcc_64"

# 编译（使用多核加速，根据 CPU 核心数调整 -j 参数）
ninja -j$(nproc)

# 安装到 install 目录
ninja install

# 检查安装结果
ls ../install/lib/cmake/QMapLibre  # 应该能看到 QMapLibreConfig.cmake 等文件
```

**预计编译时间**：5-15 分钟（取决于 CPU 性能）

### 4. 克隆并编译本项目

```bash
# 回到 projects 目录
cd ~/projects

# 克隆项目（或使用你的实际 git 仓库地址）
git clone <your-repo-url> drawing-demo
# 如果本地已有项目，跳过 clone

cd drawing-demo

# 创建构建目录
mkdir -p build && cd build

# 配置 CMake（路径会自动从 CMakeLists.txt 读取）
cmake ..

# 编译
cmake --build . -j$(nproc)
```

**编译输出**：可执行文件位于 `build/drawing-demo`

### 5. 运行程序

```bash
# 在 build 目录中直接运行
cd ~/projects/drawing-demo/build
./drawing-demo

# 或者从项目根目录运行
cd ~/projects/drawing-demo
./build/drawing-demo
```

**注意**：由于 CMakeLists.txt 中已设置 RPATH，通常无需手动设置 `LD_LIBRARY_PATH`

#### 如果出现库找不到的错误

```bash
# 手动设置库路径后运行
export LD_LIBRARY_PATH="$HOME/Qt6/6.7.3/gcc_64/lib:$HOME/projects/maplibre-native-qt/install/lib:$LD_LIBRARY_PATH"
./build/drawing-demo
```

## 🚀 使用说明

### 任务管理

#### 创建任务
1. 点击 **"创建任务"** 按钮
2. 在对话框中输入任务名称和描述
3. 点击确定创建

#### 切换任务
- 在左侧任务列表中点击任务名称即可切换
- 当前任务会高亮显示
- 只有当前任务的标记会显示在地图上

#### 编辑/删除任务
- 在任务列表中选择任务
- 点击 **"编辑任务"** 修改任务信息
- 点击 **"删除任务"** 删除任务（会同时删除该任务的所有标记）

#### 显示/隐藏任务标记
- 勾选任务列表中的复选框显示该任务的标记
- 取消勾选隐藏标记（不会删除，只是暂时不显示）

### 地图标注操作

#### 默认模式：普通浏览
- **左键点击元素**：查看元素详细信息
- **左键拖动**：移动地图
- **滚轮缩放**：放大/缩小
- **右下角**：实时显示鼠标经纬度
- **左下角**：显示当前操作模式提示

#### 放置盘旋点 📍
1. **先选择或创建一个任务**（否则会被拒绝）
2. 点击 **"放置盘旋点"** 按钮
3. 在地图上点击任意位置
4. 盘旋点图标出现，自动返回普通浏览模式
5. 右键可取消放置

#### 放置无人机 🛩️
1. **先选择或创建一个任务**
2. 选择无人机颜色（下拉框）
3. 点击 **"放置无人机"** 按钮
4. 在地图上点击任意位置
5. 无人机图标出现，自动返回普通浏览模式
6. 右键可取消放置

#### 绘制禁飞区域 🚫
1. **先选择或创建一个任务**
2. 点击 **"放置禁飞区域"** 按钮
3. 第一次点击设置中心点
4. 移动鼠标（红色预览圆跟随）
5. 第二次点击确定半径
6. 弹出对话框选择地形特征（默认：平原）
7. 自动返回普通浏览模式
8. 右键可取消操作

#### 绘制多边形区域 🔷
1. **先选择或创建一个任务**
2. 点击 **"绘制多边形"** 按钮
3. 依次点击添加顶点（至少3个）
4. **点击起点附近或按 ESC 完成绘制**（阈值随缩放自动调整）
5. 弹出对话框选择地形特征（默认：平原）
6. 右键回退上一个顶点
7. 如果没有顶点，右键取消绘制

**智能闭合阈值**：根据缩放级别自动调整
- 缩放级别 8（大视角）：约 800 米
- 缩放级别 12（中等）：约 50 米
- 缩放级别 16（近距离）：约 3 米

#### 查看元素详情 💬

在普通浏览模式下，点击已放置的元素查看详细信息：

**显示信息**：
- **任务ID** 和 **任务名称**
- **盘旋点**：经度、纬度
- **无人机**：经度、纬度、颜色
- **禁飞区域**：中心点经纬度、半径、区域面积、地形特征（可编辑）
- **多边形区域**：所有顶点坐标、区域面积、地形特征（可编辑）

**操作按钮**：
- **地形下拉框**（仅区域元素）：内联修改地形特征
- **删除按钮**：删除该元素（所有元素类型均支持）

点击空白区域可关闭详情窗口。

#### 清除标注
点击 **"清除当前任务标记"** 按钮删除当前任务的所有标注

## 核心特性

### 任务管理
- ✅ 多任务支持（创建/编辑/删除）
- ✅ 任务切换与可见性控制
- ✅ 任务持久化（内存中，未实现数据库存储）
- ✅ 标记与任务关联

### 地图交互
- ✅ 单次放置模式（自动返回普通浏览）
- ✅ 多种元素类型（盘旋点、无人机、禁飞区、多边形）
- ✅ 交互式元素详情查看
- ✅ 实时预览（禁飞区圆形预览、多边形线段预览）
- ✅ 右键取消/回退操作
- ✅ 鼠标坐标实时显示（右下角）
- ✅ **智能交互阈值**（基于缩放级别，公式：`threshold = baseThreshold * 2^(12 - zoom)`）
- ✅ 点击与拖动智能区分

### 地形特征
- ✅ 四种地形类型（平原、丘陵、山地、高山地）
- ✅ 创建时选择地形（对话框，默认平原）
- ✅ 详情窗口内联编辑地形
- ✅ 实时保存地形修改

### 元素操作
- ✅ 所有元素类型支持删除（盘旋点、无人机、禁飞区、多边形）
- ✅ 删除前确认对话框
- ✅ 区域元素支持地形编辑
- ✅ 图标自动缩放和染色
- ✅ Haversine 公式精确距离计算
- ✅ 射线法判断点在多边形内

### UI/UX 优化
- ✅ 简洁的详情窗口样式（仅标题有蓝色边框）
- ✅ 任务验证（操作前检查是否选择任务）
- ✅ 操作模式提示（左下角）
- ✅ 清晰的按钮文案（"清除当前任务标记"）

## 项目结构

```
drawing-demo/
├── MapPainter.h/cpp             # 地图画家类 - 绘制各种地图元素
├── InteractiveMapWidget.h/cpp   # 交互式地图组件 - 处理鼠标交互
├── ElementDetailWidget.h/cpp    # 元素详情浮动窗口 - 显示/编辑元素信息
├── Task.h                       # 任务数据结构 - MapElement、TerrainType
├── TaskManager.h/cpp            # 任务管理器 - 任务 CRUD、可见性控制
├── TaskListWidget.h/cpp         # 任务列表 UI - 左侧面板
├── TaskDialog.h/cpp             # 任务创建/编辑对话框
├── RegionFeatureDialog.h/cpp    # 地形特征选择对话框
├── main.cpp                     # 主程序入口 - TestPainterWindow
├── image/
│   ├── pin.png                  # 盘旋点图标
│   └── uav.png                  # 无人机图标（支持染色）
├── CMakeLists.txt               # CMake 构建配置
└── README.md                    # 项目文档（本文件）
```

### 核心类说明

#### 1. Task & TaskManager - 任务管理系统

**Task 类**：
- 存储任务信息（ID、名称、描述、可见性）
- 管理该任务下的所有地图元素（`QVector<MapElement>`）
- 提供元素增删改查接口

**TaskManager 类**：
- 管理所有任务的生命周期
- 当前任务切换
- 查找可见元素（考虑任务可见性）
- 元素删除（支持跨任务查找）

**MapElement 结构**：
```cpp
struct MapElement {
    enum Type { LoiterPoint, UAV, NoFlyZone, Polygon };
    enum TerrainType { Plain, Hills, Mountain, HighMountain };

    Type type;
    QMapLibre::AnnotationID annotationId;
    QMapLibre::Coordinate coordinate;
    QMapLibre::Coordinates vertices;
    double radius;
    QString color;
    TerrainType terrainType;  // 地形特征
};
```

#### 2. MapPainter - 地图绘制引擎

负责在地图上绘制各种元素，并管理元素信息。

**主要功能**：
- 绘制盘旋点、无人机、禁飞区域、多边形区域
- 图标加载和染色（支持多种颜色）
- 元素信息存储和查询
- 圆形坐标生成（Haversine 公式）
- 元素点击检测（距离计算、点在多边形内判断）

**关键方法**：
```cpp
// 绘制方法
QMapLibre::AnnotationID drawLoiterPoint(double lat, double lon);
QMapLibre::AnnotationID drawUAV(double lat, double lon, const QString &color);
QMapLibre::AnnotationID drawNoFlyZone(double lat, double lon, double radius);
QMapLibre::AnnotationID drawPolygonArea(const QMapLibre::Coordinates &coords);

// 预览方法
QMapLibre::AnnotationID drawPreviewNoFlyZone(double lat, double lon, double radius);
QMapLibre::AnnotationID drawPreviewLines(const QMapLibre::Coordinates &coords);
void clearPreview();

// 元素查询
const ElementInfo* findElementNear(const QMapLibre::Coordinate &coord, double threshold);
```

#### 3. InteractiveMapWidget - 交互控制器

包装 QMapLibre 地图组件，处理鼠标交互事件。

**主要功能**：
- 鼠标点击与拖动智能区分（5像素阈值）
- 实时坐标显示（右下角）
- 操作模式提示（左下角）
- 鼠标移动事件处理

**信号**：
```cpp
void mapClicked(const QMapLibre::Coordinate &coord);      // 地图被点击
void mapMouseMoved(const QMapLibre::Coordinate &coord);   // 鼠标移动
void mapRightClicked();                                   // 右键点击
```

#### 4. ElementDetailWidget - 详情展示

浮动窗口，显示点击元素的详细信息。

**主要功能**：
- 自适应大小和位置
- 简洁的信息展示（标题有蓝色边框，内容为普通文字）
- 支持多种元素类型
- 自动计算面积（禁飞区、多边形）
- 内联地形编辑（QComboBox 下拉框）
- 删除按钮（所有元素类型）

**信号**：
```cpp
void terrainChanged(QMapLibre::AnnotationID, MapElement::TerrainType);
void deleteRequested(QMapLibre::AnnotationID);
```

#### 5. TestPainterWindow（main.cpp）- 主窗口

应用程序主窗口，集成所有组件。

**主要功能**：
- 模式切换管理（普通、盘旋点、无人机、禁飞区、多边形）
- UI 布局（左侧任务面板、右侧地图、浮动按钮）
- 交互流程控制
- 元素详情显示触发
- **智能交互阈值计算**（`getZoomDependentThreshold()`）
- **任务验证**（操作前检查）

## 实现细节

### 任务与元素关联机制

每个 `MapElement` 存储在对应的 `Task` 中：
- `Task` 包含 `QVector<MapElement>`
- `TaskManager` 管理所有 `Task` 对象
- 查找元素时遍历可见任务的元素列表
- 删除元素时通过 `annotationId` 跨任务查找并移除

### 智能交互阈值算法

根据地图缩放级别动态调整交互距离：

```cpp
double getZoomDependentThreshold(double baseThreshold = 50.0) {
    double zoom = map->zoom();
    double threshold = baseThreshold * pow(2.0, 12.0 - zoom);
    return threshold;
}
```

**效果**：
- 缩放级别 8（远视角）：threshold ≈ 800m（容易点中）
- 缩放级别 12（中等）：threshold = 50m（正常）
- 缩放级别 16（近距离）：threshold ≈ 3m（精确）

**应用场景**：
- 多边形闭合判断（50m 基准）
- 元素点击检测（100m 基准）

### 点击检测算法

1. **点元素**（盘旋点、无人机）：计算点击位置到元素的 Haversine 距离
2. **圆形元素**（禁飞区）：
   - 点在圆内：距离为 0
   - 点在圆外：距离 = 点到圆心距离 - 半径
3. **多边形元素**：
   - 使用射线法判断点是否在多边形内部
   - 在内部：距离为 0
   - 在外部：计算到所有顶点的最小距离

### 点击与拖动区分

- 记录 `MouseButtonPress` 时的位置
- 在 `MouseButtonRelease` 时计算移动距离
- 移动距离 < 5 像素：判定为点击
- 移动距离 ≥ 5 像素：判定为拖动（不触发点击事件）

### 图标染色技术

无人机图标支持实时染色：
1. 加载原始 PNG 图标
2. 遍历每个像素
3. 保留原始 alpha 通道（透明度）
4. 替换 RGB 为目标颜色
5. 缓存已染色的图标避免重复处理

### 地形特征选择流程

1. 用户完成区域绘制（禁飞区或多边形）
2. 弹出 `RegionFeatureDialog` 对话框
3. 选择地形类型（默认平原）
4. 将地形信息存储到 `MapElement.terrainType`
5. 点击元素详情时显示当前地形
6. 通过下拉框修改时触发 `terrainChanged` 信号
7. 主窗口接收信号，更新元素的 `terrainType` 字段

## 技术栈

- **Qt 6.7.3** - UI 框架
  - QtCore, QtGui, QtWidgets
- **QMapLibre 3.0.0** - 地图渲染引擎
  - OpenGL 加速
  - 支持 OSM 瓦片
- **C++17** - 编程语言
- **CMake 3.19+** - 构建系统

## 常见问题 (FAQ)

### Q1: 运行时提示找不到共享库

**症状**：
```
error while loading shared libraries: libQMapLibreWidgets.so.3: cannot open shared object file
```

**解决方案**：
```bash
# 临时方案（当前终端有效）
export LD_LIBRARY_PATH="$HOME/Qt6/6.7.3/gcc_64/lib:$HOME/projects/maplibre-native-qt/install/lib:$LD_LIBRARY_PATH"
./build/drawing-demo

# 永久方案（添加到 ~/.bashrc）
echo 'export LD_LIBRARY_PATH="$HOME/Qt6/6.7.3/gcc_64/lib:$HOME/projects/maplibre-native-qt/install/lib:$LD_LIBRARY_PATH"' >> ~/.bashrc
source ~/.bashrc
```

### Q2: CMake 找不到 Qt6

**症状**：
```
CMake Error at CMakeLists.txt:16 (find_package):
  Could not find a package configuration file provided by "Qt6"
```

**解决方案**：
```bash
# 确保 Qt6 已安装
ls ~/Qt6/6.7.3/gcc_64  # 应该能看到 bin, lib 等目录

# 设置 CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH="$HOME/Qt6/6.7.3/gcc_64:$CMAKE_PREFIX_PATH"

# 重新运行 cmake
cd build
cmake ..
```

### Q3: CMake 找不到 QMapLibre

**症状**：
```
CMake Error at CMakeLists.txt:25 (find_package):
  Could not find a package configuration file provided by "QMapLibre"
```

**解决方案**：
```bash
# 检查 QMapLibre 是否安装成功
ls ~/projects/maplibre-native-qt/install/lib/cmake/QMapLibre

# 如果不存在，重新编译安装 QMapLibre
cd ~/projects/maplibre-native-qt/build
ninja install

# 确保 CMakeLists.txt 中的路径正确（已配置）
# CMAKE_PREFIX_PATH 包含 "$ENV{HOME}/projects/maplibre-native-qt/install"
```

### Q4: 编译时提示缺少头文件

**症状**：
```
fatal error: QMapLibre/Map.hpp: No such file or directory
```

**解决方案**：
确保 QMapLibre 已正确安装，并且 `CMAKE_PREFIX_PATH` 包含安装路径。

### Q5: Qt 在线安装器下载速度慢

**解决方案**：
```bash
# 使用镜像源（中国大陆用户推荐）
# 编辑安装器配置，添加镜像 URL：
# https://mirrors.tuna.tsinghua.edu.cn/qt/online/qtsdkrepository/

# 或使用离线安装包（访问 Qt 官网下载对应版本）
```

### Q6: 如何卸载重装

**Qt6**：
```bash
# 删除 Qt 安装目录
rm -rf ~/Qt6

# 删除配置文件（可选）
rm -rf ~/.config/QtProject
```

**QMapLibre**：
```bash
cd ~/projects/maplibre-native-qt
rm -rf build install
```

**本项目**：
```bash
cd ~/projects/drawing-demo
rm -rf build
```

## 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发环境设置

1. Fork 本仓库
2. 克隆你的 fork：`git clone <your-fork-url>`
3. 创建特性分支：`git checkout -b feature/your-feature`
4. 提交更改：`git commit -m "feat: add some feature"`
5. 推送到分支：`git push origin feature/your-feature`
6. 创建 Pull Request

### 代码风格

- 使用 C++17 标准
- 遵循 Qt 代码风格（驼峰命名、m_ 前缀成员变量）
- 保持代码简洁和可读性
- 添加必要的注释

## 版本历史

**当前版本**: v5.0 - 任务管理与地形特征系统

主要版本：
- **v5.0** (2025-10-09) - 任务管理系统、地形特征、智能交互阈值、UI 优化
- **v4.0** (2025-10-08) - 元素详情查看、多边形支持、UI 优化
- **v3.0** (2025-10-08) - 单次放置模式，自动返回普通浏览
- **v2.2** (2025-10-08) - 修复预览圆 + 坐标显示
- **v2.1** (2025-10-08) - 两次点击交互
- **v2.0** (2025-10-08) - 交互式界面
- **v1.0** (2025-10-08) - 基础版本

## 许可证

Copyright (C) 2023 MapLibre contributors
SPDX-License-Identifier: MIT

---

**作者**: Drawing Demo Team
**联系方式**: <your-email@example.com>
**项目地址**: <https://github.com/your-username/drawing-demo>
