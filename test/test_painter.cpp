// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "../MapPainter.h"
#include "../InteractiveMapWidget.h"
#include "../ElementDetailWidget.h"
#include <QMapLibre/Map>
#include <QMapLibre/Settings>

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QShowEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QtMath>

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

private:
    void setupUI() {
        // 创建交互式地图 Widget（占据整个窗口）
        QMapLibre::Settings settings;
        settings.setDefaultZoom(12);
        settings.setDefaultCoordinate(QMapLibre::Coordinate(39.9, 116.4)); // 北京

        m_mapWidget = new InteractiveMapWidget(settings);

        // 使用无边距布局，让地图填满整个窗口
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addWidget(m_mapWidget);

        // 创建浮动在地图上方的按钮容器
        auto *buttonContainer = new QWidget(m_mapWidget);
        auto *buttonLayout = new QHBoxLayout(buttonContainer);
        buttonLayout->setContentsMargins(10, 10, 10, 10);

        auto *loiterBtn = new QPushButton("放置盘旋点", buttonContainer);
        auto *noFlyBtn = new QPushButton("放置禁飞区域", buttonContainer);
        auto *polygonBtn = new QPushButton("绘制多边形", buttonContainer);
        auto *uavBtn = new QPushButton("放置无人机", buttonContainer);
        auto *clearBtn = new QPushButton("清除全部", buttonContainer);

        // 创建 UAV 颜色选择下拉框
        m_uavColorCombo = new QComboBox(buttonContainer);
        m_uavColorCombo->addItem("黑色", "black");
        m_uavColorCombo->addItem("红色", "red");
        m_uavColorCombo->addItem("蓝色", "blue");
        m_uavColorCombo->addItem("紫色", "purple");
        m_uavColorCombo->addItem("绿色", "green");
        m_uavColorCombo->addItem("黄色", "yellow");
        m_uavColorCombo->setCurrentIndex(0); // 默认黑色

        // 设置按钮样式
        QString buttonStyle =
            "QPushButton {"
            "  background-color: rgba(255, 255, 255, 230);"
            "  border: 1px solid #ccc;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  font-size: 13px;"
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

        setWindowTitle("MapPainter");
        resize(1000, 700);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        updateButtonPosition();
    }

    void updateButtonPosition() {
        if (m_buttonContainer && m_mapWidget) {
            // 按钮容器放在顶部，横跨整个宽度
            m_buttonContainer->setGeometry(0, 0, m_mapWidget->width(), 60);
            m_buttonContainer->raise();  // 确保显示在最上层
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

        // 创建详情窗口
        m_detailWidget = new ElementDetailWidget(this);

        // 连接地图点击信号
        connect(m_mapWidget, &InteractiveMapWidget::mapClicked,
                this, &TestPainterWindow::onMapClicked);
        connect(m_mapWidget, &InteractiveMapWidget::mapMouseMoved,
                this, &TestPainterWindow::onMapMouseMoved);
        connect(m_mapWidget, &InteractiveMapWidget::mapRightClicked,
                this, &TestPainterWindow::onMapRightClicked);

        qDebug() << "地图初始化完成";
    }

private slots:
    void startPlaceLoiter() {
        // 开始单次放置盘旋点
        m_currentMode = MODE_LOITER;
        m_mapWidget->setClickEnabled(true);
        m_mapWidget->setStatusText("放置盘旋点 - 点击地图任意位置（右键取消）", "rgba(212, 237, 218, 220)");
        qDebug() << "开始单次放置盘旋点";
    }

    void startPlaceNoFly() {
        // 开始单次放置禁飞区域
        m_currentMode = MODE_NOFLY;
        m_mapWidget->setClickEnabled(true);
        resetNoFlyZoneDrawing();
        m_mapWidget->setStatusText("放置禁飞区域 - 点击中心点，移动鼠标确定半径（右键取消）", "rgba(255, 243, 205, 220)");
        qDebug() << "开始单次放置禁飞区域";
    }

    void startPlaceUAV() {
        // 开始单次放置无人机
        m_currentMode = MODE_UAV;
        m_mapWidget->setClickEnabled(true);
        QString colorName = m_uavColorCombo->currentText();
        m_mapWidget->setStatusText(QString("放置无人机 (%1) - 点击地图任意位置（右键取消）").arg(colorName), "rgba(230, 230, 255, 220)");
        qDebug() << "开始单次放置无人机 - 颜色:" << colorName;
    }

    void returnToNormalMode() {
        // 返回普通浏览模式
        m_currentMode = MODE_NORMAL;
        m_mapWidget->setClickEnabled(true);  // 保持启用以支持元素点击
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
            const ElementInfo* element = m_painter->findElementNear(coord, 100.0);
            if (element) {
                // 找到元素，显示详情
                QPoint screenPos = QCursor::pos();
                m_detailWidget->showElement(element, screenPos);
                qDebug() << "点击到元素，显示详情";
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

        // 禁飞区模式的鼠标移动处理
        if (m_currentMode == MODE_NOFLY && m_noFlyZoneCenterSet) {
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
        auto id = m_painter->drawLoiterPoint(lat, lon);
        qDebug() << QString("在 (%1, %2) 添加盘旋点，ID: %3").arg(lat).arg(lon).arg(id);

        // 单次放置完成，返回普通模式
        returnToNormalMode();
    }

    void addUAVAt(double lat, double lon) {
        QString color = m_uavColorCombo->currentData().toString();
        auto id = m_painter->drawUAV(lat, lon, color);
        qDebug() << QString("在 (%1, %2) 添加无人机 (%3)，ID: %4").arg(lat).arg(lon).arg(color).arg(id);

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

            // 清除预览
            m_painter->clearPreview();

            // 创建正式的禁飞区
            auto id = m_painter->drawNoFlyZone(m_noFlyZoneCenter.first, m_noFlyZoneCenter.second, radius);
            qDebug() << QString("创建禁飞区: 中心(%1, %2), 半径 %3m, ID: %4")
                        .arg(m_noFlyZoneCenter.first).arg(m_noFlyZoneCenter.second).arg(radius).arg(id);

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

    void clearAll() {
        if (!m_painter) {
            qWarning() << "画家对象未初始化";
            return;
        }

        m_painter->clearAll();
        resetNoFlyZoneDrawing();
        resetPolygonDrawing();
        qDebug() << "已清除所有标注";
    }

    // 多边形绘制相关方法
    void startDrawPolygon() {
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

            // 如果距离起点小于50米，则闭合多边形
            if (distanceToStart < 50.0) {
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

        // 绘制最终的多边形区域
        m_painter->drawPolygonArea(m_polygonPoints);

        // 清除预览线并返回普通模式
        m_painter->clearPolygonPreview();
        m_polygonPoints.clear();
        returnToNormalMode();

        qDebug() << "多边形绘制完成";
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

#include "test_painter.moc"
