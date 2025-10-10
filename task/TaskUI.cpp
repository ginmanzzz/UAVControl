// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QtMath>
#include <QMenu>
#include <QIcon>
#include <cmath>

// ==================== CustomTooltip Implementation ====================
CustomTooltip::CustomTooltip(QWidget *parent) : QLabel(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setStyleSheet(
        "QLabel {"
        "  background-color: rgb(255, 255, 255);"
        "  color: rgb(0, 0, 0);"
        "  border: 1px solid rgb(200, 200, 200);"
        "  border-radius: 4px;"
        "  padding: 6px 10px;"
        "  font-size: 12px;"
        "}"
    );

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    hide();
}

void CustomTooltip::showTooltip(const QString &text, const QPoint &globalPos) {
    setText(text);
    adjustSize();
    move(globalPos.x() + 15, globalPos.y() - height() - 5);
    show();
    raise();
}

// ==================== TooltipButton Implementation ====================
TooltipButton::TooltipButton(const QString &tooltipText, QWidget *parent)
    : QPushButton(parent), m_tooltipText(tooltipText) {
    setMouseTracking(true);
    m_tooltip = new CustomTooltip(nullptr);
}

TooltipButton::~TooltipButton() {
    delete m_tooltip;
}

void TooltipButton::setTooltipText(const QString &text) {
    m_tooltipText = text;
}

void TooltipButton::enterEvent(QEnterEvent *event) {
    QPushButton::enterEvent(event);
    if (!m_tooltipText.isEmpty()) {
        m_tooltip->showTooltip(m_tooltipText, QCursor::pos());
    }
}

void TooltipButton::leaveEvent(QEvent *event) {
    QPushButton::leaveEvent(event);
    m_tooltip->hide();
}

// ==================== TaskUI Implementation ====================
TaskUI::TaskUI(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void TaskUI::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!m_mapInitialized) {
        m_mapInitialized = true;
        QTimer::singleShot(200, this, &TaskUI::setupMap);
    }
}

void TaskUI::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateOverlayPositions();
}

void TaskUI::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && m_currentMode == MODE_POLYGON) {
        qDebug() << "按下ESC，取消多边形绘制";
        returnToNormalMode();
    }
    QWidget::keyPressEvent(event);
}

void TaskUI::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建交互式地图 Widget
    QMapLibre::Settings settings;
    settings.setDefaultZoom(12);
    settings.setDefaultCoordinate(QMapLibre::Coordinate(39.9, 116.4)); // 北京

    m_mapWidget = new InteractiveMapWidget(settings);
    mainLayout->addWidget(m_mapWidget);

    // 创建浮动在地图右侧的按钮容器
    auto *buttonContainer = new QWidget(m_mapWidget);
    auto *buttonLayout = new QVBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(10, 10, 10, 10);
    buttonLayout->setSpacing(12);

    // 图标按钮样式（方形）
    QString iconButtonStyle =
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 230);"
        "  border: 2px solid #ccc;"
        "  border-radius: 8px;"
        "  padding: 0px;"
        "  min-width: 50px;"
        "  max-width: 50px;"
        "  min-height: 50px;"
        "  max-height: 50px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(33, 150, 243, 230);"
        "  border: 2px solid #2196F3;"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(25, 118, 210, 240);"
        "}";

    // 创建盘旋点按钮
    auto *loiterBtn = new TooltipButton("放置盘旋点", buttonContainer);
    loiterBtn->setStyleSheet(iconButtonStyle);
    QIcon loiterIcon("image/pin.png");
    if (!loiterIcon.isNull()) {
        loiterBtn->setIcon(loiterIcon);
        loiterBtn->setIconSize(QSize(32, 32));
    } else {
        loiterBtn->setText("📍");
        loiterBtn->setStyleSheet(iconButtonStyle + "QPushButton { font-size: 24px; }");
    }

    // 创建禁飞区域按钮
    auto *noFlyBtn = new TooltipButton("放置禁飞区域", buttonContainer);
    noFlyBtn->setStyleSheet(iconButtonStyle);
    QIcon noFlyIcon("image/nofly.png");
    if (!noFlyIcon.isNull()) {
        noFlyBtn->setIcon(noFlyIcon);
        noFlyBtn->setIconSize(QSize(32, 32));
    } else {
        noFlyBtn->setText("🚫");
        noFlyBtn->setStyleSheet(iconButtonStyle + "QPushButton { font-size: 24px; }");
    }

    // 创建多边形按钮
    auto *polygonBtn = new TooltipButton("绘制多边形", buttonContainer);
    polygonBtn->setStyleSheet(iconButtonStyle);
    QIcon polygonIcon("image/polygon.png");
    if (!polygonIcon.isNull()) {
        polygonBtn->setIcon(polygonIcon);
        polygonBtn->setIconSize(QSize(32, 32));
    } else {
        polygonBtn->setText("⬡");
        polygonBtn->setStyleSheet(iconButtonStyle + "QPushButton { font-size: 24px; }");
    }

    // 创建无人机按钮组（按钮 + 颜色选择）
    auto *uavContainer = new QWidget(buttonContainer);
    auto *uavLayout = new QHBoxLayout(uavContainer);
    uavLayout->setContentsMargins(0, 0, 0, 0);
    uavLayout->setSpacing(4);

    auto *uavBtn = new TooltipButton("放置无人机", uavContainer);
    uavBtn->setStyleSheet(iconButtonStyle);
    QIcon uavIcon("image/uav.png");
    if (!uavIcon.isNull()) {
        uavBtn->setIcon(uavIcon);
        uavBtn->setIconSize(QSize(32, 32));
    } else {
        uavBtn->setText("✈");
        uavBtn->setStyleSheet(iconButtonStyle + "QPushButton { font-size: 24px; }");
    }

    // 无人机颜色选择小按钮
    auto *uavColorBtn = new TooltipButton("选择无人机颜色", uavContainer);
    uavColorBtn->setText("▼");
    uavColorBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 230);"
        "  border: 2px solid #ccc;"
        "  border-radius: 8px;"
        "  padding: 0px;"
        "  min-width: 28px;"
        "  max-width: 28px;"
        "  min-height: 50px;"
        "  max-height: 50px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(240, 240, 240, 240);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(220, 220, 220, 240);"
        "}"
    );

    // 创建颜色选择菜单
    auto *colorMenu = new QMenu(uavColorBtn);
    colorMenu->setStyleSheet(
        "QMenu {"
        "  background-color: rgb(255, 255, 255);"
        "  border: 1px solid rgb(200, 200, 200);"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
        "QMenu::item {"
        "  background-color: transparent;"
        "  color: rgb(0, 0, 0);"
        "  padding: 6px 20px;"
        "  border-radius: 2px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: rgb(33, 150, 243);"
        "  color: white;"
        "}"
    );

    QStringList colors = {"黑色", "红色", "蓝色", "紫色", "绿色", "黄色"};
    QStringList colorValues = {"black", "red", "blue", "purple", "green", "yellow"};

    for (int i = 0; i < colors.size(); ++i) {
        QAction *action = colorMenu->addAction(colors[i]);
        action->setData(colorValues[i]);
        connect(action, &QAction::triggered, this, [this, action, uavColorBtn, colors, i]() {
            m_currentUAVColor = action->data().toString();
            uavColorBtn->setTooltipText(QString("当前颜色: %1").arg(colors[i]));
            qDebug() << "选择无人机颜色:" << m_currentUAVColor;
        });
    }
    uavColorBtn->setMenu(colorMenu);

    uavLayout->addWidget(uavBtn);
    uavLayout->addWidget(uavColorBtn);

    // 创建清除按钮
    auto *clearBtn = new TooltipButton("清除当前任务的所有标记", buttonContainer);
    clearBtn->setText("清除");
    clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(244, 67, 54, 230);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  min-width: 82px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(211, 47, 47, 240);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(198, 40, 40, 240);"
        "}"
    );

    connect(loiterBtn, &QPushButton::clicked, this, &TaskUI::startPlaceLoiter);
    connect(noFlyBtn, &QPushButton::clicked, this, &TaskUI::startPlaceNoFly);
    connect(polygonBtn, &QPushButton::clicked, this, &TaskUI::startDrawPolygon);
    connect(uavBtn, &QPushButton::clicked, this, &TaskUI::startPlaceUAV);
    connect(clearBtn, &QPushButton::clicked, this, &TaskUI::clearAll);

    buttonLayout->addWidget(loiterBtn);
    buttonLayout->addWidget(noFlyBtn);
    buttonLayout->addWidget(polygonBtn);
    buttonLayout->addWidget(uavContainer);
    buttonLayout->addStretch();
    buttonLayout->addWidget(clearBtn);

    buttonContainer->setStyleSheet("background: transparent;");
    m_buttonContainer = buttonContainer;
    m_buttonContainer->hide();
}

void TaskUI::setupMap() {
    // 设置高德地图样式
    QString amapStyle = R"({
        "version": 8,
        "name": "AMap",
        "sources": {
            "amap": {
                "type": "raster",
                "tiles": ["https://webrd01.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=8&x={x}&y={y}&z={z}"],
                "tileSize": 256,
                "maxzoom": 18
            }
        },
        "layers": [{
            "id": "amap",
            "type": "raster",
            "source": "amap"
        }]
    })";

    m_mapWidget->map()->setStyleJson(amapStyle);

    m_painter = new MapPainter(m_mapWidget->map(), this);
    m_taskManager = new TaskManager(m_painter, this);

    m_taskListWidget = new TaskListWidget(m_taskManager, m_mapWidget);
    m_taskListWidget->setCollapsible(true);
    m_taskListWidget->show();

    updateOverlayPositions();

    m_detailWidget = new ElementDetailWidget(this);

    connect(m_detailWidget, &ElementDetailWidget::terrainChanged,
            this, &TaskUI::onElementTerrainChanged);
    connect(m_detailWidget, &ElementDetailWidget::deleteRequested,
            this, &TaskUI::onElementDeleteRequested);

    connect(m_mapWidget, &InteractiveMapWidget::mapClicked,
            this, &TaskUI::onMapClicked);
    connect(m_mapWidget, &InteractiveMapWidget::mapMouseMoved,
            this, &TaskUI::onMapMouseMoved);
    connect(m_mapWidget, &InteractiveMapWidget::mapRightClicked,
            this, &TaskUI::onMapRightClicked);

    connect(m_taskManager, &TaskManager::currentTaskChanged,
            this, &TaskUI::onCurrentTaskChanged);

    emit initialized();
    qDebug() << "TaskUI 地图初始化完成";
}

void TaskUI::updateOverlayPositions() {
    if (m_buttonContainer && m_mapWidget) {
        int containerWidth = 100;
        int containerHeight = 320;
        int x = m_mapWidget->width() - containerWidth - 10;
        int y = (m_mapWidget->height() - containerHeight) / 2;
        m_buttonContainer->setGeometry(x, y, containerWidth, containerHeight);
        m_buttonContainer->raise();
    }

    if (m_taskListWidget && m_mapWidget) {
        int width = m_taskListWidget->width();
        int height = m_mapWidget->height() * 0.55;
        int y = (m_mapWidget->height() - height) / 2;
        m_taskListWidget->setGeometry(0, y, width, height);
        m_taskListWidget->raise();
    }
}

void TaskUI::onCurrentTaskChanged(int taskId) {
    if (taskId > 0 && m_taskManager->currentTask()) {
        m_buttonContainer->show();
        qDebug() << QString("任务 #%1 已选中，显示操作按钮").arg(taskId);
    } else {
        m_buttonContainer->hide();
        qDebug() << "未选中任务，隐藏操作按钮";
    }
}

void TaskUI::onElementTerrainChanged(QMapLibre::AnnotationID annotationId, MapElement::TerrainType newTerrain) {
    Task *ownerTask = nullptr;
    MapElement *element = nullptr;

    for (Task *task : m_taskManager->getAllTasks()) {
        MapElement *found = task->findElement(annotationId);
        if (found) {
            ownerTask = task;
            element = found;
            break;
        }
    }

    if (!element || !ownerTask) {
        qWarning() << "未找到对应的元素";
        return;
    }

    element->terrainType = newTerrain;
    qDebug() << QString("已更新元素 ID:%1 的地形特征为: %2")
                .arg(annotationId)
                .arg(MapElement::terrainTypeToString(newTerrain));
}

void TaskUI::onElementDeleteRequested(QMapLibre::AnnotationID annotationId) {
    Task *ownerTask = nullptr;
    MapElement *element = nullptr;

    for (Task *task : m_taskManager->getAllTasks()) {
        MapElement *found = task->findElement(annotationId);
        if (found) {
            ownerTask = task;
            element = found;
            break;
        }
    }

    if (!ownerTask || !element) {
        QMessageBox::warning(this, "错误", "未找到对应的元素");
        return;
    }

    QString elementTypeName;
    switch (element->type) {
        case MapElement::LoiterPoint:
            elementTypeName = "盘旋点";
            break;
        case MapElement::NoFlyZone:
            elementTypeName = "禁飞区域";
            break;
        case MapElement::UAV:
            elementTypeName = "无人机";
            break;
        case MapElement::Polygon:
            elementTypeName = "多边形区域";
            break;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString("确定要删除此%1吗？").arg(elementTypeName));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        m_painter->removeAnnotation(annotationId);
        ownerTask->removeElement(annotationId);
        qDebug() << QString("已删除%1 ID:%2").arg(elementTypeName).arg(annotationId);
    }
}

void TaskUI::onMapClicked(const QMapLibre::Coordinate &coord) {
    if (!m_painter) {
        return;
    }

    qDebug() << QString("地图被点击: (%1, %2)").arg(coord.first).arg(coord.second);

    if (m_currentMode == MODE_NORMAL) {
        double threshold = getZoomDependentThreshold(100.0);
        const ElementInfo* element = m_taskManager->findVisibleElementNear(coord, threshold);
        if (element) {
            QPoint screenPos = QCursor::pos();
            m_detailWidget->showElement(element, screenPos);
            qDebug() << "点击到可见任务的元素，显示详情";
        } else {
            m_detailWidget->hide();
        }
        return;
    }

    switch (m_currentMode) {
    case MODE_LOITER:
        addLoiterPointAt(coord.first, coord.second);
        break;
    case MODE_NOFLY:
        handleNoFlyZoneClick(coord.first, coord.second);
        break;
    case MODE_POLYGON:
        handlePolygonClick(coord.first, coord.second);
        break;
    case MODE_UAV:
        addUAVAt(coord.first, coord.second);
        break;
    default:
        break;
    }
}

void TaskUI::onMapMouseMoved(const QMapLibre::Coordinate &coord) {
    if (!m_painter) {
        return;
    }

    if (m_currentMode == MODE_UAV) {
        bool inNoFlyZone = m_taskManager->isInCurrentTaskNoFlyZone(coord);

        if (inNoFlyZone != m_isInNoFlyZone) {
            m_isInNoFlyZone = inNoFlyZone;

            if (inNoFlyZone) {
                m_mapWidget->setForbiddenCursor();
                qDebug() << "鼠标进入当前任务的禁飞区";
            } else {
                m_mapWidget->setCustomCursor("image/uav.png", 12, 12);
                qDebug() << "鼠标离开当前任务的禁飞区";
            }
        }
    }
    else if (m_currentMode == MODE_NOFLY && m_noFlyZoneCenterSet) {
        double radius = calculateDistance(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second,
                                          coord.first, coord.second);

        m_painter->drawPreviewNoFlyZone(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

        m_mapWidget->setStatusText(QString("禁飞区预览 - 中心: (%1, %2), 半径: %3米 (点击确定/右键取消)")
                                   .arg(m_noFlyZoneCenter.first, 0, 'f', 5)
                                   .arg(m_noFlyZoneCenter.second, 0, 'f', 5)
                                   .arg(radius, 0, 'f', 1),
                                   "rgba(255, 243, 205, 220)");
    }
    else if (m_currentMode == MODE_POLYGON && !m_polygonPoints.isEmpty()) {
        m_painter->updateDynamicLine(m_polygonPoints.last(), coord);
    }
}

void TaskUI::onMapRightClicked() {
    if (m_currentMode == MODE_LOITER) {
        qDebug() << "右键取消盘旋点放置";
        returnToNormalMode();
    } else if (m_currentMode == MODE_NOFLY) {
        qDebug() << "右键取消禁飞区放置";
        returnToNormalMode();
    } else if (m_currentMode == MODE_UAV) {
        qDebug() << "右键取消无人机放置";
        returnToNormalMode();
    } else if (m_currentMode == MODE_POLYGON) {
        handlePolygonUndo();
    }
}

void TaskUI::startPlaceLoiter() {
    if (!m_taskManager->currentTask()) {
        QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
        return;
    }

    m_currentMode = MODE_LOITER;
    m_mapWidget->setClickEnabled(true);
    m_mapWidget->setCustomCursor("image/pin.png");
    m_mapWidget->setStatusText("放置盘旋点 - 点击地图任意位置（右键取消）", "rgba(212, 237, 218, 220)");
    qDebug() << "开始单次放置盘旋点";
}

void TaskUI::startPlaceNoFly() {
    if (!m_taskManager->currentTask()) {
        QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
        return;
    }

    m_currentMode = MODE_NOFLY;
    m_mapWidget->setClickEnabled(true);
    resetNoFlyZoneDrawing();
    m_mapWidget->setStatusText("放置禁飞区域 - 点击中心点，移动鼠标确定半径（右键取消）", "rgba(255, 243, 205, 220)");
    qDebug() << "开始单次放置禁飞区域";
}

void TaskUI::startPlaceUAV() {
    if (!m_taskManager->currentTask()) {
        QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
        return;
    }

    m_currentMode = MODE_UAV;
    m_isInNoFlyZone = false;
    m_mapWidget->setClickEnabled(true);
    m_mapWidget->setCustomCursor("image/uav.png", 12, 12);

    QString colorName = getColorName(m_currentUAVColor);
    m_mapWidget->setStatusText(QString("放置无人机 (%1) - 点击地图任意位置（右键取消）").arg(colorName), "rgba(230, 230, 255, 220)");
    qDebug() << "开始单次放置无人机 - 颜色:" << m_currentUAVColor;
}

void TaskUI::startDrawPolygon() {
    if (!m_taskManager->currentTask()) {
        QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
        return;
    }

    m_currentMode = MODE_POLYGON;
    m_mapWidget->setClickEnabled(true);
    resetPolygonDrawing();
    m_mapWidget->setStatusText("绘制多边形 - 点击添加顶点，点击起点闭合，右键回退，ESC取消", "rgba(255, 243, 205, 220)");
    qDebug() << "开始绘制多边形";
}

void TaskUI::clearAll() {
    if (!m_taskManager) {
        qWarning() << "任务管理器未初始化";
        return;
    }

    if (!m_taskManager->currentTask()) {
        QMessageBox::warning(this, "未选择任务", "请先选择一个任务！");
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认清除");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString("确定要清除任务 #%1 的所有地图标记吗？")
                      .arg(m_taskManager->currentTaskId()));
    msgBox.setInformativeText(QString("任务名称: %1").arg(m_taskManager->currentTask()->name()));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        m_taskManager->clearCurrentTask();
        resetNoFlyZoneDrawing();
        resetPolygonDrawing();
        qDebug() << QString("已清除任务 #%1 的所有标注").arg(m_taskManager->currentTaskId());
    }
}

void TaskUI::addLoiterPointAt(double lat, double lon) {
    auto id = m_taskManager->addLoiterPoint(lat, lon);
    qDebug() << QString("在 (%1, %2) 添加盘旋点到任务 #%3，ID: %4")
                .arg(lat).arg(lon).arg(m_taskManager->currentTaskId()).arg(id);
    returnToNormalMode();
}

void TaskUI::addUAVAt(double lat, double lon) {
    QMapLibre::Coordinate coord(lat, lon);
    if (m_taskManager->isInCurrentTaskNoFlyZone(coord)) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("无法放置");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("无法在当前任务的禁飞区域内放置无人机！");
        msgBox.setInformativeText("请选择禁飞区域以外的位置。");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        qDebug() << QString("尝试在当前任务的禁飞区内放置无人机 (%1, %2)，已阻止").arg(lat).arg(lon);
        return;
    }

    auto id = m_taskManager->addUAV(lat, lon, m_currentUAVColor);
    qDebug() << QString("在 (%1, %2) 添加无人机 (%3) 到任务 #%4，ID: %5")
                .arg(lat).arg(lon).arg(m_currentUAVColor).arg(m_taskManager->currentTaskId()).arg(id);
    returnToNormalMode();
}

void TaskUI::handleNoFlyZoneClick(double lat, double lon) {
    if (!m_noFlyZoneCenterSet) {
        m_noFlyZoneCenter = QMapLibre::Coordinate(lat, lon);
        m_noFlyZoneCenterSet = true;
        qDebug() << QString("设置禁飞区中心点: (%1, %2)").arg(lat).arg(lon);
        m_mapWidget->setStatusText(QString("禁飞区中心已设置: (%1, %2) - 移动鼠标确定半径（右键取消）")
                                   .arg(lat, 0, 'f', 5).arg(lon, 0, 'f', 5),
                                   "rgba(255, 243, 205, 220)");
    } else {
        double radius = calculateDistance(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, lat, lon);

        QVector<const MapElement*> conflictUAVs = m_taskManager->checkNoFlyZoneConflictWithUAVs(
            m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

        if (!conflictUAVs.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("无法放置禁飞区");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(QString("该禁飞区域会覆盖 %1 架当前任务的无人机！").arg(conflictUAVs.size()));
            msgBox.setInformativeText("请调整禁飞区位置或半径，或先移除冲突的无人机。");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();

            qDebug() << QString("尝试放置禁飞区，但与 %1 架无人机冲突，已阻止")
                        .arg(conflictUAVs.size());

            m_painter->clearPreview();
            resetNoFlyZoneDrawing();
            m_mapWidget->setStatusText("放置禁飞区 - 点击中心点，移动鼠标确定半径（右键取消）", "rgba(255, 243, 205, 220)");
            return;
        }

        m_painter->clearPreview();

        RegionFeatureDialog featureDialog(this);
        if (featureDialog.exec() == QDialog::Accepted) {
            auto terrainType = featureDialog.getSelectedTerrain();
            auto id = m_taskManager->addNoFlyZone(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

            if (id > 0) {
                Task *currentTask = m_taskManager->currentTask();
                if (currentTask) {
                    MapElement *element = currentTask->findElement(id);
                    if (element) {
                        element->terrainType = static_cast<MapElement::TerrainType>(terrainType);
                    }
                }
            }

            qDebug() << QString("创建禁飞区: 中心(%1, %2), 半径 %3m, 地形 %4, 任务 #%5, ID: %6")
                        .arg(m_noFlyZoneCenter.first).arg(m_noFlyZoneCenter.second)
                        .arg(radius).arg(featureDialog.getTerrainName())
                        .arg(m_taskManager->currentTaskId()).arg(id);
        }

        returnToNormalMode();
    }
}

void TaskUI::handlePolygonClick(double lat, double lon) {
    QMapLibre::Coordinate clickedPoint(lat, lon);

    if (m_polygonPoints.size() >= 3) {
        double distanceToStart = calculateDistance(
            clickedPoint.first, clickedPoint.second,
            m_polygonPoints.first().first, m_polygonPoints.first().second
        );

        double threshold = getZoomDependentThreshold(50.0);

        qDebug() << QString("多边形闭合检测: 距离起点 %1 米, 阈值 %2 米 (缩放级别 %3)")
                    .arg(distanceToStart, 0, 'f', 2)
                    .arg(threshold, 0, 'f', 2)
                    .arg(m_mapWidget->map()->zoom(), 0, 'f', 2);

        if (distanceToStart < threshold) {
            qDebug() << "点击起点，闭合多边形";
            finishPolygon();
            return;
        }
    }

    m_polygonPoints.append(clickedPoint);
    qDebug() << QString("添加多边形顶点 #%1: (%2, %3)")
                    .arg(m_polygonPoints.size())
                    .arg(lat).arg(lon);

    m_painter->clearDynamicLine();

    if (m_polygonPoints.size() >= 2) {
        m_painter->drawPreviewLines(m_polygonPoints);
    }

    m_mapWidget->setStatusText(
        QString("绘制多边形 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
            .arg(m_polygonPoints.size()),
        "rgba(255, 243, 205, 220)"
    );
}

void TaskUI::handlePolygonUndo() {
    if (m_polygonPoints.isEmpty()) {
        qDebug() << "没有顶点，取消多边形绘制";
        returnToNormalMode();
        return;
    }

    m_polygonPoints.removeLast();
    qDebug() << QString("回退一个顶点，剩余 %1 个").arg(m_polygonPoints.size());

    if (m_polygonPoints.isEmpty()) {
        m_painter->clearPolygonPreview();
        returnToNormalMode();
    } else if (m_polygonPoints.size() == 1) {
        m_painter->clearPolygonPreview();
        m_mapWidget->setStatusText(
            QString("绘制多边形 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                .arg(m_polygonPoints.size()),
            "rgba(255, 243, 205, 220)"
        );
    } else {
        m_painter->drawPreviewLines(m_polygonPoints);
        m_mapWidget->setStatusText(
            QString("绘制多边形 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                .arg(m_polygonPoints.size()),
            "rgba(255, 243, 205, 220)"
        );
    }
}

void TaskUI::finishPolygon() {
    if (m_polygonPoints.size() < 3) {
        qWarning() << "多边形至少需要3个顶点";
        return;
    }

    RegionFeatureDialog featureDialog(this);
    if (featureDialog.exec() == QDialog::Accepted) {
        auto terrainType = featureDialog.getSelectedTerrain();
        auto id = m_taskManager->addPolygon(m_polygonPoints);

        if (id > 0) {
            Task *currentTask = m_taskManager->currentTask();
            if (currentTask) {
                MapElement *element = currentTask->findElement(id);
                if (element) {
                    element->terrainType = static_cast<MapElement::TerrainType>(terrainType);
                }
            }
        }

        qDebug() << QString("多边形绘制完成，地形 %1, 任务 #%2, ID: %3")
                    .arg(featureDialog.getTerrainName())
                    .arg(m_taskManager->currentTaskId()).arg(id);
    }

    m_painter->clearPolygonPreview();
    m_polygonPoints.clear();
    returnToNormalMode();
}

void TaskUI::returnToNormalMode() {
    m_currentMode = MODE_NORMAL;
    m_mapWidget->setClickEnabled(true);
    m_mapWidget->restoreDefaultCursor();
    resetNoFlyZoneDrawing();
    resetPolygonDrawing();
    m_mapWidget->setStatusText("普通浏览 - 点击元素查看详情，左键拖动，滚轮缩放");
    qDebug() << "返回普通浏览模式";
}

void TaskUI::resetNoFlyZoneDrawing() {
    if (m_painter) {
        m_painter->clearPreview();
    }
    m_noFlyZoneCenterSet = false;
    m_noFlyZoneCenter = QMapLibre::Coordinate(0, 0);
}

void TaskUI::resetPolygonDrawing() {
    m_polygonPoints.clear();
    if (m_painter) {
        m_painter->clearPolygonPreview();
        m_painter->clearDynamicLine();
    }
}

double TaskUI::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double EARTH_RADIUS = 6378137.0;

    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;

    double a = qSin(dLat / 2) * qSin(dLat / 2) +
               qCos(lat1 * M_PI / 180.0) * qCos(lat2 * M_PI / 180.0) *
               qSin(dLon / 2) * qSin(dLon / 2);

    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    return EARTH_RADIUS * c;
}

double TaskUI::getZoomDependentThreshold(double baseThreshold) {
    if (!m_mapWidget || !m_mapWidget->map()) {
        return baseThreshold;
    }

    double zoom = m_mapWidget->map()->zoom();
    double threshold = baseThreshold * pow(2.0, 12.0 - zoom);

    return threshold;
}

QString TaskUI::getColorName(const QString &colorValue) {
    static QMap<QString, QString> colorNames = {
        {"black", "黑色"},
        {"red", "红色"},
        {"blue", "蓝色"},
        {"purple", "紫色"},
        {"green", "绿色"},
        {"yellow", "黄色"}
    };
    return colorNames.value(colorValue, "黑色");
}
