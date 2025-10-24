# 高德地图瓦片下载工具

## 功能说明

这个工具可以下载指定经纬度范围和缩放级别的高德地图瓦片到本地，用于离线地图应用。

**重要说明**：
- 主程序默认使用**在线地图**
- 瓦片下载工具用于**提前缓存数据**或**完全离线场景**
- 如需切换到纯离线模式，需手动修改代码（见下方说明）

## 编译

```bash
cd tools
cmake -B build -DCMAKE_PREFIX_PATH=/home/user00/Qt6/6.7.3/gcc_64
cmake --build build
```

编译成功后，可执行文件位于 `tools/build/tile_downloader`

## 快速开始

### 1. 下载北京地区瓦片（示例）

```bash
cd /home/user00/projects/drawing-demo/tools

# 下载北京中心区域，缩放级别 10-14
./build/tile_downloader \
  --min-lat 39.85 \
  --max-lat 39.95 \
  --min-lon 116.35 \
  --max-lon 116.45 \
  --min-zoom 10 \
  --max-zoom 14
```

瓦片会自动保存到 `../offline_tiles` 目录（约需下载几千个瓦片，数百MB）

### 2. 运行主程序

```bash
cd /home/user00/projects/drawing-demo
./build/drawing-demo
```

程序会自动检测到离线瓦片并使用！

## 使用方法

### 基本用法

```bash
./build/tile_downloader \
  --min-lat 39.7 \
  --max-lat 40.1 \
  --min-lon 116.2 \
  --max-lon 116.6 \
  --min-zoom 10 \
  --max-zoom 14 \
  --output ../offline_tiles
```

**注意**：如果不指定 `--output`，默认保存到 `../offline_tiles`

### 参数说明

- `--min-lat`: 最小纬度（南边界）
- `--max-lat`: 最大纬度（北边界）
- `--min-lon`: 最小经度（西边界）
- `--max-lon`: 最大经度（东边界）
- `--min-zoom`: 最小缩放级别（建议 3-10）
- `--max-zoom`: 最大缩放级别（建议 14-18）
- `--output`: 输出目录（默认 ./tiles）

### 示例

#### 下载北京市中心区域

```bash
./build/tile_downloader \
  --min-lat 39.85 \
  --max-lat 39.95 \
  --min-lon 116.35 \
  --max-lon 116.45 \
  --min-zoom 12 \
  --max-zoom 16 \
  --output ~/maps/beijing
```

#### 下载小范围高精度地图

```bash
./build/tile_downloader \
  --min-lat 39.90 \
  --max-lat 39.91 \
  --min-lon 116.40 \
  --max-lon 116.41 \
  --min-zoom 14 \
  --max-zoom 18 \
  --output ~/maps/test
```

## 瓦片数量估算

不同缩放级别下，1度x1度区域的瓦片数量：

| 缩放级别 | 瓦片数 (1°x1°) |
|---------|---------------|
| 3       | ~16           |
| 10      | ~16,384       |
| 14      | ~262,144      |
| 18      | ~67,108,864   |

**建议**：
- 测试时先用小范围 + 低缩放级别（如 0.1°x0.1°, zoom 10-12）
- 正式下载前先估算瓦片数量，避免下载过多数据
- 每个缩放级别增加1，瓦片数量约增加4倍

## 存储空间估算

- 每个瓦片约 10-30 KB
- 1万个瓦片约需 100-300 MB
- 10万个瓦片约需 1-3 GB

## 切换到纯离线模式

**前提**：必须先下载完整覆盖你使用区域的瓦片。

修改 `task/TaskUI.cpp` 的 `setupMap()` 函数：

```cpp
void TaskUI::setupMap() {
    // 修改为离线模式
    QString offlineTilesPath = QCoreApplication::applicationDirPath() + "/../offline_tiles";
    QString absolutePath = QDir(offlineTilesPath).absolutePath();
    QString offlineSource = QString("file://%1/{z}/{x}/{y}.png").arg(absolutePath);

    qDebug() << "地图模式：离线";

    QString amapStyle = QString(R"({
        "version": 8,
        "name": "AMap Offline",
        "sources": {
            "amap": {
                "type": "raster",
                "tiles": ["%1"],
                "tileSize": 256,
            "minzoom": 3,
            "maxzoom": 18
        }
    },
    "layers": [{
        "id": "amap",
        "type": "raster",
        "source": "amap",
        "minzoom": 3,
        "maxzoom": 18
    }]
})";
```

**注意**：将 `file:///home/user00/maps/beijing` 替换为你的实际瓦片目录路径。

## 进阶：搭建本地瓦片服务器

如果需要更灵活的离线地图方案，可以使用 HTTP 服务器：

### 使用 Python 简易服务器

```bash
cd ~/maps/beijing
python3 -m http.server 8080
```

然后修改地图配置：

```cpp
"tiles": ["http://localhost:8080/{z}/{x}/{y}.png"]
```

### 使用 Nginx

```nginx
server {
    listen 8080;
    server_name localhost;

    location / {
        root /home/user00/maps/beijing;
        add_header Access-Control-Allow-Origin *;
    }
}
```

## 注意事项

1. **下载速度**：默认每100ms下载一个瓦片，避免请求过快被服务器限制
2. **断点续传**：重新运行下载命令会跳过已存在的瓦片
3. **存储结构**：瓦片按 `{zoom}/{x}/{y}.png` 的目录结构存储
4. **网络要求**：需要联网才能下载瓦片

## 常见问题

### Q: 下载速度慢怎么办？

A: 可以修改 `TileDownloader.cpp` 中的下载间隔：
```cpp
m_timer->setInterval(50);  // 改为50ms（更快，但可能被限速）
```

### Q: 如何查看已下载的瓦片？

A: 瓦片是 PNG 图片，可以直接用图片查看器打开查看。

### Q: 下载中断了怎么办？

A: 重新运行相同的命令，程序会自动跳过已下载的瓦片。

### Q: 如何计算我需要的经纬度范围？

A: 可以在主程序中查看地图右下角的坐标显示，移动地图到目标区域记录经纬度。
