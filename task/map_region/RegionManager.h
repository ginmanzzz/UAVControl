// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef REGIONMANAGER_H
#define REGIONMANAGER_H

#include "Region.h"
#include "MapPainter.h"
#include <QObject>
#include <QMap>

/**
 * @brief 区域管理器 - 管理所有地图标记区域（独立于任务）
 *
 * RegionManager 负责：
 * - 创建和删除区域（Region）
 * - 在地图上绘制和移除区域
 * - 提供区域的查询接口
 * - 管理区域的可见性
 *
 * 注意：RegionManager 拥有所有 Region 的所有权
 */
class RegionManager : public QObject {
    Q_OBJECT

public:
    explicit RegionManager(MapPainter *painter, QObject *parent = nullptr);
    ~RegionManager();

    // ==================== 创建区域 ====================

    /**
     * @brief 创建盘旋点
     * @param lat 纬度
     * @param lon 经度
     * @param name 区域名称（可选）
     * @return 创建的区域指针
     */
    Region* createLoiterPoint(double lat, double lon, const QString &name = QString());

    /**
     * @brief 创建无人机
     * @param lat 纬度
     * @param lon 经度
     * @param color 颜色（black, red, blue, purple, green, yellow）
     * @param name 区域名称（可选）
     * @return 创建的区域指针
     */
    Region* createUAV(double lat, double lon, const QString &color, const QString &name = QString());

    /**
     * @brief 创建禁飞区
     * @param lat 中心点纬度
     * @param lon 中心点经度
     * @param radius 半径（米）
     * @param name 区域名称（可选）
     * @return 创建的区域指针
     */
    Region* createNoFlyZone(double lat, double lon, double radius, const QString &name = QString());

    /**
     * @brief 创建任务区域
     * @param vertices 任务区域顶点
     * @param name 区域名称（可选）
     * @return 创建的区域指针
     */
    Region* createTaskRegion(const QMapLibre::Coordinates &vertices, const QString &name = QString());

    // ==================== 删除区域 ====================

    /**
     * @brief 删除区域（从地图和内存中移除）
     * @param regionId 区域ID
     * @return 成功返回 true，失败返回 false
     */
    bool removeRegion(int regionId);

    // ==================== 查询 ====================

    /**
     * @brief 获取指定区域
     * @param regionId 区域ID
     * @return 区域指针，不存在返回 nullptr
     */
    Region* getRegion(int regionId);

    /**
     * @brief 获取所有区域
     * @return 区域映射表（regionId -> Region*）
     */
    const QMap<int, Region*>& getAllRegions() const { return m_regions; }

    /**
     * @brief 通过地图标注ID查找区域
     * @param annotationId 地图标注ID
     * @return 区域指针，不存在返回 nullptr
     */
    Region* findRegionByAnnotationId(QMapLibre::AnnotationID annotationId);

    /**
     * @brief 查找点击位置附近的区域
     * @param clickCoord 点击坐标
     * @param threshold 距离阈值（米）
     * @return 区域指针，不存在返回 nullptr
     */
    Region* findRegionNear(const QMapLibre::Coordinate &clickCoord, double threshold = 100.0);

    /**
     * @brief 查找点击位置附近的区域（返回 RegionInfo）
     * @param clickCoord 点击坐标
     * @param threshold 距离阈值（米）
     * @return 区域信息指针，不存在返回 nullptr
     */
    const RegionInfo* findRegionInfoNear(const QMapLibre::Coordinate &clickCoord, double threshold = 100.0) const;

    // ==================== 可见性控制 ====================

    /**
     * @brief 显示区域
     * @param regionId 区域ID
     */
    void showRegion(int regionId);

    /**
     * @brief 隐藏区域
     * @param regionId 区域ID
     */
    void hideRegion(int regionId);

    /**
     * @brief 显示所有区域
     */
    void showAllRegions();

    /**
     * @brief 隐藏所有区域
     */
    void hideAllRegions();

    // ==================== 修改属性 ====================

    /**
     * @brief 更新区域的地形类型
     * @param regionId 区域ID
     * @param type 地形类型
     * @return 成功返回 true
     */
    bool updateRegionTerrainType(int regionId, Region::TerrainType type);

    /**
     * @brief 更新区域的颜色（仅用于UAV）
     * @param regionId 区域ID
     * @param color 颜色
     * @return 成功返回 true
     */
    bool updateRegionColor(int regionId, const QString &color);

    /**
     * @brief 更新区域的名称
     * @param regionId 区域ID
     * @param name 名称
     * @return 成功返回 true
     */
    bool updateRegionName(int regionId, const QString &name);

signals:
    /**
     * @brief 区域创建信号
     * @param regionId 区域ID
     */
    void regionCreated(int regionId);

    /**
     * @brief 区域删除信号
     * @param regionId 区域ID
     */
    void regionRemoved(int regionId);

    /**
     * @brief 区域更新信号
     * @param regionId 区域ID
     */
    void regionUpdated(int regionId);

private:
    /**
     * @brief 生成下一个区域ID
     */
    int generateNextId();

    /**
     * @brief 在地图上绘制区域
     * @param region 区域指针
     */
    void drawRegion(Region *region);

private:
    MapPainter *m_painter;             // 地图绘制器（不拥有所有权）
    QMap<int, Region*> m_regions;     // regionId -> Region*（拥有所有权）
    int m_nextId;                     // 下一个区域ID
};

#endif // REGIONMANAGER_H
