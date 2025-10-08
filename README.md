# MapLibre 地图标注工具

这是一个基于 QMapLibre 的交互式地图标注工具，支持多种地图元素的绘制和交互查看。

## ✨ 主要功能

1. **放置盘旋点** 📍 - 使用自定义图标标记地图位置
2. **放置无人机** 🛩️ - 支持多种颜色的无人机图标（黑、红、蓝、紫、绿、黄）
3. **绘制禁飞区域** 🚫 - 两次点击交互绘制圆形禁飞区
4. **绘制多边形区域** 🔷 - 多点绘制任意多边形区域
5. **元素详情查看** 💬 - 点击已放置的元素查看详细信息
6. **实时坐标显示** 📊 - 右下角显示鼠标经纬度
7. **单次放置模式** 🎯 - 完成放置后自动返回普通浏览

## 快速开始

### 构建

```bash
cd /home/user00/projects/build-example/drawing-demo
mkdir -p build && cd build

# 设置 QMapLibre 安装路径
export CMAKE_PREFIX_PATH="/home/user00/projects/maplibre-native-qt/install"

# 配置和编译
cmake ..
cmake --build .
```

### 运行

```bash
# 设置运行时库路径
export LD_LIBRARY_PATH="/home/user00/projects/maplibre-native-qt/install/lib:/home/user00/Qt6/6.7.3/gcc_64/lib:$LD_LIBRARY_PATH"

# 运行程序
./build/drawing-demo
```

## 使用说明

### 默认模式：普通浏览
- **左键点击元素**：查看元素详细信息
- **左键拖动**：移动地图
- **滚轮缩放**：放大/缩小
- **右下角**：实时显示鼠标经纬度
- **左下角**：显示当前操作模式提示

### 放置盘旋点 📍
1. 点击 **"放置盘旋点"** 按钮
2. 在地图上点击任意位置
3. 盘旋点图标出现，自动返回普通浏览模式
4. 右键可取消放置

### 放置无人机 🛩️
1. 选择无人机颜色（下拉框）
2. 点击 **"放置无人机"** 按钮
3. 在地图上点击任意位置
4. 无人机图标出现，自动返回普通浏览模式
5. 右键可取消放置

### 绘制禁飞区域 🚫
1. 点击 **"放置禁飞区域"** 按钮
2. 第一次点击设置中心点
3. 移动鼠标（红色预览圆跟随）
4. 第二次点击确定半径
5. 自动返回普通浏览模式
6. 右键可取消操作

### 绘制多边形区域 🔷
1. 点击 **"绘制多边形"** 按钮
2. 依次点击添加顶点（至少3个）
3. 点击起点或按 ESC 完成绘制
4. 右键回退上一个顶点
5. 如果没有顶点，右键取消绘制

### 查看元素详情 💬
在普通浏览模式下，点击已放置的元素即可查看详细信息：
- **盘旋点**：经度、纬度
- **无人机**：经度、纬度、颜色
- **禁飞区域**：中心点经纬度、半径、区域面积
- **多边形区域**：所有顶点坐标、区域面积

点击空白区域可关闭详情窗口。

### 清除标注
点击 **"清除全部"** 按钮删除所有标注

## 核心特性

- ✅ 单次放置模式（自动返回普通浏览）
- ✅ 多种元素类型（盘旋点、无人机、禁飞区、多边形）
- ✅ 交互式元素详情查看
- ✅ 实时预览（禁飞区圆形预览、多边形线段预览）
- ✅ 右键取消/回退操作
- ✅ 鼠标坐标实时显示（右下角）
- ✅ 图标自动缩放和染色
- ✅ Haversine 公式精确距离计算
- ✅ 射线法判断点在多边形内
- ✅ 点击与拖动智能区分

## 项目结构

```
drawing-demo/
├── MapPainter.h                 # 地图画家类 - 负责绘制各种地图元素
├── MapPainter.cpp               #   - 支持盘旋点、无人机、禁飞区、多边形
├── InteractiveMapWidget.h       # 交互式地图组件 - 处理鼠标交互
├── InteractiveMapWidget.cpp     #   - 点击/拖动区分、坐标显示
├── ElementDetailWidget.h        # 元素详情浮动窗口 - 显示元素信息
├── main.cpp                     # 主程序入口
├── image/
│   ├── pin.png                  # 盘旋点图标
│   └── uav.png                  # 无人机图标（支持染色）
├── CMakeLists.txt               # CMake 构建配置
├── README.md                    # 项目文档（本文件）
└── CHANGELOG.md                 # 版本变更历史
```

### 核心类说明

#### 1. MapPainter - 地图绘制引擎
负责在地图上绘制各种元素，并管理元素信息。

**主要功能：**
- 绘制盘旋点、无人机、禁飞区域、多边形区域
- 图标加载和染色（支持多种颜色）
- 元素信息存储和查询
- 圆形坐标生成（Haversine 公式）
- 元素点击检测（距离计算、点在多边形内判断）

**关键方法：**
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

#### 2. InteractiveMapWidget - 交互控制器
包装 QMapLibre 地图组件，处理鼠标交互事件。

**主要功能：**
- 鼠标点击与拖动智能区分（5像素阈值）
- 实时坐标显示（右下角）
- 操作模式提示（左下角）
- 鼠标移动事件处理

**信号：**
```cpp
void mapClicked(const QMapLibre::Coordinate &coord);      // 地图被点击
void mapMouseMoved(const QMapLibre::Coordinate &coord);   // 鼠标移动
void mapRightClicked();                                   // 右键点击
```

#### 3. ElementDetailWidget - 详情展示
浮动窗口，显示点击元素的详细信息。

**主要功能：**
- 自适应大小和位置
- 紧凑的信息展示（键值对格式）
- 支持多种元素类型
- 自动计算面积（禁飞区、多边形）

#### 4. TestPainterWindow（main.cpp）- 主窗口
应用程序主窗口，集成所有组件。

**主要功能：**
- 模式切换管理（普通、盘旋点、无人机、禁飞区、多边形）
- UI 布局（浮动按钮、地图容器）
- 交互流程控制
- 元素详情显示触发

## 实现细节

### 元素存储机制
每个绘制的元素都有唯一的 `AnnotationID`，通过 `QMap<AnnotationID, ElementInfo>` 存储元素信息：
```cpp
struct ElementInfo {
    ElementType type;                    // 元素类型
    QMapLibre::Coordinate coordinate;    // 位置/中心点
    QMapLibre::Coordinates vertices;     // 多边形顶点
    double radius;                       // 禁飞区半径
    QString color;                       // 无人机颜色
};
```

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

## 技术栈

- **Qt 6.7.3** - UI 框架
  - QtCore, QtGui, QtWidgets
- **QMapLibre 3.0.0** - 地图渲染引擎
  - OpenGL 加速
  - 支持 OSM 瓦片
- **C++17** - 编程语言
- **CMake 3.19+** - 构建系统

## 版本历史

**当前版本**: v4.0 - 完整交互系统

主要版本：
- **v4.0** (2025-10-08) - 元素详情查看、多边形支持、UI 优化
- **v3.0** (2025-10-08) - 单次放置模式，自动返回普通浏览
- **v2.2** (2025-10-08) - 修复预览圆 + 坐标显示
- **v2.1** (2025-10-08) - 两次点击交互
- **v2.0** (2025-10-08) - 交互式界面
- **v1.0** (2025-10-08) - 基础版本

## 依赖项

- Qt 6.7.3 (Core, Gui, Widgets)
- QMapLibre 3.0.0
- CMake 3.19+
- C++17

## 许可证

Copyright (C) 2023 MapLibre contributors
SPDX-License-Identifier: MIT
