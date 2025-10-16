// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "RegionManager.h"
#include <QDebug>

RegionManager::RegionManager(MapPainter *painter, QObject *parent)
    : QObject(parent)
    , m_painter(painter)
    , m_nextId(1)
{
    if (!m_painter) {
        qWarning() << "RegionManager: painter is null!";
    }
}

RegionManager::~RegionManager() {
    // 清理所有区域
    qDeleteAll(m_regions);
    m_regions.clear();
}

// ==================== 创建区域 ====================

Region* RegionManager::createLoiterPoint(double lat, double lon, const QString &name) {
    if (!m_painter) {
        qWarning() << "RegionManager::createLoiterPoint: painter is null!";
        return nullptr;
    }

    int regionId = generateNextId();
    Region *region = new Region(regionId, RegionType::LoiterPoint);
    region->setName(name.isEmpty() ? QString("盘旋点 %1").arg(regionId) : name);
    region->setCoordinate(QMapLibre::Coordinate(lat, lon));

    // 在地图上绘制
    drawRegion(region);

    // 存储
    m_regions.insert(regionId, region);

    emit regionCreated(regionId);
    qDebug() << "创建盘旋点: ID =" << regionId << ", 名称 =" << region->name();

    return region;
}

Region* RegionManager::createUAV(double lat, double lon, const QString &color, const QString &name) {
    if (!m_painter) {
        qWarning() << "RegionManager::createUAV: painter is null!";
        return nullptr;
    }

    int regionId = generateNextId();
    Region *region = new Region(regionId, RegionType::UAV);
    region->setName(name.isEmpty() ? QString("无人机 %1").arg(regionId) : name);
    region->setCoordinate(QMapLibre::Coordinate(lat, lon));
    region->setColor(color);

    // 在地图上绘制
    drawRegion(region);

    // 存储
    m_regions.insert(regionId, region);

    emit regionCreated(regionId);
    qDebug() << "创建无人机: ID =" << regionId << ", 颜色 =" << color;

    return region;
}

Region* RegionManager::createNoFlyZone(double lat, double lon, double radius, const QString &name) {
    if (!m_painter) {
        qWarning() << "RegionManager::createNoFlyZone: painter is null!";
        return nullptr;
    }

    int regionId = generateNextId();
    Region *region = new Region(regionId, RegionType::NoFlyZone);
    region->setName(name.isEmpty() ? QString("禁飞区 %1").arg(regionId) : name);
    region->setCoordinate(QMapLibre::Coordinate(lat, lon));
    region->setRadius(radius);
    region->setTerrainType(TerrainType::Plain);  // 默认平原

    // 在地图上绘制
    drawRegion(region);

    // 存储
    m_regions.insert(regionId, region);

    emit regionCreated(regionId);
    qDebug() << "创建禁飞区: ID =" << regionId << ", 半径 =" << radius << "米";

    return region;
}

Region* RegionManager::createTaskRegion(const QMapLibre::Coordinates &vertices, const QString &name) {
    if (!m_painter) {
        qWarning() << "RegionManager::createTaskRegion: painter is null!";
        return nullptr;
    }

    if (vertices.size() < 3) {
        qWarning() << "RegionManager::createTaskRegion: 多边形顶点数不足（至少需要3个）";
        return nullptr;
    }

    int regionId = generateNextId();
    Region *region = new Region(regionId, RegionType::TaskRegion);
    region->setName(name.isEmpty() ? QString("任务区域 %1").arg(regionId) : name);
    region->setVertices(vertices);
    region->setTerrainType(TerrainType::Plain);  // 默认平原

    // 计算中心点
    double sumLat = 0.0, sumLon = 0.0;
    for (const auto &coord : vertices) {
        sumLat += coord.first;
        sumLon += coord.second;
    }
    region->setCoordinate(QMapLibre::Coordinate(
        sumLat / vertices.size(),
        sumLon / vertices.size()
    ));

    // 在地图上绘制
    drawRegion(region);

    // 存储
    m_regions.insert(regionId, region);

    emit regionCreated(regionId);
    qDebug() << "创建多边形: ID =" << regionId << ", 顶点数 =" << vertices.size();

    return region;
}

Region* RegionManager::createCircularTaskRegion(const QMapLibre::Coordinate &center, double radius,
                                                const QMapLibre::Coordinates &vertices, const QString &name) {
    if (!m_painter) {
        qWarning() << "RegionManager::createCircularTaskRegion: painter is null!";
        return nullptr;
    }

    if (vertices.size() < 3) {
        qWarning() << "RegionManager::createCircularTaskRegion: 顶点数不足（至少需要3个）";
        return nullptr;
    }

    int regionId = generateNextId();
    Region *region = new Region(regionId, RegionType::TaskRegion);
    region->setName(name.isEmpty() ? QString("任务区域 %1").arg(regionId) : name);
    region->setVertices(vertices);
    region->setCoordinate(center);  // 使用传入的圆心
    region->setRadius(radius);      // 保存半径信息
    region->setTerrainType(TerrainType::Plain);  // 默认平原

    // 在地图上绘制
    drawRegion(region);

    // 存储
    m_regions.insert(regionId, region);

    emit regionCreated(regionId);
    qDebug() << "创建圆形任务区域: ID =" << regionId << ", 半径 =" << radius << "米, 顶点数 =" << vertices.size();

    return region;
}

// ==================== 删除区域 ====================

bool RegionManager::removeRegion(int regionId) {
    qDebug() << "RegionManager::removeRegion 开始删除区域 ID:" << regionId;

    Region *region = m_regions.value(regionId, nullptr);
    if (!region) {
        qWarning() << "RegionManager::removeRegion: 区域不存在, ID =" << regionId;
        return false;
    }

    QMapLibre::AnnotationID annotationId = region->annotationId();
    qDebug() << "  - 区域名称:" << region->name();
    qDebug() << "  - 区域类型:" << Region::typeToString(region->type());
    qDebug() << "  - AnnotationID:" << annotationId;

    // 从地图移除标注
    if (m_painter) {
        qDebug() << "  - 调用 MapPainter::removeAnnotation(" << annotationId << ")";
        m_painter->removeAnnotation(annotationId);
    } else {
        qWarning() << "  - 无法删除标注: painter is null";
    }

    // 从映射表删除
    m_regions.remove(regionId);
    qDebug() << "  - 已从 m_regions 中移除";

    // 发出信号（让 TaskManager 清理引用）
    emit regionRemoved(regionId);
    qDebug() << "  - 已发送 regionRemoved 信号";

    qDebug() << "RegionManager::removeRegion 完成删除区域 ID:" << regionId;

    // 释放内存
    delete region;

    return true;
}

// ==================== 查询 ====================

Region* RegionManager::getRegion(int regionId) {
    return m_regions.value(regionId, nullptr);
}

Region* RegionManager::findRegionByAnnotationId(QMapLibre::AnnotationID annotationId) {
    for (Region *region : m_regions) {
        if (region->annotationId() == annotationId) {
            return region;
        }
    }
    return nullptr;
}

Region* RegionManager::findRegionNear(const QMapLibre::Coordinate &clickCoord, double threshold) {
    if (!m_painter) {
        return nullptr;
    }

    // 使用 MapPainter 的查找功能
    const RegionInfo *regionInfo = m_painter->findRegionNear(clickCoord, threshold);
    if (!regionInfo) {
        return nullptr;
    }

    // 通过 annotationId 查找区域
    return findRegionByAnnotationId(regionInfo->annotationId);
}

const RegionInfo* RegionManager::findRegionInfoNear(const QMapLibre::Coordinate &clickCoord, double threshold) const {
    if (!m_painter) {
        return nullptr;
    }

    // 直接委托给 MapPainter
    return m_painter->findRegionNear(clickCoord, threshold);
}

// ==================== 可见性控制 ====================

void RegionManager::showRegion(int regionId) {
    Region *region = m_regions.value(regionId, nullptr);
    if (!region || !m_painter) {
        return;
    }

    // 如果已经在地图上，先移除
    if (region->annotationId() != 0) {
        m_painter->removeAnnotation(region->annotationId());
    }

    // 重新绘制
    drawRegion(region);
}

void RegionManager::hideRegion(int regionId) {
    Region *region = m_regions.value(regionId, nullptr);
    if (!region || !m_painter) {
        return;
    }

    // 从地图移除
    if (region->annotationId() != 0) {
        m_painter->removeAnnotation(region->annotationId());
        region->setAnnotationId(0);  // 标记为未绘制
    }
}

void RegionManager::showAllRegions() {
    for (Region *region : m_regions) {
        showRegion(region->id());
    }
}

void RegionManager::hideAllRegions() {
    for (Region *region : m_regions) {
        hideRegion(region->id());
    }
}

// ==================== 修改属性 ====================

bool RegionManager::updateRegionTerrainType(int regionId, Region::TerrainType type) {
    Region *region = m_regions.value(regionId, nullptr);
    if (!region) {
        return false;
    }

    region->setTerrainType(type);
    emit regionUpdated(regionId);

    qDebug() << "更新区域地形: ID =" << regionId << ", 地形 =" << Region::terrainTypeToString(type);
    return true;
}

bool RegionManager::updateRegionColor(int regionId, const QString &color) {
    Region *region = m_regions.value(regionId, nullptr);
    if (!region || region->type() != RegionType::UAV) {
        return false;
    }

    region->setColor(color);

    // 重新绘制（更新颜色）
    if (m_painter && region->annotationId() != 0) {
        m_painter->removeAnnotation(region->annotationId());
        drawRegion(region);
    }

    emit regionUpdated(regionId);

    qDebug() << "更新无人机颜色: ID =" << regionId << ", 颜色 =" << color;
    return true;
}

bool RegionManager::updateRegionName(int regionId, const QString &name) {
    Region *region = m_regions.value(regionId, nullptr);
    if (!region) {
        return false;
    }

    region->setName(name);
    emit regionUpdated(regionId);

    qDebug() << "更新区域名称: ID =" << regionId << ", 名称 =" << name;
    return true;
}

// ==================== 私有方法 ====================

int RegionManager::generateNextId() {
    return m_nextId++;
}

void RegionManager::drawRegion(Region *region) {
    if (!m_painter || !region) {
        return;
    }

    QMapLibre::AnnotationID annotationId = 0;

    switch (region->type()) {
        case RegionType::LoiterPoint:
            annotationId = m_painter->drawLoiterPoint(
                region->coordinate().first,
                region->coordinate().second
            );
            break;

        case RegionType::UAV:
            annotationId = m_painter->drawUAV(
                region->coordinate().first,
                region->coordinate().second,
                region->color()
            );
            break;

        case RegionType::NoFlyZone:
            annotationId = m_painter->drawNoFlyZone(
                region->coordinate().first,
                region->coordinate().second,
                region->radius()
            );
            break;

        case RegionType::TaskRegion:
            annotationId = m_painter->drawTaskRegionArea(region->vertices(), region->coordinate(), region->radius());
            break;
    }

    region->setAnnotationId(annotationId);
}
