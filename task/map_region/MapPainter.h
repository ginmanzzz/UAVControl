// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef MAPPAINTER_H
#define MAPPAINTER_H

#include "MapRegionTypes.h"
#include <QMapLibre/Map>
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>

/**
 * @brief 地图画家类 - 用于在地图上绘制区域标记
 *
 */
class MapPainter : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param map QMapLibre 地图对象指针
     * @param parent 父对象
     */
    explicit MapPainter(QMapLibre::Map *map, QObject *parent = nullptr);


    /**
     * @brief 画盘旋点（使用自定义图标）
     * @param latitude 纬度
     * @param longitude 经度
     * @return 标注 ID，用于后续删除
     */
    QMapLibre::AnnotationID drawLoiterPoint(double latitude, double longitude);

    /**
     * @brief 画无人机图标（使用 UAV 图标）
     * @param latitude 纬度
     * @param longitude 经度
     * @param color 颜色（black=不染色, red, blue, purple, green, yellow）
     * @return 标注 ID，用于后续删除
     */
    QMapLibre::AnnotationID drawUAV(double latitude, double longitude, const QString &color = "black");

    /**
     * @brief 画禁飞区域（红色半透明圆形）
     * @param latitude 中心点纬度
     * @param longitude 中心点经度
     * @param radiusInMeters 半径（米）
     * @return 区域标注 ID
     */
    QMapLibre::AnnotationID drawNoFlyZone(double latitude, double longitude, double radiusInMeters);

    /**
     * @brief 绘制任务区域（蓝色半透明）
     * @param coordinates 任务区域顶点坐标列表
     * @param center 圆心坐标（仅圆形任务区域需要）
     * @param radius 半径（仅圆形任务区域需要）
     * @return 区域标注 ID
     */
    QMapLibre::AnnotationID drawTaskRegionArea(const QMapLibre::Coordinates &coordinates,
                                               const QMapLibre::Coordinate &center = QMapLibre::Coordinate(),
                                               double radius = 0.0);

    /**
     * @brief 删除指定标注
     * @param id 标注 ID
     */
    void removeAnnotation(QMapLibre::AnnotationID id);

    /**
     * @brief 根据点击位置查找最近的区域
     * @param clickCoord 点击的地理坐标
     * @param threshold 阈值距离（米）
     * @return 区域信息指针，未找到则返回 nullptr
     */
    const RegionInfo* findRegionNear(const QMapLibre::Coordinate &clickCoord, double threshold = 100.0) const;

    // ==================== MapPainter 特有方法 ====================

    /**
     * @brief 清除所有由该画家创建的标注
     */
    void clearAll();

    /**
     * @brief 绘制预览禁飞区域（用于鼠标移动时显示）
     * @param latitude 中心点纬度
     * @param longitude 中心点经度
     * @param radiusInMeters 半径（米）
     * @return 预览标注 ID
     */
    QMapLibre::AnnotationID drawPreviewNoFlyZone(double latitude, double longitude, double radiusInMeters);

    /**
     * @brief 绘制预览矩形区域（用于鼠标移动时显示）
     * @param coordinates 矩形的四个顶点坐标
     * @return 预览标注 ID
     */
    QMapLibre::AnnotationID drawPreviewRectangle(const QMapLibre::Coordinates &coordinates);

    /**
     * @brief 清除预览标注
     */
    void clearPreview();

    /**
     * @brief 绘制预览线段（从点集合绘制连线）
     * @param coordinates 点坐标列表
     * @return 线段标注 ID
     */
    QMapLibre::AnnotationID drawPreviewLines(const QMapLibre::Coordinates &coordinates);

    /**
     * @brief 清除任务区域预览（包括线段和临时标记）
     */
    void clearTaskRegionPreview();

    /**
     * @brief 更新动态预览线（从上一个点到鼠标当前位置）
     * @param fromCoord 起始坐标
     * @param toCoord 目标坐标（鼠标位置）
     * @return 线段标注 ID
     */
    QMapLibre::AnnotationID updateDynamicLine(const QMapLibre::Coordinate &fromCoord, const QMapLibre::Coordinate &toCoord);

    /**
     * @brief 清除动态预览线
     */
    void clearDynamicLine();

    /**
     * @brief 设置盘旋点图标路径（默认为 image/pin.png）
     * @param iconPath 图标文件路径
     * @return 是否成功加载图标
     */
    bool setLoiterIconPath(const QString &iconPath);

    /**
     * @brief 检查点是否在任何禁飞区内
     * @param coord 地理坐标
     * @return 如果在禁飞区内返回 true，否则返回 false
     */
    bool isInNoFlyZone(const QMapLibre::Coordinate &coord) const;

private:
    /**
     * @brief 加载盘旋点图标
     * @return 是否成功加载
     */
    bool loadLoiterIcon();

    /**
     * @brief 加载 UAV 图标（指定颜色）
     * @param color 颜色名称
     * @return 是否成功加载
     */
    bool loadUAVIcon(const QString &color);

    /**
     * @brief 生成圆形的坐标点集合
     * @param centerLat 中心纬度
     * @param centerLon 中心经度
     * @param radiusInMeters 半径（米）
     * @param numPoints 圆周点数（默认64个点）
     * @return 坐标集合
     */
    QMapLibre::Coordinates generateCircleCoordinates(
        double centerLat,
        double centerLon,
        double radiusInMeters,
        int numPoints = 64
    );

private:
    QMapLibre::Map *m_map;                          // 地图对象
    QString m_loiterIconPath;                       // 盘旋点图标路径
    bool m_iconLoaded;                              // 盘旋点图标是否已加载
    QSet<QString> m_loadedUAVColors;                // 已加载的 UAV 图标颜色集合
    QVector<QMapLibre::AnnotationID> m_annotations; // 所有标注 ID
    QMap<QMapLibre::AnnotationID, RegionInfo> m_regionInfo; // 区域信息映射表
    QMapLibre::AnnotationID m_previewAnnotationId;  // 预览区域标注 ID
    QMapLibre::AnnotationID m_taskRegionPreviewLineId; // 任务区域预览线段 ID
    QMapLibre::AnnotationID m_dynamicLineId;        // 动态预览线 ID

    static constexpr const char* LOITER_ICON_NAME = "loiter-point-icon";
    static constexpr const char* UAV_ICON_PATH = "image/uav.png";
};

#endif // MAPPAINTER_H
