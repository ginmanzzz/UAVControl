// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "Region.h"

Region::Region()
    : m_id(0)
    , m_type(RegionType::LoiterPoint)
    , m_annotationId(0)
    , m_radius(0.0)
    , m_terrainType(TerrainType::Plain)
{
}

Region::Region(int id, Type type)
    : m_id(id)
    , m_type(type)
    , m_annotationId(0)
    , m_radius(0.0)
    , m_terrainType(TerrainType::Plain)
{
}

QString Region::typeToString(Type type) {
    switch (type) {
        case RegionType::LoiterPoint:
            return "盘旋点";
        case RegionType::UAV:
            return "无人机";
        case RegionType::NoFlyZone:
            return "禁飞区";
        case RegionType::TaskRegion:
            return "任务区域";
        default:
            return "未知";
    }
}

QString Region::terrainTypeToString(TerrainType type) {
    switch (type) {
        case TerrainType::Plain:
            return "平原";
        case TerrainType::Hills:
            return "丘陵";
        case TerrainType::Mountain:
            return "山地";
        case TerrainType::HighMountain:
            return "高山地";
        default:
            return "未知";
    }
}
