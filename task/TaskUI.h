// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASKUI_H
#define TASKUI_H

#include "map_region/MapRegionTypes.h"
#include "map_region/MapPainter.h"
#include "map_region/InteractiveMapWidget.h"
#include "RegionDetailWidget.h"
#include "map_region/RegionManager.h"
#include "TaskManager.h"
#include "TaskLeftControlWidget.h"
#include "RegionFeatureDialog.h"
#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

// 自定义悬浮提示标签
class CustomTooltip : public QLabel {
public:
    explicit CustomTooltip(QWidget *parent = nullptr);
    void showTooltip(const QString &text, const QPoint &globalPos);
};

// 自定义按钮，支持自定义 tooltip
class TooltipButton : public QPushButton {
    Q_OBJECT
public:
    explicit TooltipButton(const QString &tooltipText, QWidget *parent = nullptr);
    ~TooltipButton();
    void setTooltipText(const QString &text);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString m_tooltipText;
    CustomTooltip *m_tooltip;
};

// 任务管理 UI Widget
class TaskUI : public QWidget {
    Q_OBJECT

public:
    explicit TaskUI(QWidget *parent = nullptr);

    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void initialized();  // 当地图和组件初始化完成时发出

public slots:
    void onRegionTerrainChanged(int regionId, TerrainType newTerrain);
    void onRegionDeleteRequested(int regionId);

private slots:
    void setupMap();
    void updateOverlayPositions();
    void onCurrentTaskChanged(int taskId);

    // 地图交互
    void onMapClicked(const QMapLibre::Coordinate &coord);
    void onMapMouseMoved(const QMapLibre::Coordinate &coord);
    void onMapRightClicked();

    // 操作按钮
    void startPlaceLoiter();
    void startPlaceNoFly();
    void startPlaceUAV();
    void startDrawTaskRegion();
    void clearAll();
    void openTaskPlanDialog();

    // 绘制处理
    void addLoiterPointAt(double lat, double lon);
    void addUAVAt(double lat, double lon);
    void handleNoFlyZoneClick(double lat, double lon);
    void handleTaskRegionClick(double lat, double lon);
    void handleTaskRegionUndo();
    void finishTaskRegion();

    void returnToNormalMode();
    void resetNoFlyZoneDrawing();
    void resetTaskRegionDrawing();

private:
    void setupUI();
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    double getZoomDependentThreshold(double baseThreshold = 50.0);
    QString getColorName(const QString &colorValue);

    enum InteractionMode {
        MODE_NORMAL = 0,
        MODE_LOITER = 1,
        MODE_NOFLY = 2,
        MODE_TASK_REGION = 3,
        MODE_UAV = 4
    };

    enum TaskRegionDrawMode {
        DRAW_MODE_POLYGON = 0,   // 手绘多边形（默认）
        DRAW_MODE_RECTANGLE = 1, // 矩形（两次点击）
        DRAW_MODE_CIRCLE = 2     // 圆形（两次点击）
    };

    // UI 组件
    InteractiveMapWidget *m_mapWidget = nullptr;
    MapPainter *m_painter = nullptr;
    RegionManager *m_regionManager = nullptr;
    TaskManager *m_taskManager = nullptr;
    TaskLeftControlWidget *m_taskListWidget = nullptr;
    RegionDetailWidget *m_detailWidget = nullptr;
    QWidget *m_buttonContainer = nullptr;
    class CreateTaskPlanDialog *m_taskPlanDialog = nullptr;

    InteractionMode m_currentMode = MODE_NORMAL;
    bool m_mapInitialized = false;

    // 禁飞区绘制状态
    bool m_noFlyZoneCenterSet = false;
    QMapLibre::Coordinate m_noFlyZoneCenter;

    // 任务区域绘制状态
    TaskRegionDrawMode m_taskRegionDrawMode = DRAW_MODE_RECTANGLE;
    QMapLibre::Coordinates m_taskRegionPoints;
    QMapLibre::Coordinate m_rectangleFirstPoint;      // 矩形模式：第一个点
    bool m_rectangleFirstPointSet = false;
    QMapLibre::Coordinate m_circleCenter;             // 圆形模式：圆心
    bool m_circleCenterSet = false;
    double m_circleRadius = 0.0;                      // 圆形模式：半径（米）

    // 无人机模式状态
    bool m_isInNoFlyZone = false;
    QString m_currentUAVColor = "black";
};

#endif // TASKUI_H
