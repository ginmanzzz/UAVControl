// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "RegionFeatureDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

RegionFeatureDialog::RegionFeatureDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    // 默认选择平原
    m_terrainComboBox->setCurrentIndex(0);
}

RegionFeatureDialog::RegionFeatureDialog(TerrainType currentTerrain, QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    // 设置当前地形类型
    m_terrainComboBox->setCurrentIndex(static_cast<int>(currentTerrain));
}

void RegionFeatureDialog::setupUI()
{
    setWindowTitle("选择区域特征");
    setModal(true);
    setMinimumWidth(300);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题说明
    auto *titleLabel = new QLabel("请选择该区域的地形特征:", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    // 地形类型下拉框
    m_terrainComboBox = new QComboBox(this);
    m_terrainComboBox->addItem("平原", Plain);
    m_terrainComboBox->addItem("丘陵", Hills);
    m_terrainComboBox->addItem("山地", Mountain);
    m_terrainComboBox->addItem("高山地", HighMountain);
    m_terrainComboBox->setStyleSheet(
        "QComboBox {"
        "  padding: 8px;"
        "  font-size: 13px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(down_arrow.png);"
        "}"
    );
    mainLayout->addWidget(m_terrainComboBox);

    // 按钮区域
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #f5f5f5;"
        "  color: #333;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  padding: 8px 16px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e0e0e0;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_confirmButton = new QPushButton("确定", this);
    m_confirmButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px 16px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b7dda;"
        "}"
    );
    connect(m_confirmButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);

    mainLayout->addLayout(buttonLayout);
}

RegionFeatureDialog::TerrainType RegionFeatureDialog::getSelectedTerrain() const
{
    return static_cast<TerrainType>(m_terrainComboBox->currentIndex());
}

QString RegionFeatureDialog::getTerrainName() const
{
    return m_terrainComboBox->currentText();
}

QString RegionFeatureDialog::terrainTypeToString(TerrainType type)
{
    switch (type) {
        case Plain: return "平原";
        case Hills: return "丘陵";
        case Mountain: return "山地";
        case HighMountain: return "高山地";
        default: return "未知";
    }
}
