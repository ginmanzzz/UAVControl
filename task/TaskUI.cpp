// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskUI.h"
#include "CreateTaskPlanDialog.h"
#include "TaskPlan.h"
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
    if (event->key() == Qt::Key_Escape && m_currentMode == MODE_TASK_REGION) {
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

    // 创建任务区域按钮组（按钮 + 模式选择）
    auto *taskRegionContainer = new QWidget(buttonContainer);
    auto *taskRegionLayout = new QHBoxLayout(taskRegionContainer);
    taskRegionLayout->setContentsMargins(0, 0, 0, 0);
    taskRegionLayout->setSpacing(4);

    auto *taskRegionBtn = new TooltipButton("绘制任务区域", taskRegionContainer);
    taskRegionBtn->setStyleSheet(iconButtonStyle);
    QIcon taskRegionIcon("image/polygon.png");
    if (!taskRegionIcon.isNull()) {
        taskRegionBtn->setIcon(taskRegionIcon);
        taskRegionBtn->setIconSize(QSize(32, 32));
    } else {
        taskRegionBtn->setText("⬡");
        taskRegionBtn->setStyleSheet(iconButtonStyle + "QPushButton { font-size: 24px; }");
    }

    // 任务区域绘制模式选择小按钮
    auto *taskRegionModeBtn = new TooltipButton("选择绘制模式", taskRegionContainer);
    taskRegionModeBtn->setText("▼");
    taskRegionModeBtn->setStyleSheet(
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

    // 创建绘制模式选择菜单
    auto *drawModeMenu = new QMenu(taskRegionModeBtn);
    drawModeMenu->setStyleSheet(
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

    QAction *rectangleAction = drawModeMenu->addAction("矩形");
    QAction *circleAction = drawModeMenu->addAction("圆形");
    QAction *polygonAction = drawModeMenu->addAction("手绘多边形");

    connect(polygonAction, &QAction::triggered, this, [this, taskRegionModeBtn]() {
        m_taskRegionDrawMode = DRAW_MODE_POLYGON;
        taskRegionModeBtn->setTooltipText("当前模式: 手绘多边形");
        qDebug() << "切换到手绘多边形模式";
    });
    connect(rectangleAction, &QAction::triggered, this, [this, taskRegionModeBtn]() {
        m_taskRegionDrawMode = DRAW_MODE_RECTANGLE;
        taskRegionModeBtn->setTooltipText("当前模式: 矩形");
        qDebug() << "切换到矩形模式";
    });
    connect(circleAction, &QAction::triggered, this, [this, taskRegionModeBtn]() {
        m_taskRegionDrawMode = DRAW_MODE_CIRCLE;
        taskRegionModeBtn->setTooltipText("当前模式: 圆形");
        qDebug() << "切换到圆形模式";
    });

    taskRegionModeBtn->setMenu(drawModeMenu);
    // 设置初始提示为矩形（因为默认模式是矩形）
    taskRegionModeBtn->setTooltipText("当前模式: 矩形");

    taskRegionLayout->addWidget(taskRegionBtn);
    taskRegionLayout->addWidget(taskRegionModeBtn);

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

    // 创建方案规划按钮
    auto *planBtn = new TooltipButton("打开方案规划窗口", buttonContainer);
    planBtn->setText("方案规划");
    planBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(103, 58, 183, 230);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  min-width: 82px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(94, 53, 177, 240);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(81, 45, 168, 240);"
        "}"
    );

    connect(loiterBtn, &QPushButton::clicked, this, &TaskUI::startPlaceLoiter);
    connect(noFlyBtn, &QPushButton::clicked, this, &TaskUI::startPlaceNoFly);
    connect(taskRegionBtn, &QPushButton::clicked, this, &TaskUI::startDrawTaskRegion);
    connect(uavBtn, &QPushButton::clicked, this, &TaskUI::startPlaceUAV);
    connect(clearBtn, &QPushButton::clicked, this, &TaskUI::clearAll);
    connect(planBtn, &QPushButton::clicked, this, &TaskUI::openTaskPlanDialog);

    buttonLayout->addWidget(loiterBtn);
    buttonLayout->addWidget(noFlyBtn);
    buttonLayout->addWidget(taskRegionContainer);
    buttonLayout->addWidget(uavContainer);
    buttonLayout->addStretch();
    buttonLayout->addWidget(clearBtn);
    buttonLayout->addWidget(planBtn);

    buttonContainer->setStyleSheet("background: transparent;");
    m_buttonContainer = buttonContainer;
    m_buttonContainer->show();  // 按钮常驻显示，支持创建独立区域
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
    m_regionManager = new RegionManager(m_painter, this);
    m_taskManager = new TaskManager(m_regionManager, this);

    m_taskListWidget = new TaskLeftControlWidget(m_taskManager, m_mapWidget);
    m_taskListWidget->setCollapsible(true);
    m_taskListWidget->show();

    updateOverlayPositions();

    m_detailWidget = new RegionDetailWidget(this);
    m_detailWidget->setTaskManager(m_taskManager);  // 设置TaskManager引用
    m_detailWidget->setRegionManager(m_regionManager);  // 设置RegionManager引用

    connect(m_detailWidget, &RegionDetailWidget::terrainChanged,
            this, &TaskUI::onRegionTerrainChanged);
    connect(m_detailWidget, &RegionDetailWidget::deleteRequested,
            this, &TaskUI::onRegionDeleteRequested);
    connect(m_detailWidget, &RegionDetailWidget::nameChanged,
            this, [this](int regionId, const QString &newName) {
                qDebug() << "区域名称更改信号: ID =" << regionId << ", 新名称 =" << newName;
            });

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
        int height = m_mapWidget->height();  // 贯穿整个地图高度
        m_taskListWidget->setGeometry(0, 0, width, height);
        m_taskListWidget->raise();
    }
}

void TaskUI::onCurrentTaskChanged(int taskId) {
    // 按钮常驻显示，无需任务即可创建区域
    m_buttonContainer->show();

    if (taskId > 0 && m_taskManager->currentTask()) {
        qDebug() << QString("任务 #%1 已选中").arg(taskId);
    } else {
        qDebug() << "未选中任务（可创建独立区域）";
    }
}

void TaskUI::onRegionTerrainChanged(int regionId, TerrainType newTerrain) {
    Region *region = m_regionManager->getRegion(regionId);
    if (!region) {
        qWarning() << "未找到对应的区域";
        return;
    }

    m_regionManager->updateRegionTerrainType(regionId, static_cast<Region::TerrainType>(newTerrain));
    qDebug() << QString("已更新区域 ID:%1 的地形特征").arg(regionId);
}

void TaskUI::onRegionDeleteRequested(int regionId) {
    Region *region = m_regionManager->getRegion(regionId);
    if (!region) {
        QMessageBox::warning(this, "错误", "未找到对应的区域");
        return;
    }

    QString regionTypeName = Region::typeToString(region->type());

    // 获取引用计数和引用任务列表
    int refCount = m_taskManager->getRegionReferenceCount(regionId);
    QVector<Task*> referencingTasks = m_taskManager->getTasksReferencingRegion(regionId);

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setIcon(QMessageBox::Question);

    // 根据引用情况显示不同的提示
    if (refCount == 0) {
        // 独立区域，直接删除
        msgBox.setText(QString("确定要删除此%1吗？\n\n这是一个独立区域，删除后将从地图上永久移除。").arg(regionTypeName));
    } else if (refCount == 1) {
        // 被一个任务引用
        msgBox.setText(QString("确定要删除此%1吗？\n\n该区域被任务 #%2 (%3) 引用。\n删除后将从地图上永久移除。")
                       .arg(regionTypeName)
                       .arg(referencingTasks[0]->id())
                       .arg(referencingTasks[0]->name()));
    } else {
        // 被多个任务引用
        QString taskList;
        for (Task *task : referencingTasks) {
            if (!taskList.isEmpty()) taskList += ", ";
            taskList += QString("#%1 (%2)").arg(task->id()).arg(task->name());
        }
        msgBox.setText(QString("确定要删除此%1吗？\n\n该区域被 %2 个任务引用：%3\n删除后将从地图上永久移除，并从所有任务中移除。")
                       .arg(regionTypeName)
                       .arg(refCount)
                       .arg(taskList));
        msgBox.setInformativeText("警告：多个任务正在使用此区域！");
    }

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        // 真正删除region（RegionManager会自动通知TaskManager清理引用）
        m_regionManager->removeRegion(regionId);
        qDebug() << QString("已删除%1 ID:%2 (引用计数: %3)").arg(regionTypeName).arg(regionId).arg(refCount);
    }
}

void TaskUI::onMapClicked(const QMapLibre::Coordinate &coord) {
    if (!m_painter) {
        return;
    }

    qDebug() << QString("地图被点击: (%1, %2)").arg(coord.first).arg(coord.second);

    if (m_currentMode == MODE_NORMAL) {
        double threshold = getZoomDependentThreshold(100.0);
        const RegionInfo* element = m_taskManager->findVisibleElementNear(coord, threshold);
        if (element) {
            QPoint screenPos = QCursor::pos();
            m_detailWidget->showRegion(element, screenPos);
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
    case MODE_TASK_REGION:
        handleTaskRegionClick(coord.first, coord.second);
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
        bool inNoFlyZone = m_taskManager->isInAnyNoFlyZone(coord);

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
    else if (m_currentMode == MODE_TASK_REGION) {
        // 任务区域绘制的鼠标跟随动画
        switch (m_taskRegionDrawMode) {
        case DRAW_MODE_RECTANGLE:
            if (m_rectangleFirstPointSet) {
                // 矩形模式：显示动态矩形预览（蓝色填充，无边框）
                double lat1 = m_rectangleFirstPoint.first;
                double lon1 = m_rectangleFirstPoint.second;
                double lat2 = coord.first;
                double lon2 = coord.second;

                QMapLibre::Coordinates rectPoints;
                rectPoints.append(QMapLibre::Coordinate(lat1, lon1));  // 左上
                rectPoints.append(QMapLibre::Coordinate(lat1, lon2));  // 右上
                rectPoints.append(QMapLibre::Coordinate(lat2, lon2));  // 右下
                rectPoints.append(QMapLibre::Coordinate(lat2, lon1));  // 左下

                // 使用新的蓝色填充矩形预览方法
                m_painter->drawPreviewRectangle(rectPoints);

                double width = calculateDistance(lat1, lon1, lat1, lon2);
                double height = calculateDistance(lat1, lon1, lat2, lon1);
                m_mapWidget->setStatusText(
                    QString("矩形预览 - 宽: %1m, 高: %2m (点击确定/右键取消)")
                        .arg(width, 0, 'f', 1).arg(height, 0, 'f', 1),
                    "rgba(255, 243, 205, 220)"
                );
            }
            break;

        case DRAW_MODE_CIRCLE:
            if (m_circleCenterSet) {
                // 圆形模式：显示动态圆形预览
                double radius = calculateDistance(
                    m_circleCenter.first, m_circleCenter.second,
                    coord.first, coord.second
                );

                m_painter->drawPreviewNoFlyZone(m_circleCenter.first, m_circleCenter.second, radius);

                m_mapWidget->setStatusText(
                    QString("圆形预览 - 中心: (%1, %2), 半径: %3m (点击确定/右键取消)")
                        .arg(m_circleCenter.first, 0, 'f', 5)
                        .arg(m_circleCenter.second, 0, 'f', 5)
                        .arg(radius, 0, 'f', 1),
                    "rgba(255, 243, 205, 220)"
                );
            }
            break;

        case DRAW_MODE_POLYGON:
        default:
            // 手绘多边形模式：显示动态连线
            if (!m_taskRegionPoints.isEmpty()) {
                m_painter->updateDynamicLine(m_taskRegionPoints.last(), coord);
            }
            break;
        }
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
    } else if (m_currentMode == MODE_TASK_REGION) {
        handleTaskRegionUndo();
    }
}

void TaskUI::startPlaceLoiter() {
    m_currentMode = MODE_LOITER;
    m_mapWidget->setClickEnabled(true);
    m_mapWidget->setCustomCursor("image/pin.png");

    if (m_taskManager->currentTask()) {
        m_mapWidget->setStatusText(QString("放置盘旋点到任务 #%1 - 点击地图任意位置（右键取消）")
                                   .arg(m_taskManager->currentTaskId()), "rgba(212, 237, 218, 220)");
        qDebug() << QString("开始放置盘旋点到任务 #%1").arg(m_taskManager->currentTaskId());
    } else {
        m_mapWidget->setStatusText("放置独立盘旋点 - 点击地图任意位置（右键取消）", "rgba(212, 237, 218, 220)");
        qDebug() << "开始放置独立盘旋点";
    }
}

void TaskUI::startPlaceNoFly() {
    m_currentMode = MODE_NOFLY;
    m_mapWidget->setClickEnabled(true);
    resetNoFlyZoneDrawing();

    if (m_taskManager->currentTask()) {
        m_mapWidget->setStatusText(QString("放置禁飞区到任务 #%1 - 点击中心点，移动鼠标确定半径（右键取消）")
                                   .arg(m_taskManager->currentTaskId()), "rgba(255, 243, 205, 220)");
        qDebug() << QString("开始放置禁飞区到任务 #%1").arg(m_taskManager->currentTaskId());
    } else {
        m_mapWidget->setStatusText("放置独立禁飞区 - 点击中心点，移动鼠标确定半径（右键取消）", "rgba(255, 243, 205, 220)");
        qDebug() << "开始放置独立禁飞区";
    }
}

void TaskUI::startPlaceUAV() {
    m_currentMode = MODE_UAV;
    m_isInNoFlyZone = false;
    m_mapWidget->setClickEnabled(true);
    m_mapWidget->setCustomCursor("image/uav.png", 12, 12);

    QString colorName = getColorName(m_currentUAVColor);
    if (m_taskManager->currentTask()) {
        m_mapWidget->setStatusText(QString("放置无人机 (%1) 到任务 #%2 - 点击地图任意位置（右键取消）")
                                   .arg(colorName).arg(m_taskManager->currentTaskId()), "rgba(230, 230, 255, 220)");
        qDebug() << QString("开始放置无人机到任务 #%1 - 颜色: %2").arg(m_taskManager->currentTaskId()).arg(m_currentUAVColor);
    } else {
        m_mapWidget->setStatusText(QString("放置独立无人机 (%1) - 点击地图任意位置（右键取消）").arg(colorName), "rgba(230, 230, 255, 220)");
        qDebug() << "开始放置独立无人机 - 颜色:" << m_currentUAVColor;
    }
}

void TaskUI::startDrawTaskRegion() {
    m_currentMode = MODE_TASK_REGION;
    m_mapWidget->setClickEnabled(true);
    resetTaskRegionDrawing();

    QString modeText;
    QString statusHint;

    // 根据绘制模式显示不同的提示
    switch (m_taskRegionDrawMode) {
    case DRAW_MODE_RECTANGLE:
        modeText = "矩形";
        statusHint = "点击设置左上角和右下角（右键取消）";
        break;
    case DRAW_MODE_CIRCLE:
        modeText = "圆形";
        statusHint = "点击设置圆心，移动鼠标确定半径（右键取消）";
        break;
    case DRAW_MODE_POLYGON:
    default:
        modeText = "手绘多边形";
        statusHint = "点击添加顶点，点击起点闭合，右键回退，ESC取消";
        break;
    }

    if (m_taskManager->currentTask()) {
        m_mapWidget->setStatusText(
            QString("绘制任务区域（%1）到任务 #%2 - %3")
                .arg(modeText)
                .arg(m_taskManager->currentTaskId())
                .arg(statusHint),
            "rgba(255, 243, 205, 220)"
        );
        qDebug() << QString("开始绘制任务区域（%1）到任务 #%2").arg(modeText).arg(m_taskManager->currentTaskId());
    } else {
        m_mapWidget->setStatusText(
            QString("绘制独立任务区域（%1） - %2").arg(modeText).arg(statusHint),
            "rgba(255, 243, 205, 220)"
        );
        qDebug() << QString("开始绘制独立任务区域（%1）").arg(modeText);
    }
}

void TaskUI::clearAll() {
    if (!m_taskManager) {
        qWarning() << "任务管理器未初始化";
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认清除");
    msgBox.setIcon(QMessageBox::Question);

    if (m_taskManager->currentTask()) {
        // 有当前任务：清除当前任务的所有标记
        msgBox.setText(QString("确定要清除任务 #%1 的所有地图标记吗？")
                          .arg(m_taskManager->currentTaskId()));
        msgBox.setInformativeText(QString("任务名称: %1").arg(m_taskManager->currentTask()->name()));
    } else {
        // 无当前任务：清除所有独立区域
        msgBox.setText("确定要清除所有独立区域吗？");
        msgBox.setInformativeText("将清除所有不属于任何任务的区域。");
    }

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        if (m_taskManager->currentTask()) {
            // 清除当前任务的标记
            m_taskManager->clearCurrentTask();
            qDebug() << QString("已清除任务 #%1 的所有标注").arg(m_taskManager->currentTaskId());
        } else {
            // 清除所有独立区域（引用计数为0的区域）
            const QMap<int, Region*>& allRegions = m_regionManager->getAllRegions();
            QVector<int> independentRegionIds;

            for (Region *region : allRegions) {
                if (m_taskManager->getRegionReferenceCount(region->id()) == 0) {
                    independentRegionIds.append(region->id());
                }
            }

            for (int regionId : independentRegionIds) {
                m_regionManager->removeRegion(regionId);
            }

            qDebug() << QString("已清除 %1 个独立区域").arg(independentRegionIds.size());
        }

        resetNoFlyZoneDrawing();
        resetTaskRegionDrawing();
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
    if (m_taskManager->isInAnyNoFlyZone(coord)) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("无法放置");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("无法在禁飞区域内放置无人机！");
        msgBox.setInformativeText("请选择禁飞区域以外的位置。");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        qDebug() << QString("尝试在禁飞区内放置无人机 (%1, %2)，已阻止").arg(lat).arg(lon);
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

        QVector<Region*> conflictUAVs = m_taskManager->checkNoFlyZoneConflictWithUAVs(
            m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

        if (!conflictUAVs.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("无法放置禁飞区");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(QString("该禁飞区域会覆盖 %1 架无人机！").arg(conflictUAVs.size()));
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

        RegionPropertyDialog featureDialog("临时禁飞区", static_cast<RegionPropertyDialog::TerrainType>(0), this);
        if (featureDialog.exec() == QDialog::Accepted) {
            auto terrainType = featureDialog.getSelectedTerrain();
            auto id = m_taskManager->addNoFlyZone(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

            if (id > 0) {
                m_regionManager->updateRegionTerrainType(id, static_cast<Region::TerrainType>(terrainType));
            }

            qDebug() << QString("创建禁飞区: 中心(%1, %2), 半径 %3m, 地形 %4, 任务 #%5, ID: %6")
                        .arg(m_noFlyZoneCenter.first).arg(m_noFlyZoneCenter.second)
                        .arg(radius).arg(featureDialog.getTerrainName())
                        .arg(m_taskManager->currentTaskId()).arg(id);
        }

        returnToNormalMode();
    }
}

void TaskUI::handleTaskRegionClick(double lat, double lon) {
    QMapLibre::Coordinate clickedPoint(lat, lon);

    // 根据不同模式处理点击
    switch (m_taskRegionDrawMode) {
    case DRAW_MODE_RECTANGLE:
        // 矩形模式：两次点击
        if (!m_rectangleFirstPointSet) {
            // 第一次点击：设置左上角
            m_rectangleFirstPoint = clickedPoint;
            m_rectangleFirstPointSet = true;
            qDebug() << QString("矩形第一点（左上角）: (%1, %2)").arg(lat).arg(lon);
            m_mapWidget->setStatusText(
                QString("矩形左上角已设置: (%1, %2) - 点击设置右下角（右键取消）")
                    .arg(lat, 0, 'f', 5).arg(lon, 0, 'f', 5),
                "rgba(255, 243, 205, 220)"
            );
        } else {
            // 第二次点击：设置右下角并完成矩形
            double lat1 = m_rectangleFirstPoint.first;
            double lon1 = m_rectangleFirstPoint.second;
            double lat2 = clickedPoint.first;
            double lon2 = clickedPoint.second;

            // 构建矩形的四个顶点
            m_taskRegionPoints.clear();
            m_taskRegionPoints.append(QMapLibre::Coordinate(lat1, lon1));  // 左上
            m_taskRegionPoints.append(QMapLibre::Coordinate(lat1, lon2));  // 右上
            m_taskRegionPoints.append(QMapLibre::Coordinate(lat2, lon2));  // 右下
            m_taskRegionPoints.append(QMapLibre::Coordinate(lat2, lon1));  // 左下

            qDebug() << QString("矩形第二点（右下角）: (%1, %2)，矩形绘制完成").arg(lat).arg(lon);
            finishTaskRegion();
        }
        break;

    case DRAW_MODE_CIRCLE:
        // 圆形模式：两次点击
        if (!m_circleCenterSet) {
            // 第一次点击：设置圆心
            m_circleCenter = clickedPoint;
            m_circleCenterSet = true;
            qDebug() << QString("圆形中心点: (%1, %2)").arg(lat).arg(lon);
            m_mapWidget->setStatusText(
                QString("圆心已设置: (%1, %2) - 移动鼠标确定半径（右键取消）")
                    .arg(lat, 0, 'f', 5).arg(lon, 0, 'f', 5),
                "rgba(255, 243, 205, 220)"
            );
        } else {
            // 第二次点击：设置半径并完成圆形
            m_circleRadius = calculateDistance(
                m_circleCenter.first, m_circleCenter.second,
                clickedPoint.first, clickedPoint.second
            );

            // 用多边形近似圆形（32个顶点）
            int segments = 32;
            m_taskRegionPoints.clear();
            const double EARTH_RADIUS = 6378137.0;

            for (int i = 0; i < segments; ++i) {
                double angle = 2.0 * M_PI * i / segments;
                double dx = m_circleRadius * std::cos(angle);
                double dy = m_circleRadius * std::sin(angle);

                double lat_rad = m_circleCenter.first * M_PI / 180.0;
                double lon_rad = m_circleCenter.second * M_PI / 180.0;

                double new_lat = lat_rad + (dy / EARTH_RADIUS);
                double new_lon = lon_rad + (dx / (EARTH_RADIUS * std::cos(lat_rad)));

                m_taskRegionPoints.append(QMapLibre::Coordinate(
                    new_lat * 180.0 / M_PI,
                    new_lon * 180.0 / M_PI
                ));
            }

            qDebug() << QString("圆形半径点: (%1, %2)，半径 %3m，圆形绘制完成")
                        .arg(lat).arg(lon).arg(m_circleRadius);
            finishTaskRegion();
        }
        break;

    case DRAW_MODE_POLYGON:
    default:
        // 手绘多边形模式：多次点击
        if (m_taskRegionPoints.size() >= 3) {
            double distanceToStart = calculateDistance(
                clickedPoint.first, clickedPoint.second,
                m_taskRegionPoints.first().first, m_taskRegionPoints.first().second
            );

            double threshold = getZoomDependentThreshold(50.0);

            qDebug() << QString("多边形闭合检测: 距离起点 %1 米, 阈值 %2 米 (缩放级别 %3)")
                        .arg(distanceToStart, 0, 'f', 2)
                        .arg(threshold, 0, 'f', 2)
                        .arg(m_mapWidget->map()->zoom(), 0, 'f', 2);

            if (distanceToStart < threshold) {
                qDebug() << "点击起点，闭合多边形";
                finishTaskRegion();
                return;
            }
        }

        m_taskRegionPoints.append(clickedPoint);
        qDebug() << QString("添加多边形顶点 #%1: (%2, %3)")
                        .arg(m_taskRegionPoints.size())
                        .arg(lat).arg(lon);

        m_painter->clearDynamicLine();

        if (m_taskRegionPoints.size() >= 2) {
            m_painter->drawPreviewLines(m_taskRegionPoints);
        }

        m_mapWidget->setStatusText(
            QString("绘制任务区域 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                .arg(m_taskRegionPoints.size()),
            "rgba(255, 243, 205, 220)"
        );
        break;
    }
}

void TaskUI::handleTaskRegionUndo() {
    // 根据不同模式处理撤销
    switch (m_taskRegionDrawMode) {
    case DRAW_MODE_RECTANGLE:
        if (m_rectangleFirstPointSet) {
            // 撤销矩形的第一个点
            m_rectangleFirstPointSet = false;
            m_rectangleFirstPoint = QMapLibre::Coordinate(0, 0);
            m_painter->clearPreview();
            m_painter->clearTaskRegionPreview();
            qDebug() << "撤销矩形第一点";
            m_mapWidget->setStatusText("绘制矩形 - 点击设置左上角（右键取消）", "rgba(255, 243, 205, 220)");
        } else {
            qDebug() << "没有矩形顶点，取消矩形绘制";
            returnToNormalMode();
        }
        break;

    case DRAW_MODE_CIRCLE:
        if (m_circleCenterSet) {
            // 撤销圆形的圆心
            m_circleCenterSet = false;
            m_circleCenter = QMapLibre::Coordinate(0, 0);
            m_painter->clearPreview();
            m_painter->clearTaskRegionPreview();
            qDebug() << "撤销圆形中心点";
            m_mapWidget->setStatusText("绘制圆形 - 点击设置圆心（右键取消）", "rgba(255, 243, 205, 220)");
        } else {
            qDebug() << "没有圆心，取消圆形绘制";
            returnToNormalMode();
        }
        break;

    case DRAW_MODE_POLYGON:
    default:
        if (m_taskRegionPoints.isEmpty()) {
            qDebug() << "没有顶点，取消多边形绘制";
            returnToNormalMode();
            return;
        }

        m_taskRegionPoints.removeLast();
        qDebug() << QString("回退一个顶点，剩余 %1 个").arg(m_taskRegionPoints.size());

        if (m_taskRegionPoints.isEmpty()) {
            m_painter->clearTaskRegionPreview();
            returnToNormalMode();
        } else if (m_taskRegionPoints.size() == 1) {
            m_painter->clearTaskRegionPreview();
            m_mapWidget->setStatusText(
                QString("绘制任务区域 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                    .arg(m_taskRegionPoints.size()),
                "rgba(255, 243, 205, 220)"
            );
        } else {
            m_painter->drawPreviewLines(m_taskRegionPoints);
            m_mapWidget->setStatusText(
                QString("绘制任务区域 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                    .arg(m_taskRegionPoints.size()),
                "rgba(255, 243, 205, 220)"
            );
        }
        break;
    }
}

void TaskUI::finishTaskRegion() {
    if (m_taskRegionPoints.size() < 3) {
        qWarning() << "多边形至少需要3个顶点";
        return;
    }

    RegionPropertyDialog featureDialog("临时任务区域", static_cast<RegionPropertyDialog::TerrainType>(0), this);
    if (featureDialog.exec() == QDialog::Accepted) {
        auto terrainType = featureDialog.getSelectedTerrain();
        QMapLibre::AnnotationID id = 0;

        // 根据绘制模式选择不同的创建方法
        if (m_taskRegionDrawMode == DRAW_MODE_CIRCLE && m_circleRadius > 0) {
            // 圆形任务区域：传入圆心和半径
            id = m_taskManager->addCircularTaskRegion(m_circleCenter, m_circleRadius, m_taskRegionPoints);
            qDebug() << QString("圆形任务区域绘制完成，圆心: (%1, %2), 半径: %3m, 地形: %4, 任务 #%5, ID: %6")
                        .arg(m_circleCenter.first, 0, 'f', 5)
                        .arg(m_circleCenter.second, 0, 'f', 5)
                        .arg(m_circleRadius)
                        .arg(featureDialog.getTerrainName())
                        .arg(m_taskManager->currentTaskId()).arg(id);
        } else if (m_taskRegionDrawMode == DRAW_MODE_RECTANGLE && m_taskRegionPoints.size() == 4) {
            // 矩形任务区域
            id = m_taskManager->addRectangularTaskRegion(m_taskRegionPoints);
            qDebug() << QString("矩形任务区域绘制完成，地形 %1, 任务 #%2, ID: %3")
                        .arg(featureDialog.getTerrainName())
                        .arg(m_taskManager->currentTaskId()).arg(id);
        } else {
            // 多边形任务区域（手绘）
            id = m_taskManager->addTaskRegion(m_taskRegionPoints);
            qDebug() << QString("多边形绘制完成，地形 %1, 任务 #%2, ID: %3")
                        .arg(featureDialog.getTerrainName())
                        .arg(m_taskManager->currentTaskId()).arg(id);
        }

        if (id > 0) {
            m_regionManager->updateRegionTerrainType(id, static_cast<Region::TerrainType>(terrainType));
        }
    }

    m_painter->clearTaskRegionPreview();
    m_taskRegionPoints.clear();
    m_circleRadius = 0.0;  // 重置圆形半径
    returnToNormalMode();
}

void TaskUI::returnToNormalMode() {
    m_currentMode = MODE_NORMAL;
    m_mapWidget->setClickEnabled(true);
    m_mapWidget->restoreDefaultCursor();
    resetNoFlyZoneDrawing();
    resetTaskRegionDrawing();
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

void TaskUI::resetTaskRegionDrawing() {
    m_taskRegionPoints.clear();
    m_rectangleFirstPointSet = false;
    m_rectangleFirstPoint = QMapLibre::Coordinate(0, 0);
    m_circleCenterSet = false;
    m_circleCenter = QMapLibre::Coordinate(0, 0);
    if (m_painter) {
        m_painter->clearTaskRegionPreview();
        m_painter->clearDynamicLine();
        m_painter->clearPreview();  // 清除圆形/矩形预览
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

void TaskUI::openTaskPlanDialog() {
    qDebug() << "打开方案规划窗口";

    // 延迟创建 CreateTaskPlanDialog
    if (!m_taskPlanDialog) {
        m_taskPlanDialog = new CreateTaskPlanDialog(m_taskManager, this);  // 传入 TaskManager，TaskUI作为父窗口

        // 创建一个示例方案
        static TaskPlan *sampleTaskPlan = new TaskPlan(1, "示例方案");
        m_taskPlanDialog->setTaskPlan(sampleTaskPlan);
    }

    // 显示窗口并定位到左上角
    m_taskPlanDialog->show();
    m_taskPlanDialog->raise();

    // 定位到左上角（带10px边距）
    m_taskPlanDialog->setGeometry(50, 50, m_taskPlanDialog->width(), m_taskPlanDialog->height());

    qDebug() << "方案窗口定位到左上角: (50, 50)";
}
