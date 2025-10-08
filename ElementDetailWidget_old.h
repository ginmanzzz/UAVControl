// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef ELEMENTDETAILWIDGET_H
#define ELEMENTDETAILWIDGET_H

#include "MapPainter.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief 元素详情浮动窗口
 */
class ElementDetailWidget : public QWidget {
public:
    explicit ElementDetailWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_contentWidget = new QWidget(this);
        m_contentWidget->setStyleSheet(
            "QWidget {"
            "  background-color: rgba(255, 255, 255, 245);"
            "  border: 1px solid #4a90e2;"
            "  border-radius: 6px;"
            "}"
        );

        m_contentLayout = new QVBoxLayout(m_contentWidget);
        m_contentLayout->setContentsMargins(12, 8, 12, 8);
        m_contentLayout->setSpacing(2);

        layout->addWidget(m_contentWidget);

        hide();
    }

    void showElement(const ElementInfo *info, const QPoint &screenPos)
    {
        if (!info) {
            hide();
            return;
        }

        // 清除旧内容
        QLayoutItem *item;
        while ((item = m_contentLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        // 添加标题
        QString title;
        switch (info->type) {
            case ElementType::LoiterPoint:
                title = "📍 盘旋点";
                break;
            case ElementType::UAV:
                title = QString("🛩️ 无人机 (%1)").arg(getColorName(info->color));
                break;
            case ElementType::NoFlyZone:
                title = "🚫 禁飞区域";
                break;
            case ElementType::Polygon:
                title = "🔷 多边形区域";
                break;
        }

        addTitle(title);

        // 根据类型显示不同内容
        switch (info->type) {
            case ElementType::LoiterPoint:
            case ElementType::UAV:
                addInfoLine("经度", QString("%1°").arg(info->coordinate.second, 0, 'f', 6));
                addInfoLine("纬度", QString("%1°").arg(info->coordinate.first, 0, 'f', 6));
                break;

            case ElementType::NoFlyZone: {
                addInfoLine("中心经度", QString("%1°").arg(info->coordinate.second, 0, 'f', 6));
                addInfoLine("中心纬度", QString("%1°").arg(info->coordinate.first, 0, 'f', 6));
                addInfoLine("半径", QString("%1 米").arg(info->radius, 0, 'f', 1));
                double areaKm2 = M_PI * info->radius * info->radius / 1000000.0;
                addInfoLine("区域面积", QString("%1 km²").arg(areaKm2, 0, 'f', 3));
                break;
            }

            case ElementType::Polygon: {
                addInfoLine("顶点数量", QString("%1").arg(info->vertices.size()));
                for (int i = 0; i < info->vertices.size(); ++i) {
                    addInfoLine(QString("顶点%1").arg(i + 1),
                        QString("(%1°, %2°)")
                        .arg(info->vertices[i].first, 0, 'f', 5)
                        .arg(info->vertices[i].second, 0, 'f', 5));
                }
                double areaKm2 = calculatePolygonArea(info->vertices) / 1000000.0;
                addInfoLine("区域面积", QString("%1 km²").arg(areaKm2, 0, 'f', 3));
                break;
            }
        }

        adjustSize();
        move(screenPos.x() + 20, screenPos.y() + 20);
        show();
        raise();
    }

private:
    void addTitle(const QString &text)
    {
        auto *titleLabel = new QLabel(text, m_contentWidget);
        titleLabel->setStyleSheet(
            "font-weight: bold; "
            "font-size: 13px; "
            "color: #2c3e50; "
            "padding: 4px 0px 6px 0px;"
        );
        m_contentLayout->addWidget(titleLabel);
    }

    void addInfoLine(const QString &label, const QString &value)
    {
        auto *lineWidget = new QWidget(m_contentWidget);
        auto *lineLayout = new QHBoxLayout(lineWidget);
        lineLayout->setContentsMargins(0, 1, 0, 1);
        lineLayout->setSpacing(8);

        auto *labelWidget = new QLabel(label + ":", lineWidget);
        labelWidget->setStyleSheet(
            "font-size: 11px; "
            "color: #7f8c8d; "
            "font-weight: normal;"
        );
        labelWidget->setMinimumWidth(60);

        auto *valueWidget = new QLabel(value, lineWidget);
        valueWidget->setStyleSheet(
            "font-size: 11px; "
            "color: #2c3e50; "
            "font-family: monospace;"
        );

        lineLayout->addWidget(labelWidget);
        lineLayout->addWidget(valueWidget);
        lineLayout->addStretch();

        m_contentLayout->addWidget(lineWidget);
    }

    QString getColorName(const QString &color) const
    {
        if (color == "black") return "黑色";
        if (color == "red") return "红色";
        if (color == "blue") return "蓝色";
        if (color == "purple") return "紫色";
        if (color == "green") return "绿色";
        if (color == "yellow") return "黄色";
        return color;
    }

    // 计算多边形面积（平方米）
    double calculatePolygonArea(const QMapLibre::Coordinates &vertices) const
    {
        if (vertices.size() < 3) return 0.0;

        // 使用 Shoelace 公式计算近似面积
        double area = 0.0;
        int n = vertices.size();

        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            // 转换为近似米制坐标（粗略估算）
            double x1 = vertices[i].second * 111320.0 * cos(vertices[i].first * M_PI / 180.0);
            double y1 = vertices[i].first * 110540.0;
            double x2 = vertices[j].second * 111320.0 * cos(vertices[j].first * M_PI / 180.0);
            double y2 = vertices[j].first * 110540.0;

            area += x1 * y2 - x2 * y1;
        }

        return fabs(area) / 2.0;
    }

private:
    QWidget *m_contentWidget;
    QVBoxLayout *m_contentLayout;
};

#endif // ELEMENTDETAILWIDGET_H
