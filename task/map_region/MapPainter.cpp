// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "MapPainter.h"
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QtMath>

MapPainter::MapPainter(QMapLibre::Map *map, QObject *parent)
    : QObject(parent)
    , m_map(map)
    , m_loiterIconPath("image/pin.png")
    , m_iconLoaded(false)
    , m_previewAnnotationId(0)
    , m_taskRegionPreviewLineId(0)
    , m_dynamicLineId(0)
{
}

bool MapPainter::setLoiterIconPath(const QString &iconPath)
{
    m_loiterIconPath = iconPath;
    m_iconLoaded = false;
    return loadLoiterIcon();
}

bool MapPainter::loadLoiterIcon()
{
    if (m_iconLoaded) {
        return true;
    }

    QImage icon(m_loiterIconPath);
    if (icon.isNull()) {
        qWarning() << "无法加载盘旋点图标:" << m_loiterIconPath;
        return false;
    }

    // 缩放图标到合适大小（32x32 像素）
    if (icon.width() > 32 || icon.height() > 32) {
        icon = icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << "图标已缩放至 32x32 像素";
    }

    // 为盘旋点图标创建更大的画布，使图标内容居上，底部中心对齐地理位置
    // 创建 32x48 的画布（增加 16 像素高度），图标内容在上半部分
    QImage anchoredIcon(32, 48, QImage::Format_ARGB32);
    anchoredIcon.fill(Qt::transparent);

    // 将原图标绘制到画布顶部居中位置
    QPainter painter(&anchoredIcon);
    painter.setRenderHint(QPainter::Antialiasing);
    int xOffset = (32 - icon.width()) / 2;
    painter.drawImage(xOffset, 0, icon);
    painter.end();

    m_map->addAnnotationIcon(LOITER_ICON_NAME, anchoredIcon);
    m_iconLoaded = true;
    qDebug() << "成功加载盘旋点图标:" << m_loiterIconPath << "最终尺寸:" << anchoredIcon.size();
    return true;
}

QMapLibre::AnnotationID MapPainter::drawLoiterPoint(double latitude, double longitude)
{
    // 确保图标已加载
    if (!loadLoiterIcon()) {
        qWarning() << "无法绘制盘旋点：图标加载失败";
        return 0;
    }

    // 创建符号标注
    QMapLibre::SymbolAnnotation marker;
    marker.geometry = QMapLibre::Coordinate(latitude, longitude);
    marker.icon = LOITER_ICON_NAME;

    // 添加到地图
    QVariant annotation = QVariant::fromValue(marker);
    QMapLibre::AnnotationID id = m_map->addAnnotation(annotation);
    m_annotations.append(id);

    // 保存元素信息
    RegionInfo info;
    info.type = RegionType::LoiterPoint;
    info.coordinate = QMapLibre::Coordinate(latitude, longitude);
    info.annotationId = id;
    m_regionInfo[id] = info;

    qDebug() << QString("添加盘旋点: (%1, %2), ID: %3").arg(latitude).arg(longitude).arg(id);
    return id;
}

QMapLibre::AnnotationID MapPainter::drawUAV(double latitude, double longitude, const QString &color)
{
    // 确保该颜色的图标已加载
    if (!loadUAVIcon(color)) {
        qWarning() << "无法绘制无人机：图标加载失败";
        return 0;
    }

    // 创建符号标注
    QMapLibre::SymbolAnnotation marker;
    marker.geometry = QMapLibre::Coordinate(latitude, longitude);
    marker.icon = QString("uav-icon-%1").arg(color);

    // 添加到地图
    QVariant annotation = QVariant::fromValue(marker);
    QMapLibre::AnnotationID id = m_map->addAnnotation(annotation);
    m_annotations.append(id);

    // 保存元素信息
    RegionInfo info;
    info.type = RegionType::UAV;
    info.coordinate = QMapLibre::Coordinate(latitude, longitude);
    info.color = color;
    info.annotationId = id;
    m_regionInfo[id] = info;

    qDebug() << QString("添加无人机 (%1): (%2, %3), ID: %4").arg(color).arg(latitude).arg(longitude).arg(id);
    return id;
}

QMapLibre::AnnotationID MapPainter::drawNoFlyZone(double latitude, double longitude, double radiusInMeters)
{
    // 生成圆形坐标
    QMapLibre::Coordinates circleCoords = generateCircleCoordinates(latitude, longitude, radiusInMeters);

    // 创建填充标注（多边形）
    QMapLibre::FillAnnotation noFlyZone;
    noFlyZone.geometry.type = QMapLibre::ShapeAnnotationGeometry::PolygonType;

    QMapLibre::CoordinatesCollection polygonCoords;
    polygonCoords.append(circleCoords);
    noFlyZone.geometry.geometry.append(polygonCoords);

    // 设置样式：红色半透明
    noFlyZone.color = QColor(255, 0, 0, 100);        // 红色，透明度 ~40%
    noFlyZone.outlineColor = QColor(200, 0, 0, 200); // 深红色边框
    noFlyZone.opacity = 0.6f;

    // 添加区域到地图
    QVariant annotation = QVariant::fromValue(noFlyZone);
    QMapLibre::AnnotationID zoneId = m_map->addAnnotation(annotation);
    m_annotations.append(zoneId);

    // 保存元素信息
    RegionInfo info;
    info.type = RegionType::NoFlyZone;
    info.coordinate = QMapLibre::Coordinate(latitude, longitude);
    info.radius = radiusInMeters;
    info.annotationId = zoneId;
    m_regionInfo[zoneId] = info;

    qDebug() << QString("添加禁飞区域: 中心(%1, %2), 半径 %3m, ID: %4")
                    .arg(latitude).arg(longitude).arg(radiusInMeters).arg(zoneId);
    return zoneId;
}

void MapPainter::removeAnnotation(QMapLibre::AnnotationID id)
{
    qDebug() << "MapPainter::removeAnnotation 开始删除标注 ID:" << id;
    qDebug() << "  - 删除前 m_annotations 数量:" << m_annotations.size();
    qDebug() << "  - 删除前 m_regionInfo 数量:" << m_regionInfo.size();

    m_map->removeAnnotation(id);
    m_annotations.removeAll(id);
    m_regionInfo.remove(id);  // 清理元素信息

    qDebug() << "  - 删除后 m_annotations 数量:" << m_annotations.size();
    qDebug() << "  - 删除后 m_regionInfo 数量:" << m_regionInfo.size();
    qDebug() << "MapPainter::removeAnnotation 完成删除标注 ID:" << id;
}

void MapPainter::clearAll()
{
    clearPreview();

    // 清除所有标注
    for (auto id : m_annotations) {
        m_map->removeAnnotation(id);
    }
    m_annotations.clear();
    m_regionInfo.clear();  // 清理所有元素信息

    qDebug() << "清除所有画家标注";
}

QMapLibre::AnnotationID MapPainter::drawPreviewNoFlyZone(double latitude, double longitude, double radiusInMeters)
{
    // 先清除之前的预览
    clearPreview();

    // 生成圆形坐标
    QMapLibre::Coordinates circleCoords = generateCircleCoordinates(latitude, longitude, radiusInMeters);

    // 创建填充标注（多边形）- 使用不同的颜色表示预览状态
    QMapLibre::FillAnnotation previewZone;
    previewZone.geometry.type = QMapLibre::ShapeAnnotationGeometry::PolygonType;

    QMapLibre::CoordinatesCollection polygonCoords;
    polygonCoords.append(circleCoords);
    previewZone.geometry.geometry.append(polygonCoords);

    // 设置样式：蓝色半透明（预览状态）
    previewZone.color = QColor(0, 120, 255, 80);        // 蓝色，更透明
    previewZone.outlineColor = QColor(0, 80, 200, 180); // 蓝色边框
    previewZone.opacity = 0.5f;

    // 添加区域到地图
    QVariant annotation = QVariant::fromValue(previewZone);
    m_previewAnnotationId = m_map->addAnnotation(annotation);

    return m_previewAnnotationId;
}

QMapLibre::AnnotationID MapPainter::drawPreviewRectangle(const QMapLibre::Coordinates &coordinates)
{
    // 先清除之前的预览
    clearPreview();

    if (coordinates.size() < 4) {
        qWarning() << "矩形至少需要4个顶点";
        return 0;
    }

    // 创建填充标注（多边形）- 蓝色填充，无边框
    QMapLibre::FillAnnotation previewRect;
    previewRect.geometry.type = QMapLibre::ShapeAnnotationGeometry::PolygonType;

    // 确保矩形闭合
    QMapLibre::Coordinates closedCoords = coordinates;
    if (closedCoords.first() != closedCoords.last()) {
        closedCoords.append(closedCoords.first());
    }

    QMapLibre::CoordinatesCollection polygonCoords;
    polygonCoords.append(closedCoords);
    previewRect.geometry.geometry.append(polygonCoords);

    // 设置样式：蓝色填充，无边框（设置边框颜色为透明）
    previewRect.color = QColor(33, 150, 243, 100);      // 蓝色填充（Material Design Blue 500）
    previewRect.outlineColor = QColor(0, 0, 0, 0);      // 完全透明的边框（无边框）
    previewRect.opacity = 0.6f;

    // 添加区域到地图
    QVariant annotation = QVariant::fromValue(previewRect);
    m_previewAnnotationId = m_map->addAnnotation(annotation);

    return m_previewAnnotationId;
}

void MapPainter::clearPreview()
{
    if (m_previewAnnotationId != 0) {
        m_map->removeAnnotation(m_previewAnnotationId);
        m_previewAnnotationId = 0;
    }
}

QMapLibre::Coordinates MapPainter::generateCircleCoordinates(
    double centerLat,
    double centerLon,
    double radiusInMeters,
    int numPoints)
{
    QMapLibre::Coordinates coords;

    // 地球半径（米）
    const double EARTH_RADIUS = 6378137.0;

    // 将半径转换为度数（近似）
    double radiusInDegLat = (radiusInMeters / EARTH_RADIUS) * (180.0 / M_PI);
    double radiusInDegLon = radiusInDegLat / qCos(centerLat * M_PI / 180.0);

    // 生成圆周上的点
    for (int i = 0; i <= numPoints; ++i) {
        double angle = 2.0 * M_PI * i / numPoints;
        double lat = centerLat + radiusInDegLat * qSin(angle);
        double lon = centerLon + radiusInDegLon * qCos(angle);
        coords.append(QMapLibre::Coordinate(lat, lon));
    }

    return coords;
}

QMapLibre::AnnotationID MapPainter::drawTaskRegionArea(const QMapLibre::Coordinates &coordinates,
                                                       const QMapLibre::Coordinate &center,
                                                       double radius)
{
    if (coordinates.size() < 3) {
        qWarning() << "多边形至少需要3个点";
        return 0;
    }

    // 创建填充标注（多边形）
    QMapLibre::FillAnnotation polygon;
    polygon.geometry.type = QMapLibre::ShapeAnnotationGeometry::PolygonType;

    // 确保多边形闭合
    QMapLibre::Coordinates closedCoords = coordinates;
    if (closedCoords.first() != closedCoords.last()) {
        closedCoords.append(closedCoords.first());
    }

    QMapLibre::CoordinatesCollection polygonCoords;
    polygonCoords.append(closedCoords);
    polygon.geometry.geometry.append(polygonCoords);

    // 设置样式：蓝色半透明
    polygon.color = QColor(0, 120, 255, 100);        // 蓝色，半透明
    polygon.outlineColor = QColor(0, 80, 200, 200);  // 深蓝色边框
    polygon.opacity = 0.6f;

    // 添加到地图
    QVariant annotation = QVariant::fromValue(polygon);
    QMapLibre::AnnotationID id = m_map->addAnnotation(annotation);
    m_annotations.append(id);

    // 保存元素信息
    RegionInfo info;
    info.type = RegionType::TaskRegion;
    info.vertices = coordinates;
    info.coordinate = center;  // 保存圆心（圆形区域）或几何中心（多边形）
    info.radius = radius;      // 保存半径（圆形区域才有）
    info.annotationId = id;
    m_regionInfo[id] = info;

    if (radius > 0) {
        qDebug() << QString("添加圆形任务区域: 圆心(%1, %2), 半径 %3m, 顶点数 %4, ID: %5")
                        .arg(center.first, 0, 'f', 5)
                        .arg(center.second, 0, 'f', 5)
                        .arg(radius)
                        .arg(coordinates.size()).arg(id);
    } else {
        qDebug() << QString("添加多边形区域: %1个顶点, ID: %2")
                        .arg(coordinates.size()).arg(id);
    }
    return id;
}

QMapLibre::AnnotationID MapPainter::drawPreviewLines(const QMapLibre::Coordinates &coordinates)
{
    // 先清除之前的预览线
    clearTaskRegionPreview();

    if (coordinates.size() < 2) {
        return 0;
    }

    // 创建线段标注
    QMapLibre::LineAnnotation line;
    line.geometry.type = QMapLibre::ShapeAnnotationGeometry::LineStringType;

    QMapLibre::CoordinatesCollection lineCoords;
    lineCoords.append(coordinates);
    line.geometry.geometry.append(lineCoords);

    // 设置样式：黄色线条
    line.color = QColor(255, 200, 0);  // 黄色
    line.width = 3.0f;
    line.opacity = 0.9f;

    // 添加到地图
    QVariant annotation = QVariant::fromValue(line);
    m_taskRegionPreviewLineId = m_map->addAnnotation(annotation);

    return m_taskRegionPreviewLineId;
}

void MapPainter::clearTaskRegionPreview()
{
    if (m_taskRegionPreviewLineId != 0) {
        m_map->removeAnnotation(m_taskRegionPreviewLineId);
        m_taskRegionPreviewLineId = 0;
    }
}

QMapLibre::AnnotationID MapPainter::updateDynamicLine(const QMapLibre::Coordinate &fromCoord, const QMapLibre::Coordinate &toCoord)
{
    // 先清除旧的动态线
    clearDynamicLine();

    // 创建从上一个点到鼠标位置的线段
    QMapLibre::LineAnnotation line;
    line.geometry.type = QMapLibre::ShapeAnnotationGeometry::LineStringType;

    QMapLibre::Coordinates coords;
    coords.append(fromCoord);
    coords.append(toCoord);

    QMapLibre::CoordinatesCollection lineCoords;
    lineCoords.append(coords);
    line.geometry.geometry.append(lineCoords);

    // 橙色虚线样式（表示临时预览）
    line.color = QColor(255, 150, 0);
    line.width = 2.0f;
    line.opacity = 0.7f;

    QVariant annotation = QVariant::fromValue(line);
    m_dynamicLineId = m_map->addAnnotation(annotation);

    return m_dynamicLineId;
}

void MapPainter::clearDynamicLine()
{
    if (m_dynamicLineId != 0) {
        m_map->removeAnnotation(m_dynamicLineId);
        m_dynamicLineId = 0;
    }
}

bool MapPainter::loadUAVIcon(const QString &color)
{
    // 如果该颜色已经加载过，直接返回成功
    if (m_loadedUAVColors.contains(color)) {
        return true;
    }

    QImage icon(UAV_ICON_PATH);
    if (icon.isNull()) {
        qWarning() << "无法加载 UAV 图标:" << UAV_ICON_PATH;
        return false;
    }

    // 如果不是黑色，需要染色
    if (color != "black") {
        // 确定染色的颜色
        QColor tintColor;
        if (color == "red") {
            tintColor = QColor(255, 0, 0);
        } else if (color == "blue") {
            tintColor = QColor(0, 120, 255);
        } else if (color == "purple") {
            tintColor = QColor(160, 32, 240);
        } else if (color == "green") {
            tintColor = QColor(0, 200, 0);
        } else if (color == "yellow") {
            tintColor = QColor(255, 215, 0);
        } else {
            qWarning() << "不支持的颜色:" << color;
            return false;
        }

        // 转换图像格式用于染色
        icon = icon.convertToFormat(QImage::Format_ARGB32);

        // 将整个图标染成目标颜色（保留透明度）
        for (int y = 0; y < icon.height(); ++y) {
            for (int x = 0; x < icon.width(); ++x) {
                QColor pixelColor = icon.pixelColor(x, y);
                if (pixelColor.alpha() > 0) {
                    // 直接使用目标颜色，只保留原始透明度
                    QColor newColor(
                        tintColor.red(),
                        tintColor.green(),
                        tintColor.blue(),
                        pixelColor.alpha()
                    );
                    icon.setPixelColor(x, y, newColor);
                }
            }
        }
    }

    // 缩放图标到合适大小（32x32 像素，与盘旋点一致）
    if (icon.width() > 32 || icon.height() > 32) {
        icon = icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qDebug() << QString("UAV 图标 (%1) 已缩放至 32x32 像素").arg(color);
    }

    // 使用正确的 API：addAnnotationIcon
    QString iconName = QString("uav-icon-%1").arg(color);
    m_map->addAnnotationIcon(iconName, icon);
    m_loadedUAVColors.insert(color);

    qDebug() << QString("成功加载 UAV 图标 (%1): %2 尺寸: %3x%4")
                    .arg(color).arg(UAV_ICON_PATH)
                    .arg(icon.width()).arg(icon.height());
    return true;
}

// 计算两点之间的距离（米）
static double calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    const double EARTH_RADIUS = 6378137.0; // 地球半径（米）

    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;

    double a = qSin(dLat / 2) * qSin(dLat / 2) +
               qCos(lat1 * M_PI / 180.0) * qCos(lat2 * M_PI / 180.0) *
               qSin(dLon / 2) * qSin(dLon / 2);

    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    return EARTH_RADIUS * c;
}

// 使用射线法判断点是否在多边形内部
static bool isPointInPolygon(const QMapLibre::Coordinate &point, const QMapLibre::Coordinates &vertices)
{
    if (vertices.size() < 3) {
        return false;
    }

    int crossings = 0;
    for (int i = 0; i < vertices.size(); ++i) {
        int j = (i + 1) % vertices.size();

        double lat1 = vertices[i].first;
        double lon1 = vertices[i].second;
        double lat2 = vertices[j].first;
        double lon2 = vertices[j].second;

        // 检查射线是否与边相交
        if (((lat1 <= point.first && point.first < lat2) ||
             (lat2 <= point.first && point.first < lat1)) &&
            (point.second < (lon2 - lon1) * (point.first - lat1) / (lat2 - lat1) + lon1)) {
            crossings++;
        }
    }

    return (crossings % 2) == 1;
}

// 计算点到多边形的最小距离（米）
static double distanceToPolygon(const QMapLibre::Coordinate &point, const QMapLibre::Coordinates &vertices)
{
    if (vertices.isEmpty()) {
        return std::numeric_limits<double>::max();
    }

    // 先检查点是否在多边形内部
    if (isPointInPolygon(point, vertices)) {
        return 0.0;  // 在多边形内部，距离为0
    }

    // 不在内部，计算到边界的最小距离
    double minDistance = std::numeric_limits<double>::max();

    // 计算点到每条边的距离
    for (int i = 0; i < vertices.size(); ++i) {
        int nextIdx = (i + 1) % vertices.size();
        const auto &v1 = vertices[i];
        const auto &v2 = vertices[nextIdx];

        // 计算点到顶点的距离
        double dist1 = calculateDistance(point.first, point.second, v1.first, v1.second);
        double dist2 = calculateDistance(point.first, point.second, v2.first, v2.second);

        minDistance = qMin(minDistance, qMin(dist1, dist2));
    }

    return minDistance;
}

const RegionInfo* MapPainter::findRegionNear(const QMapLibre::Coordinate &clickCoord, double threshold) const
{
    const RegionInfo* nearestPointElement = nullptr;   // 最近的点状元素（UAV/盘旋点）
    const RegionInfo* nearestAreaElement = nullptr;    // 最近的区域元素（多边形/禁飞区）
    double minPointDistance = threshold;
    double minAreaDistance = threshold;

    // 遍历所有元素，分别记录点状和区域元素
    for (auto it = m_regionInfo.constBegin(); it != m_regionInfo.constEnd(); ++it) {
        const RegionInfo &info = it.value();
        double distance = 0.0;

        switch (info.type) {
        case RegionType::LoiterPoint:
            // 盘旋点图标：底部中心是真实位置，需要调整检测范围
            // 图标 32x32 像素，约等于地图上 10-20 米的范围（取决于缩放级别）
            // 我们使用更宽松的阈值来检测
            distance = calculateDistance(clickCoord.first, clickCoord.second,
                                        info.coordinate.first, info.coordinate.second);
            if (distance < minPointDistance) {
                minPointDistance = distance;
                nearestPointElement = &info;
            }
            break;

        case RegionType::UAV:
            // 无人机图标：中心对齐
            distance = calculateDistance(clickCoord.first, clickCoord.second,
                                        info.coordinate.first, info.coordinate.second);
            if (distance < minPointDistance) {
                minPointDistance = distance;
                nearestPointElement = &info;
            }
            break;

        case RegionType::NoFlyZone:
            // 计算点到圆心的距离
            distance = calculateDistance(clickCoord.first, clickCoord.second,
                                        info.coordinate.first, info.coordinate.second);
            // 如果点在圆内，距离设为0
            if (distance <= info.radius) {
                distance = 0.0;
            } else {
                // 计算到圆边界的距离
                distance = distance - info.radius;
            }
            if (distance < minAreaDistance) {
                minAreaDistance = distance;
                nearestAreaElement = &info;
            }
            break;

        case RegionType::TaskRegion:
            // 计算点到多边形的距离
            distance = distanceToPolygon(clickCoord, info.vertices);
            if (distance < minAreaDistance) {
                minAreaDistance = distance;
                nearestAreaElement = &info;
            }
            break;
        }
    }

    // 优先返回点状元素（如果存在且在阈值内）
    if (nearestPointElement != nullptr) {
        return nearestPointElement;
    }

    // 否则返回区域元素
    return nearestAreaElement;
}

bool MapPainter::isInNoFlyZone(const QMapLibre::Coordinate &coord) const
{
    // 遍历所有元素，检查是否在禁飞区内
    for (auto it = m_regionInfo.constBegin(); it != m_regionInfo.constEnd(); ++it) {
        const RegionInfo &info = it.value();

        // 只检查禁飞区类型
        if (info.type == RegionType::NoFlyZone) {
            // 计算点到圆心的距离
            double distance = calculateDistance(coord.first, coord.second,
                                                info.coordinate.first, info.coordinate.second);

            // 如果距离小于半径，说明在禁飞区内
            if (distance <= info.radius) {
                return true;
            }
        }
    }

    return false;
}
