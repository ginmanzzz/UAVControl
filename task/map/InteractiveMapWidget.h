// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef INTERACTIVEMAPWIDGET_H
#define INTERACTIVEMAPWIDGET_H

#include <QMapLibreWidgets/GLWidget>
#include <QMapLibre/Types>
#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLabel>

/**
 * @brief 交互式地图容器 - 包装 GLWidget 并支持鼠标点击获取坐标
 */
class InteractiveMapWidget : public QWidget {
    Q_OBJECT

public:
    explicit InteractiveMapWidget(const QMapLibre::Settings &settings, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_clickEnabled(false)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_glWidget = new QMapLibre::GLWidget(settings);
        m_glWidget->installEventFilter(this);
        // 启用鼠标追踪以接收 MouseMove 事件
        m_glWidget->setMouseTracking(true);
        layout->addWidget(m_glWidget);

        // 创建坐标显示标签（浮动在地图右下角）
        m_coordLabel = new QLabel(m_glWidget);
        m_coordLabel->setStyleSheet(
            "background-color: rgba(0, 0, 0, 180);"
            "color: white;"
            "padding: 8px 12px;"
            "border-radius: 6px;"
            "font-family: monospace;"
            "font-size: 12px;"
        );
        m_coordLabel->setText("经度: ---, 纬度: ---");
        m_coordLabel->adjustSize();

        // 创建状态提示标签（浮动在地图左下角）
        m_statusLabel = new QLabel(m_glWidget);
        m_statusLabel->setStyleSheet(
            "background-color: rgba(232, 244, 248, 220);"
            "color: #333;"
            "padding: 8px 12px;"
            "border-radius: 6px;"
            "font-size: 13px;"
        );
        m_statusLabel->setText("普通浏览 - 左键拖动，滚轮缩放");
        m_statusLabel->adjustSize();

        updateOverlayPositions();
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        updateOverlayPositions();
    }

    /**
     * @brief 更新状态提示文字
     */
    void setStatusText(const QString &text, const QString &bgColor = "rgba(232, 244, 248, 220)") {
        m_statusLabel->setText(text);
        m_statusLabel->setStyleSheet(QString(
            "background-color: %1;"
            "color: #333;"
            "padding: 8px 12px;"
            "border-radius: 6px;"
            "font-size: 13px;"
        ).arg(bgColor));
        m_statusLabel->adjustSize();
        updateOverlayPositions();
    }

    /**
     * @brief 获取地图对象
     */
    QMapLibre::Map* map() {
        return m_glWidget->map();
    }

    /**
     * @brief 启用/禁用点击监听
     */
    void setClickEnabled(bool enabled) {
        m_clickEnabled = enabled;
    }

    bool isClickEnabled() const {
        return m_clickEnabled;
    }

    /**
     * @brief 设置地图光标为自定义图标
     * @param iconPath 图标文件路径
     * @param hotX 热点 X 坐标（默认图标中心）
     * @param hotY 热点 Y 坐标（默认图标底部中心，适合 pin 类图标）
     */
    void setCustomCursor(const QString &iconPath, int hotX = -1, int hotY = -1) {
        QPixmap pixmap(iconPath);
        if (pixmap.isNull()) {
            qWarning() << "无法加载光标图标:" << iconPath;
            return;
        }

        // 缩放到合适大小（24x24 像素用于光标）
        if (pixmap.width() > 24 || pixmap.height() > 24) {
            pixmap = pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // 如果没有指定热点，使用默认值
        if (hotX < 0) hotX = pixmap.width() / 2;
        if (hotY < 0) hotY = pixmap.height();  // 底部中心，适合 pin

        QCursor cursor(pixmap, hotX, hotY);
        m_glWidget->setCursor(cursor);
    }

    /**
     * @brief 恢复默认光标
     */
    void restoreDefaultCursor() {
        m_glWidget->setCursor(Qt::ArrowCursor);
    }

    /**
     * @brief 设置禁止光标（带红色斜杠的圆圈）
     */
    void setForbiddenCursor() {
        m_glWidget->setCursor(Qt::ForbiddenCursor);
    }

signals:
    /**
     * @brief 地图被点击时发出信号
     * @param coordinate 点击位置的经纬度坐标
     */
    void mapClicked(const QMapLibre::Coordinate &coordinate);

    /**
     * @brief 地图上鼠标移动时发出信号
     * @param coordinate 鼠标位置的经纬度坐标
     */
    void mapMouseMoved(const QMapLibre::Coordinate &coordinate);

    /**
     * @brief 地图被右键点击时发出信号
     */
    void mapRightClicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj != m_glWidget) {
            return QWidget::eventFilter(obj, event);
        }

        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            m_mousePressed = true;
            m_mousePressPos = mouseEvent->pos();

            if (mouseEvent->button() == Qt::RightButton && m_clickEnabled) {
                // 右键点击 - 取消操作
                emit mapRightClicked();
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton && m_mousePressed) {
                // 检查是否是点击（而不是拖动）
                int dx = mouseEvent->pos().x() - m_mousePressPos.x();
                int dy = mouseEvent->pos().y() - m_mousePressPos.y();
                int distance = dx * dx + dy * dy;

                // 如果移动距离小于5像素，认为是点击
                if (distance < 25) {
                    QMapLibre::Coordinate coord = m_glWidget->map()->coordinateForPixel(mouseEvent->pos());
                    emit mapClicked(coord);
                }
            }
            m_mousePressed = false;
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            // 鼠标移动 - 转换坐标
            QMapLibre::Coordinate coord = m_glWidget->map()->coordinateForPixel(mouseEvent->pos());

            // 更新坐标显示（始终显示）
            updateCoordLabel(coord);

            // 如果启用了点击，发出移动信号（用于禁飞区预览）
            if (m_clickEnabled) {
                emit mapMouseMoved(coord);
            }
        }

        return QWidget::eventFilter(obj, event);
    }

private:
    void updateOverlayPositions() {
        if (!m_glWidget) return;

        int margin = 10;

        // 坐标标签 - 右下角
        if (m_coordLabel) {
            int x = m_glWidget->width() - m_coordLabel->width() - margin;
            int y = m_glWidget->height() - m_coordLabel->height() - margin;
            m_coordLabel->move(x, y);
            m_coordLabel->raise();
        }

        // 状态标签 - 左下角
        if (m_statusLabel) {
            int x = margin;
            int y = m_glWidget->height() - m_statusLabel->height() - margin;
            m_statusLabel->move(x, y);
            m_statusLabel->raise();
        }
    }

    void updateCoordLabel(const QMapLibre::Coordinate &coord) {
        QString text = QString("经度: %1, 纬度: %2")
                       .arg(coord.second, 0, 'f', 6)
                       .arg(coord.first, 0, 'f', 6);
        m_coordLabel->setText(text);
        m_coordLabel->adjustSize();
        updateOverlayPositions();
    }

private:
    QMapLibre::GLWidget *m_glWidget;
    QLabel *m_coordLabel;
    QLabel *m_statusLabel;
    bool m_clickEnabled;
    bool m_mousePressed = false;
    QPoint m_mousePressPos;
};

#endif // INTERACTIVEMAPWIDGET_H
