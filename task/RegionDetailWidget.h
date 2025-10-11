// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef REGIONDETAILWIDGET_H
#define REGIONDETAILWIDGET_H

#include "map_region/MapRegionTypes.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

// 前向声明
class TaskManager;

/**
 * @brief 区域详情浮动窗口
 */
class RegionDetailWidget : public QWidget {
    Q_OBJECT

public:
    explicit RegionDetailWidget(QWidget *parent = nullptr);
    void setTaskManager(TaskManager *taskManager) { m_taskManager = taskManager; }
    void showRegion(const RegionInfo *info, const QPoint &screenPos);

signals:
    void terrainChanged(int regionId, TerrainType newTerrain);
    void deleteRequested(int regionId);

private:
    void addTitle(const QString &text);
    void addInfoLine(const QString &label, const QString &value);
    void addTerrainLine(const QString &label, TerrainType currentTerrain);
    void addDeleteButton();
    QString getColorName(const QString &color) const;
    double calculateTaskRegionArea(const QMapLibre::Coordinates &vertices) const;

private:
    TaskManager *m_taskManager = nullptr;
    QWidget *m_contentWidget;
    QVBoxLayout *m_contentLayout;
    const RegionInfo *m_currentRegion;
    QPushButton *m_deleteButton;
};

#endif // REGIONDETAILWIDGET_H
