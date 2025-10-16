// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "RegionDetailWidget.h"
#include "TaskManager.h"
#include "Task.h"
#include <QtMath>

RegionDetailWidget::RegionDetailWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentRegion(nullptr)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_contentWidget = new QWidget(this);
    m_contentWidget->setStyleSheet(
        "QWidget {"
        "  background-color: rgba(255, 255, 255, 245);"
        "  border: 1px solid #ccc;"
        "  border-radius: 6px;"
        "}"
    );

    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(12, 8, 12, 8);
    m_contentLayout->setSpacing(2);

    layout->addWidget(m_contentWidget);

    hide();
}

void RegionDetailWidget::showRegion(const RegionInfo *info, const QPoint &screenPos)
{
    if (!info) {
        hide();
        return;
    }

    m_currentRegion = info;

    // 清除旧内容
    QLayoutItem *item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // 添加标题
    QString title;
    switch (info->type) {
        case RegionType::LoiterPoint:
            title = "📍 盘旋点";
            break;
        case RegionType::UAV:
            title = QString("🛩️ 无人机 (%1)").arg(getColorName(info->color));
            break;
        case RegionType::NoFlyZone:
            title = "🚫 禁飞区域";
            break;
        case RegionType::TaskRegion:
            title = "🔷 任务区域";
            break;
    }

    addTitle(title);

    // 显示关联的任务列表
    if (m_taskManager) {
        QVector<Task*> referencingTasks = m_taskManager->getTasksReferencingRegion(info->regionId);

        if (referencingTasks.isEmpty()) {
            addInfoLine("任务", "无任务关联");
        } else {
            // 以列表形式显示所有关联任务
            for (int i = 0; i < referencingTasks.size(); ++i) {
                Task *task = referencingTasks[i];
                QString label = (i == 0) ? "任务" : "";  // 第一行显示"任务"标签，后续行留空对齐
                addInfoLine(label, QString("#%1 %2").arg(task->id()).arg(task->name()));
            }
        }
    } else {
        addInfoLine("任务", "（管理器未初始化）");
    }

    // 根据类型显示不同内容
    switch (info->type) {
        case RegionType::LoiterPoint:
        case RegionType::UAV:
            addInfoLine("经度", QString("%1°").arg(info->coordinate.second, 0, 'f', 6));
            addInfoLine("纬度", QString("%1°").arg(info->coordinate.first, 0, 'f', 6));
            break;

        case RegionType::NoFlyZone: {
            addInfoLine("中心经度", QString("%1°").arg(info->coordinate.second, 0, 'f', 6));
            addInfoLine("中心纬度", QString("%1°").arg(info->coordinate.first, 0, 'f', 6));
            addInfoLine("半径", QString("%1 米").arg(info->radius, 0, 'f', 1));
            double areaKm2 = M_PI * info->radius * info->radius / 1000000.0;
            addInfoLine("区域面积", QString("%1 km²").arg(areaKm2, 0, 'f', 3));
            // 地形特征下拉选择
            addTerrainLine("地形特征", info->terrainType);
            break;
        }

        case RegionType::TaskRegion: {
            addInfoLine("顶点数量", QString("%1").arg(info->vertices.size()));
            for (int i = 0; i < info->vertices.size(); ++i) {
                addInfoLine(QString("顶点%1").arg(i + 1),
                    QString("(%1°, %2°)")
                    .arg(info->vertices[i].first, 0, 'f', 5)
                    .arg(info->vertices[i].second, 0, 'f', 5));
            }
            double areaKm2 = calculateTaskRegionArea(info->vertices) / 1000000.0;
            addInfoLine("区域面积", QString("%1 km²").arg(areaKm2, 0, 'f', 3));
            // 地形特征下拉选择
            addTerrainLine("地形特征", info->terrainType);
            break;
        }
    }

    // 添加删除按钮（所有区域类型都支持）
    addDeleteButton();

    adjustSize();
    move(screenPos.x() + 20, screenPos.y() + 20);
    show();
    raise();
}

void RegionDetailWidget::addTitle(const QString &text)
{
    // 标题容器（带蓝色边框）
    auto *titleContainer = new QWidget(m_contentWidget);
    titleContainer->setStyleSheet(
        "QWidget {"
        "  background-color: #e3f2fd;"
        "  border: 1px solid #4a90e2;"
        "  border-radius: 4px;"
        "  padding: 6px 8px;"
        "}"
    );

    auto *titleLayout = new QVBoxLayout(titleContainer);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    auto *titleLabel = new QLabel(text, titleContainer);
    titleLabel->setStyleSheet(
        "font-weight: bold; "
        "font-size: 13px; "
        "color: #2c3e50; "
        "background-color: transparent;"
        "border: none;"
    );
    titleLayout->addWidget(titleLabel);

    m_contentLayout->addWidget(titleContainer);
    m_contentLayout->addSpacing(8);
}

void RegionDetailWidget::addInfoLine(const QString &label, const QString &value)
{
    auto *lineWidget = new QWidget(m_contentWidget);
    auto *lineLayout = new QHBoxLayout(lineWidget);
    lineLayout->setContentsMargins(0, 2, 0, 2);
    lineLayout->setSpacing(8);

    auto *labelWidget = new QLabel(label + ":", lineWidget);
    labelWidget->setStyleSheet(
        "font-size: 12px; "
        "color: #555; "
        "font-weight: normal;"
    );
    labelWidget->setMinimumWidth(70);

    auto *valueWidget = new QLabel(value, lineWidget);
    valueWidget->setStyleSheet(
        "font-size: 12px; "
        "color: #333;"
    );

    lineLayout->addWidget(labelWidget);
    lineLayout->addWidget(valueWidget);
    lineLayout->addStretch();

    m_contentLayout->addWidget(lineWidget);
}

void RegionDetailWidget::addTerrainLine(const QString &label, TerrainType currentTerrain)
{
    auto *lineWidget = new QWidget(m_contentWidget);
    auto *lineLayout = new QHBoxLayout(lineWidget);
    lineLayout->setContentsMargins(0, 2, 0, 2);
    lineLayout->setSpacing(8);

    auto *labelWidget = new QLabel(label + ":", lineWidget);
    labelWidget->setStyleSheet(
        "font-size: 12px; "
        "color: #555; "
        "font-weight: normal;"
    );
    labelWidget->setMinimumWidth(70);

    // 地形下拉选择框
    auto *terrainCombo = new QComboBox(lineWidget);
    terrainCombo->addItem("平原", static_cast<int>(TerrainType::Plain));
    terrainCombo->addItem("丘陵", static_cast<int>(TerrainType::Hills));
    terrainCombo->addItem("山地", static_cast<int>(TerrainType::Mountain));
    terrainCombo->addItem("高山地", static_cast<int>(TerrainType::HighMountain));
    terrainCombo->setCurrentIndex(static_cast<int>(currentTerrain));
    terrainCombo->setStyleSheet(
        "QComboBox {"
        "  font-size: 12px;"
        "  color: #333;"
        "  padding: 2px 6px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 3px;"
        "  background-color: white;"
        "}"
        "QComboBox:hover {"
        "  border: 1px solid #999;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "}"
    );

    connect(terrainCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, terrainCombo](int index) {
        if (m_currentRegion) {
            TerrainType newTerrain = static_cast<TerrainType>(terrainCombo->currentData().toInt());
            emit terrainChanged(m_currentRegion->regionId, newTerrain);
        }
    });

    lineLayout->addWidget(labelWidget);
    lineLayout->addWidget(terrainCombo);
    lineLayout->addStretch();

    m_contentLayout->addWidget(lineWidget);
}

void RegionDetailWidget::addDeleteButton()
{
    // 添加分隔线
    auto *separator = new QWidget(m_contentWidget);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background-color: #e0e0e0; margin: 6px 0px;");
    m_contentLayout->addWidget(separator);

    // 删除按钮
    auto *buttonWidget = new QWidget(m_contentWidget);
    auto *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 4, 0, 0);
    buttonLayout->setSpacing(8);

    m_deleteButton = new QPushButton("删除", buttonWidget);
    m_deleteButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #f44336;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 6px 12px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #da190b;"
        "}"
    );
    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        if (m_currentRegion) {
            emit deleteRequested(m_currentRegion->regionId);
            hide();
        }
    });

    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();

    m_contentLayout->addWidget(buttonWidget);
}

QString RegionDetailWidget::getColorName(const QString &color) const
{
    if (color == "black") return "黑色";
    if (color == "red") return "红色";
    if (color == "blue") return "蓝色";
    if (color == "purple") return "紫色";
    if (color == "green") return "绿色";
    if (color == "yellow") return "黄色";
    return color;
}

double RegionDetailWidget::calculateTaskRegionArea(const QMapLibre::Coordinates &vertices) const
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
