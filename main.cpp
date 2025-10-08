// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "MapPainter.h"
#include "InteractiveMapWidget.h"
#include "ElementDetailWidget.h"
#include "TaskManager.h"
#include "TaskListWidget.h"
#include "RegionFeatureDialog.h"
#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QTimer>
#include <QShowEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QtMath>
#include <cmath>

class TestPainterWindow : public QWidget {
    Q_OBJECT

public:
    TestPainterWindow() {
        setupUI();
    }

    void showEvent(QShowEvent *event) override {
        QWidget::showEvent(event);
        if (!m_mapInitialized) {
            m_mapInitialized = true;
            // 延迟设置样式，确保 OpenGL 上下文已初始化
            QTimer::singleShot(200, this, &TestPainterWindow::setupMap);
        }
    }

    void onElementTerrainChanged(QMapLibre::AnnotationID annotationId, MapElement::TerrainType newTerrain) {
        // 查找包含此annotationId的任务和元素
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

        // 更新地形特征
        element->terrainType = newTerrain;
        qDebug() << QString("已更新元素 ID:%1 的地形特征为: %2")
                    .arg(annotationId)
                    .arg(MapElement::terrainTypeToString(newTerrain));
    }

    void onElementDeleteRequested(QMapLibre::AnnotationID annotationId) {
        // 查找包含此annotationId的任务
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

        // 确定元素类型的中文名称
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

        // 确认删除
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("确认删除");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(QString("确定要删除此%1吗？").arg(elementTypeName));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes) {
            // 删除地图标注
            m_painter->removeAnnotation(annotationId);

            // 从任务中移除元素
            ownerTask->removeElement(annotationId);

            qDebug() << QString("已删除%1 ID:%2").arg(elementTypeName).arg(annotationId);
        }
    }

private:
    void setupUI() {
        // 创建主布局（地图占据整个窗口）
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 创建交互式地图 Widget
        QMapLibre::Settings settings;
        settings.setDefaultZoom(12);
        settings.setDefaultCoordinate(QMapLibre::Coordinate(39.9, 116.4)); // 北京

        m_mapWidget = new InteractiveMapWidget(settings);

        // 添加到主布局
        mainLayout->addWidget(m_mapWidget);

        // 创建浮动在地图右侧的按钮容器
        auto *buttonContainer = new QWidget(m_mapWidget);
        auto *buttonLayout = new QVBoxLayout(buttonContainer);
        buttonLayout->setContentsMargins(10, 10, 10, 10);
        buttonLayout->setSpacing(8);

        auto *loiterBtn = new QPushButton("放置盘旋点", buttonContainer);
        auto *noFlyBtn = new QPushButton("放置禁飞区域", buttonContainer);
        auto *polygonBtn = new QPushButton("绘制多边形", buttonContainer);
        auto *uavBtn = new QPushButton("放置无人机", buttonContainer);

        // 创建 UAV 颜色选择下拉框
        m_uavColorCombo = new QComboBox(buttonContainer);
        m_uavColorCombo->addItem("黑色", "black");
        m_uavColorCombo->addItem("红色", "red");
        m_uavColorCombo->addItem("蓝色", "blue");
        m_uavColorCombo->addItem("紫色", "purple");
        m_uavColorCombo->addItem("绿色", "green");
        m_uavColorCombo->addItem("黄色", "yellow");
        m_uavColorCombo->setCurrentIndex(0); // 默认黑色

        auto *clearBtn = new QPushButton("清除当前任务标记", buttonContainer);

        // 设置按钮样式
        QString buttonStyle =
            "QPushButton {"
            "  background-color: rgba(255, 255, 255, 230);"
            "  border: 1px solid #ccc;"
            "  border-radius: 4px;"
            "  padding: 8px 12px;"
            "  font-size: 13px;"
            "  min-width: 140px;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(240, 240, 240, 240);"
            "}"
            "QPushButton:pressed {"
            "  background-color: rgba(220, 220, 220, 240);"
            "}";

        QString comboStyle =
            "QComboBox {"
            "  background-color: rgba(255, 255, 255, 230);"
            "  border: 1px solid #ccc;"
            "  border-radius: 4px;"
            "  padding: 6px 12px;"
            "  font-size: 13px;"
            "  min-width: 140px;"
            "}"
            "QComboBox:hover {"
            "  background-color: rgba(240, 240, 240, 240);"
            "}";

        loiterBtn->setStyleSheet(buttonStyle);
        noFlyBtn->setStyleSheet(buttonStyle);
        polygonBtn->setStyleSheet(buttonStyle);
        uavBtn->setStyleSheet(buttonStyle);
        clearBtn->setStyleSheet(buttonStyle);
        m_uavColorCombo->setStyleSheet(comboStyle);

        connect(loiterBtn, &QPushButton::clicked, this, &TestPainterWindow::startPlaceLoiter);
        connect(noFlyBtn, &QPushButton::clicked, this, &TestPainterWindow::startPlaceNoFly);
        connect(polygonBtn, &QPushButton::clicked, this, &TestPainterWindow::startDrawPolygon);
        connect(uavBtn, &QPushButton::clicked, this, &TestPainterWindow::startPlaceUAV);
        connect(clearBtn, &QPushButton::clicked, this, &TestPainterWindow::clearAll);

        buttonLayout->addWidget(loiterBtn);
        buttonLayout->addWidget(noFlyBtn);
        buttonLayout->addWidget(polygonBtn);
        buttonLayout->addWidget(uavBtn);
        buttonLayout->addWidget(m_uavColorCombo);
        buttonLayout->addStretch();
        buttonLayout->addWidget(clearBtn);

        buttonContainer->setStyleSheet("background: transparent;");
        m_buttonContainer = buttonContainer;

        // 初始隐藏按钮容器（未选中任务时）
        m_buttonContainer->hide();

        setWindowTitle("MapPainter");
        resize(1000, 700);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        updateOverlayPositions();
    }

    void updateOverlayPositions() {
        if (m_buttonContainer && m_mapWidget) {
            // 按钮容器放在右侧居中位置
            int containerWidth = 180;  // 按钮容器宽度
            int containerHeight = 300; // 按钮容器高度（根据按钮数量调整）
            int x = m_mapWidget->width() - containerWidth - 10;  // 距离右边缘10像素
            int y = (m_mapWidget->height() - containerHeight) / 2;  // 垂直居中
            m_buttonContainer->setGeometry(x, y, containerWidth, containerHeight);
            m_buttonContainer->raise();  // 确保显示在最上层
        }

        if (m_taskListWidget && m_mapWidget) {
            // 任务列表放在左侧，高度为屏幕的55%，垂直居中
            int width = m_taskListWidget->width();
            int height = m_mapWidget->height() * 0.55;  // 55%高度
            int y = (m_mapWidget->height() - height) / 2;  // 垂直居中
            m_taskListWidget->setGeometry(0, y, width, height);
            m_taskListWidget->raise();
        }
    }

    void setupMap() {
        // 设置 OSM 样式
        QString osmStyle = R"({
            "version": 8,
            "name": "OSM Standard",
            "sources": {
                "osm": {
                    "type": "raster",
                    "tiles": ["https://tile.openstreetmap.org/{z}/{x}/{y}.png"],
                    "tileSize": 256,
                    "maxzoom": 19
                }
            },
            "layers": [{
                "id": "osm",
                "type": "raster",
                "source": "osm"
            }]
        })";

        m_mapWidget->map()->setStyleJson(osmStyle);

        // 创建画家对象
        m_painter = new MapPainter(m_mapWidget->map(), this);

        // 创建任务管理器
        m_taskManager = new TaskManager(m_painter, this);

        // 创建任务列表 Widget（浮动在地图左侧）
        m_taskListWidget = new TaskListWidget(m_taskManager, m_mapWidget);
        m_taskListWidget->setCollapsible(true);
        m_taskListWidget->show();  // 确保显示

        // 更新位置
        updateOverlayPositions();

        // 创建详情窗口
        m_detailWidget = new ElementDetailWidget(this);

        // 连接详情窗口的地形修改和删除信号
        connect(m_detailWidget, &ElementDetailWidget::terrainChanged,
                this, &TestPainterWindow::onElementTerrainChanged);
        connect(m_detailWidget, &ElementDetailWidget::deleteRequested,
                this, &TestPainterWindow::onElementDeleteRequested);

        // 连接地图点击信号
        connect(m_mapWidget, &InteractiveMapWidget::mapClicked,
                this, &TestPainterWindow::onMapClicked);
        connect(m_mapWidget, &InteractiveMapWidget::mapMouseMoved,
                this, &TestPainterWindow::onMapMouseMoved);
        connect(m_mapWidget, &InteractiveMapWidget::mapRightClicked,
                this, &TestPainterWindow::onMapRightClicked);

        // 连接任务切换信号，控制按钮显示/隐藏
        connect(m_taskManager, &TaskManager::currentTaskChanged,
                this, &TestPainterWindow::onCurrentTaskChanged);

        qDebug() << "地图初始化完成";
    }

    void onCurrentTaskChanged(int taskId) {
        // 当任务切换时，根据是否有选中任务来显示/隐藏按钮
        if (taskId > 0 && m_taskManager->currentTask()) {
            // 有选中的任务，显示按钮
            m_buttonContainer->show();
            qDebug() << QString("任务 #%1 已选中，显示操作按钮").arg(taskId);
        } else {
            // 没有选中任务，隐藏按钮
            m_buttonContainer->hide();
            qDebug() << "未选中任务，隐藏操作按钮";
        }
    }

private slots:
    void startPlaceLoiter() {
        // 检查是否选中了任务
        if (!m_taskManager->currentTask()) {
            QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
            return;
        }

        // 开始单次放置盘旋点
        m_currentMode = MODE_LOITER;
        m_mapWidget->setClickEnabled(true);
        m_mapWidget->setCustomCursor("image/pin.png");  // 设置光标为盘旋点图标
        m_mapWidget->setStatusText("放置盘旋点 - 点击地图任意位置（右键取消）", "rgba(212, 237, 218, 220)");
        qDebug() << "开始单次放置盘旋点";
    }

    void startPlaceNoFly() {
        // 检查是否选中了任务
        if (!m_taskManager->currentTask()) {
            QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
            return;
        }

        // 开始单次放置禁飞区域
        m_currentMode = MODE_NOFLY;
        m_mapWidget->setClickEnabled(true);
        resetNoFlyZoneDrawing();
        m_mapWidget->setStatusText("放置禁飞区域 - 点击中心点，移动鼠标确定半径（右键取消）", "rgba(255, 243, 205, 220)");
        qDebug() << "开始单次放置禁飞区域";
    }

    void startPlaceUAV() {
        // 检查是否选中了任务
        if (!m_taskManager->currentTask()) {
            QMessageBox::warning(this, "未选择任务", "请先创建或选择一个任务！");
            return;
        }

        // 开始单次放置无人机
        m_currentMode = MODE_UAV;
        m_isInNoFlyZone = false;  // 重置禁飞区状态
        m_mapWidget->setClickEnabled(true);
        m_mapWidget->setCustomCursor("image/uav.png", 12, 12);  // 设置光标为无人机图标，中心对齐
        QString colorName = m_uavColorCombo->currentText();
        m_mapWidget->setStatusText(QString("放置无人机 (%1) - 点击地图任意位置（右键取消）").arg(colorName), "rgba(230, 230, 255, 220)");
        qDebug() << "开始单次放置无人机 - 颜色:" << colorName;
    }

    void returnToNormalMode() {
        // 返回普通浏览模式
        m_currentMode = MODE_NORMAL;
        m_mapWidget->setClickEnabled(true);  // 保持启用以支持元素点击
        m_mapWidget->restoreDefaultCursor();  // 恢复默认箭头光标
        resetNoFlyZoneDrawing();
        resetPolygonDrawing();
        m_mapWidget->setStatusText("普通浏览 - 点击元素查看详情，左键拖动，滚轮缩放");
        qDebug() << "返回普通浏览模式";
    }

    void onMapClicked(const QMapLibre::Coordinate &coord) {
        if (!m_painter) {
            return;
        }

        qDebug() << QString("地图被点击: (%1, %2)").arg(coord.first).arg(coord.second);

        // 如果在普通浏览模式，检查是否点击到元素
        if (m_currentMode == MODE_NORMAL) {
            // 使用 TaskManager 的方法，只查找可见任务中的元素
            // 使用缩放相关的阈值（基准阈值100米）
            double threshold = getZoomDependentThreshold(100.0);
            const ElementInfo* element = m_taskManager->findVisibleElementNear(coord, threshold);
            if (element) {
                // 找到元素，显示详情
                QPoint screenPos = QCursor::pos();
                m_detailWidget->showElement(element, screenPos);
                qDebug() << "点击到可见任务的元素，显示详情";
            } else {
                // 未找到元素，隐藏详情窗口
                m_detailWidget->hide();
            }
            return;
        }

        // 其他模式的处理
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

    void onMapMouseMoved(const QMapLibre::Coordinate &coord) {
        if (!m_painter) {
            return;
        }

        // 无人机模式：检测是否在当前任务的禁飞区内
        if (m_currentMode == MODE_UAV) {
            bool inNoFlyZone = m_taskManager->isInCurrentTaskNoFlyZone(coord);

            // 如果状态变化，更新光标
            if (inNoFlyZone != m_isInNoFlyZone) {
                m_isInNoFlyZone = inNoFlyZone;

                if (inNoFlyZone) {
                    // 进入禁飞区，显示禁止光标
                    m_mapWidget->setForbiddenCursor();
                    qDebug() << "鼠标进入当前任务的禁飞区";
                } else {
                    // 离开禁飞区，恢复无人机光标
                    m_mapWidget->setCustomCursor("image/uav.png", 12, 12);
                    qDebug() << "鼠标离开当前任务的禁飞区";
                }
            }
        }
        // 禁飞区模式的鼠标移动处理
        else if (m_currentMode == MODE_NOFLY && m_noFlyZoneCenterSet) {
            // 计算当前鼠标位置到中心点的距离作为半径
            double radius = calculateDistance(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second,
                                              coord.first, coord.second);

            // 绘制预览圆形
            m_painter->drawPreviewNoFlyZone(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

            // 更新状态提示
            m_mapWidget->setStatusText(QString("禁飞区预览 - 中心: (%1, %2), 半径: %3米 (点击确定/右键取消)")
                                       .arg(m_noFlyZoneCenter.first, 0, 'f', 5)
                                       .arg(m_noFlyZoneCenter.second, 0, 'f', 5)
                                       .arg(radius, 0, 'f', 1),
                                       "rgba(255, 243, 205, 220)");
        }
        // 多边形模式的鼠标移动处理
        else if (m_currentMode == MODE_POLYGON && !m_polygonPoints.isEmpty()) {
            // 绘制从最后一个点到当前鼠标位置的动态线
            m_painter->updateDynamicLine(m_polygonPoints.last(), coord);
        }
    }

    void onMapRightClicked() {
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

    void addLoiterPointAt(double lat, double lon) {
        auto id = m_taskManager->addLoiterPoint(lat, lon);
        qDebug() << QString("在 (%1, %2) 添加盘旋点到任务 #%3，ID: %4")
                    .arg(lat).arg(lon).arg(m_taskManager->currentTaskId()).arg(id);

        // 单次放置完成，返回普通模式
        returnToNormalMode();
    }

    void addUAVAt(double lat, double lon) {
        // 检查是否在当前任务的禁飞区内
        QMapLibre::Coordinate coord(lat, lon);
        if (m_taskManager->isInCurrentTaskNoFlyZone(coord)) {
            // 在禁飞区内，弹出警告对话框
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("无法放置");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("无法在当前任务的禁飞区域内放置无人机！");
            msgBox.setInformativeText("请选择禁飞区域以外的位置。");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();

            qDebug() << QString("尝试在当前任务的禁飞区内放置无人机 (%1, %2)，已阻止").arg(lat).arg(lon);
            // 保持在无人机放置模式，不返回普通模式
            return;
        }

        // 不在禁飞区，正常放置
        QString color = m_uavColorCombo->currentData().toString();
        auto id = m_taskManager->addUAV(lat, lon, color);
        qDebug() << QString("在 (%1, %2) 添加无人机 (%3) 到任务 #%4，ID: %5")
                    .arg(lat).arg(lon).arg(color).arg(m_taskManager->currentTaskId()).arg(id);

        // 单次放置完成，返回普通模式
        returnToNormalMode();
    }

    void handleNoFlyZoneClick(double lat, double lon) {
        if (!m_noFlyZoneCenterSet) {
            // 第一次点击：设置中心点
            m_noFlyZoneCenter = QMapLibre::Coordinate(lat, lon);
            m_noFlyZoneCenterSet = true;
            qDebug() << QString("设置禁飞区中心点: (%1, %2)").arg(lat).arg(lon);
            m_mapWidget->setStatusText(QString("禁飞区中心已设置: (%1, %2) - 移动鼠标确定半径（右键取消）")
                                       .arg(lat, 0, 'f', 5).arg(lon, 0, 'f', 5),
                                       "rgba(255, 243, 205, 220)");
        } else {
            // 第二次点击：确定半径并创建禁飞区
            double radius = calculateDistance(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, lat, lon);

            // 检查是否与当前任务的无人机冲突
            QVector<const MapElement*> conflictUAVs = m_taskManager->checkNoFlyZoneConflictWithUAVs(
                m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

            if (!conflictUAVs.isEmpty()) {
                // 有冲突，弹出警告对话框
                QMessageBox msgBox(this);
                msgBox.setWindowTitle("无法放置禁飞区");
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText(QString("该禁飞区域会覆盖 %1 架当前任务的无人机！").arg(conflictUAVs.size()));
                msgBox.setInformativeText("请调整禁飞区位置或半径，或先移除冲突的无人机。");
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();

                qDebug() << QString("尝试放置禁飞区，但与 %1 架无人机冲突，已阻止")
                            .arg(conflictUAVs.size());

                // 清除预览，但保持在禁飞区放置模式，让用户重新选择
                m_painter->clearPreview();
                resetNoFlyZoneDrawing();
                m_mapWidget->setStatusText("放置禁飞区 - 点击中心点，移动鼠标确定半径（右键取消）", "rgba(255, 243, 205, 220)");
                return;
            }

            // 清除预览
            m_painter->clearPreview();

            // 弹出地形特征选择对话框
            RegionFeatureDialog featureDialog(this);
            if (featureDialog.exec() == QDialog::Accepted) {
                // 用户选择了地形特征，创建禁飞区
                auto terrainType = featureDialog.getSelectedTerrain();
                auto id = m_taskManager->addNoFlyZone(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);

                // 设置地形特征
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

            // 单次放置完成，返回普通模式
            returnToNormalMode();
        }
    }

    void resetNoFlyZoneDrawing() {
        if (m_painter) {
            m_painter->clearPreview();
        }
        m_noFlyZoneCenterSet = false;
        m_noFlyZoneCenter = QMapLibre::Coordinate(0, 0);
    }

    // 计算两点之间的距离（米）
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
        const double EARTH_RADIUS = 6378137.0; // 地球半径（米）

        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;

        double a = qSin(dLat / 2) * qSin(dLat / 2) +
                   qCos(lat1 * M_PI / 180.0) * qCos(lat2 * M_PI / 180.0) *
                   qSin(dLon / 2) * qSin(dLon / 2);

        double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
        return EARTH_RADIUS * c;
    }

    /**
     * @brief 根据地图缩放级别计算交互阈值（米）
     * @param baseThreshold 基准阈值（在zoom=12时的阈值，单位：米）
     * @return 根据当前缩放级别调整后的阈值（米）
     *
     * 缩放级别越大（越近），阈值越小；缩放级别越小（越远），阈值越大
     * 公式：threshold = baseThreshold * 2^(12 - zoom)
     *
     * 示例（baseThreshold=50米）：
     * - zoom 8  -> 800米   (超大视角)
     * - zoom 10 -> 200米   (大视角)
     * - zoom 12 -> 50米    (中等视角，基准)
     * - zoom 14 -> 12.5米  (近距离)
     * - zoom 16 -> 3.125米 (精细操作)
     */
    double getZoomDependentThreshold(double baseThreshold = 50.0) {
        if (!m_mapWidget || !m_mapWidget->map()) {
            return baseThreshold; // 如果无法获取缩放级别，返回基准值
        }

        double zoom = m_mapWidget->map()->zoom();
        double threshold = baseThreshold * pow(2.0, 12.0 - zoom);

        return threshold;
    }

    void clearAll() {
        if (!m_taskManager) {
            qWarning() << "任务管理器未初始化";
            return;
        }

        if (!m_taskManager->currentTask()) {
            QMessageBox::warning(this, "未选择任务", "请先选择一个任务！");
            return;
        }

        // 弹出确认对话框
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

    // 多边形绘制相关方法
    void startDrawPolygon() {
        // 检查是否选中了任务
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

    void handlePolygonClick(double lat, double lon) {
        QMapLibre::Coordinate clickedPoint(lat, lon);

        // 检查是否点击到了起点（闭合多边形）
        if (m_polygonPoints.size() >= 3) {
            double distanceToStart = calculateDistance(
                clickedPoint.first, clickedPoint.second,
                m_polygonPoints.first().first, m_polygonPoints.first().second
            );

            // 根据缩放级别计算闭合阈值（基准阈值50米）
            double threshold = getZoomDependentThreshold(50.0);

            qDebug() << QString("多边形闭合检测: 距离起点 %1 米, 阈值 %2 米 (缩放级别 %3)")
                        .arg(distanceToStart, 0, 'f', 2)
                        .arg(threshold, 0, 'f', 2)
                        .arg(m_mapWidget->map()->zoom(), 0, 'f', 2);

            // 如果距离起点小于动态阈值，则闭合多边形
            if (distanceToStart < threshold) {
                qDebug() << "点击起点，闭合多边形";
                finishPolygon();
                return;
            }
        }

        // 添加新顶点
        m_polygonPoints.append(clickedPoint);
        qDebug() << QString("添加多边形顶点 #%1: (%2, %3)")
                        .arg(m_polygonPoints.size())
                        .arg(lat).arg(lon);

        // 清除动态线（因为点已经固定了）
        m_painter->clearDynamicLine();

        // 更新预览线
        if (m_polygonPoints.size() >= 2) {
            m_painter->drawPreviewLines(m_polygonPoints);
        }

        // 更新状态文本
        m_mapWidget->setStatusText(
            QString("绘制多边形 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                .arg(m_polygonPoints.size()),
            "rgba(255, 243, 205, 220)"
        );
    }

    void handlePolygonUndo() {
        if (m_polygonPoints.isEmpty()) {
            qDebug() << "没有顶点，取消多边形绘制";
            returnToNormalMode();
            return;
        }

        m_polygonPoints.removeLast();
        qDebug() << QString("回退一个顶点，剩余 %1 个").arg(m_polygonPoints.size());

        // 如果没有顶点了，清除预览并回到普通模式
        if (m_polygonPoints.isEmpty()) {
            m_painter->clearPolygonPreview();
            returnToNormalMode();
        } else if (m_polygonPoints.size() == 1) {
            // 只剩一个点，清除线条
            m_painter->clearPolygonPreview();
            m_mapWidget->setStatusText(
                QString("绘制多边形 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                    .arg(m_polygonPoints.size()),
                "rgba(255, 243, 205, 220)"
            );
        } else {
            // 更新预览线
            m_painter->drawPreviewLines(m_polygonPoints);
            m_mapWidget->setStatusText(
                QString("绘制多边形 - 已添加 %1 个顶点（点击起点闭合，右键回退，ESC取消）")
                    .arg(m_polygonPoints.size()),
                "rgba(255, 243, 205, 220)"
            );
        }
    }

    void finishPolygon() {
        if (m_polygonPoints.size() < 3) {
            qWarning() << "多边形至少需要3个顶点";
            return;
        }

        // 弹出地形特征选择对话框
        RegionFeatureDialog featureDialog(this);
        if (featureDialog.exec() == QDialog::Accepted) {
            // 用户选择了地形特征，创建多边形
            auto terrainType = featureDialog.getSelectedTerrain();
            auto id = m_taskManager->addPolygon(m_polygonPoints);

            // 设置地形特征
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

        // 清除预览线并返回普通模式
        m_painter->clearPolygonPreview();
        m_polygonPoints.clear();
        returnToNormalMode();
    }

    void resetPolygonDrawing() {
        m_polygonPoints.clear();
        if (m_painter) {
            m_painter->clearPolygonPreview();
            m_painter->clearDynamicLine();
        }
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape && m_currentMode == MODE_POLYGON) {
            qDebug() << "按下ESC，取消多边形绘制";
            returnToNormalMode();
        }
        QWidget::keyPressEvent(event);
    }

    // eventFilter 不再需要，因为 TaskListWidget 自己处理 hover 事件

private:
    enum InteractionMode {
        MODE_NORMAL = 0,
        MODE_LOITER = 1,
        MODE_NOFLY = 2,
        MODE_POLYGON = 3,
        MODE_UAV = 4
    };

    InteractiveMapWidget *m_mapWidget = nullptr;
    MapPainter *m_painter = nullptr;
    TaskManager *m_taskManager = nullptr;
    TaskListWidget *m_taskListWidget = nullptr;
    ElementDetailWidget *m_detailWidget = nullptr;
    QWidget *m_buttonContainer = nullptr;
    QComboBox *m_uavColorCombo = nullptr;
    InteractionMode m_currentMode = MODE_NORMAL;
    bool m_mapInitialized = false;

    // 禁飞区绘制状态
    bool m_noFlyZoneCenterSet = false;
    QMapLibre::Coordinate m_noFlyZoneCenter;

    // 多边形绘制状态
    QMapLibre::Coordinates m_polygonPoints;  // 已点击的顶点

    // 无人机模式状态
    bool m_isInNoFlyZone = false;  // 当前鼠标是否在禁飞区内
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    qDebug() << "==============================================";
    qDebug() << "MapPainter 交互式测试程序启动";
    qDebug() << "==============================================";

    TestPainterWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
