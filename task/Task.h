// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QVector>
#include <QMapLibre/Types>

/**
 * @brief 地图元素 - 单个地图标记
 */
struct MapElement {
    enum Type {
        LoiterPoint,    // 盘旋点
        NoFlyZone,      // 禁飞区
        UAV,            // 无人机
        Polygon         // 多边形区域
    };

    enum TerrainType {
        Plain = 0,      // 平原
        Hills = 1,      // 丘陵
        Mountain = 2,   // 山地
        HighMountain = 3 // 高山地
    };

    Type type;
    QMapLibre::AnnotationID annotationId;  // 地图标注ID
    QMapLibre::Coordinate coordinate;      // 位置（对于区域类型：中心点）
    double radius;                         // 禁飞区半径（米）
    QString color;                         // UAV颜色
    QMapLibre::Coordinates vertices;       // 多边形顶点
    TerrainType terrainType;               // 地形特征（用于多边形和禁飞区）

    MapElement()
        : type(LoiterPoint)
        , annotationId(0)
        , radius(0.0)
        , terrainType(Plain)
    {}

    static QString terrainTypeToString(TerrainType type) {
        switch (type) {
            case Plain: return "平原";
            case Hills: return "丘陵";
            case Mountain: return "山地";
            case HighMountain: return "高山地";
            default: return "未知";
        }
    }
};

/**
 * @brief 任务类 - 包含一组地图标记
 */
class Task {
public:
    Task() : m_id(0), m_visible(true) {}

    Task(int id, const QString &name, const QString &description = QString())
        : m_id(id)
        , m_name(name)
        , m_description(description)
        , m_visible(true)
    {}

    // Getters
    int id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    bool isVisible() const { return m_visible; }
    const QVector<MapElement>& elements() const { return m_elements; }
    QVector<MapElement>& elements() { return m_elements; }

    // Setters
    void setId(int id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setDescription(const QString &description) { m_description = description; }
    void setVisible(bool visible) { m_visible = visible; }

    // 元素管理
    void addElement(const MapElement &element) {
        m_elements.append(element);
    }

    void removeElement(QMapLibre::AnnotationID annotationId) {
        for (int i = 0; i < m_elements.size(); ++i) {
            if (m_elements[i].annotationId == annotationId) {
                m_elements.removeAt(i);
                return;
            }
        }
    }

    void clearElements() {
        m_elements.clear();
    }

    MapElement* findElement(QMapLibre::AnnotationID annotationId) {
        for (int i = 0; i < m_elements.size(); ++i) {
            if (m_elements[i].annotationId == annotationId) {
                return &m_elements[i];
            }
        }
        return nullptr;
    }

private:
    int m_id;
    QString m_name;
    QString m_description;
    bool m_visible;
    QVector<MapElement> m_elements;
};

#endif // TASK_H
