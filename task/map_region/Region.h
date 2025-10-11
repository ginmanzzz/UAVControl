// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef REGION_H
#define REGION_H

#include "MapRegionTypes.h"
#include <QString>

/**
 * @brief 地图标记区域 - 完全独立的实体，不属于任何任务
 *
 * Region 是系统中的核心实体，代表地图上的一个标记区域。
 * 它完全独立于任务（Task），可以被多个任务引用。
 */
class Region {
public:
    // 使用 MapRegionTypes.h 中的全局枚举
    using Type = RegionType;
    using TerrainType = ::TerrainType;

    Region();
    Region(int id, Type type);

    // ==================== 基本属性 ====================

    int id() const { return m_id; }
    QString name() const { return m_name; }
    Type type() const { return m_type; }
    QMapLibre::AnnotationID annotationId() const { return m_annotationId; }

    void setId(int id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setType(Type type) { m_type = type; }
    void setAnnotationId(QMapLibre::AnnotationID id) { m_annotationId = id; }

    // ==================== 几何信息 ====================

    /**
     * @brief 获取坐标（点类型：位置；区域类型：中心点）
     */
    QMapLibre::Coordinate coordinate() const { return m_coordinate; }
    void setCoordinate(const QMapLibre::Coordinate &coord) { m_coordinate = coord; }

    /**
     * @brief 获取多边形顶点（仅用于 Polygon 类型）
     */
    QMapLibre::Coordinates vertices() const { return m_vertices; }
    void setVertices(const QMapLibre::Coordinates &vertices) { m_vertices = vertices; }

    /**
     * @brief 获取半径（仅用于 NoFlyZone 类型）
     */
    double radius() const { return m_radius; }
    void setRadius(double radius) { m_radius = radius; }

    // ==================== 属性 ====================

    /**
     * @brief 获取颜色（仅用于 UAV 类型）
     */
    QString color() const { return m_color; }
    void setColor(const QString &color) { m_color = color; }

    /**
     * @brief 获取地形类型（用于 NoFlyZone 和 Polygon）
     */
    TerrainType terrainType() const { return m_terrainType; }
    void setTerrainType(TerrainType type) { m_terrainType = type; }

    // ==================== 辅助方法 ====================

    static QString typeToString(Type type);
    static QString terrainTypeToString(TerrainType type);

private:
    // 基本属性
    int m_id;                              // 全局唯一ID
    QString m_name;                        // 区域名称（可选）
    Type m_type;                           // 类型
    QMapLibre::AnnotationID m_annotationId; // 地图标注ID

    // 几何信息
    QMapLibre::Coordinate m_coordinate;    // 位置/中心点
    QMapLibre::Coordinates m_vertices;     // 多边形顶点
    double m_radius;                       // 禁飞区半径（米）

    // 属性
    QString m_color;                       // UAV颜色
    TerrainType m_terrainType;             // 地形类型
};

#endif // REGION_H
