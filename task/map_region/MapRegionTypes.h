// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef MAPREGIONTYPES_H
#define MAPREGIONTYPES_H

#include <QMapLibre/Types>
#include <QString>

/**
 * @brief 区域类型枚举
 */
enum class RegionType {
    LoiterPoint,  // 盘旋点
    UAV,          // 无人机
    NoFlyZone,    // 禁飞区
    TaskRegion    // 任务区域
};

/**
 * @brief 地形类型枚举
 */
enum class TerrainType {
    Plain = 0,        // 平原
    Hills = 1,        // 丘陵
    Mountain = 2,     // 山地
    HighMountain = 3  // 高山
};

/**
 * @brief 任务区域形状类型枚举
 */
enum class TaskRegionShape {
    Polygon = 0,   // 手绘多边形
    Rectangle = 1, // 矩形
    Circle = 2     // 圆形
};

/**
 * @brief 区域信息结构（用于地图交互）
 */
struct RegionInfo {
    RegionType type;
    QMapLibre::Coordinate coordinate;  // 对于点类型：位置；对于区域类型：中心点
    QMapLibre::Coordinates vertices;   // 对于任务区域：所有顶点
    double radius;                     // 对于禁飞区：半径（米）
    QString color;                     // 对于 UAV：颜色
    QMapLibre::AnnotationID annotationId;  // 标注ID
    TerrainType terrainType;           // 地形类型
    TaskRegionShape taskRegionShape;   // 任务区域形状类型（仅TaskRegion类型使用）
    int regionId;                      // 区域ID（来自RegionManager）
    int taskId;                        // 所属任务ID
    QString taskName;                  // 所属任务名称
    QString regionName;                // 区域名称
};

/**
 * @brief 辅助函数：将整数转换为 RegionType
 */
inline RegionType toRegionType(int type) {
    return static_cast<RegionType>(type);
}

/**
 * @brief 辅助函数：将整数转换为 TerrainType
 */
inline TerrainType toTerrainType(int type) {
    return static_cast<TerrainType>(type);
}

#endif // MAPREGIONTYPES_H
