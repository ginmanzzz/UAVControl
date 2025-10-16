// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "RegionPropertyDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

RegionPropertyDialog::RegionPropertyDialog(const QString &defaultName, QWidget *parent)
    : QDialog(parent), m_showTerrain(false)
{
    setupUI(defaultName, false);
}

RegionPropertyDialog::RegionPropertyDialog(const QString &defaultName, TerrainType currentTerrain, QWidget *parent)
    : QDialog(parent), m_showTerrain(true)
{
    setupUI(defaultName, true);
    // 设置当前地形类型
    if (m_terrainComboBox) {
        m_terrainComboBox->setCurrentIndex(static_cast<int>(currentTerrain));
    }
}

void RegionPropertyDialog::setupUI(const QString &defaultName, bool showTerrain)
{
    setWindowTitle(showTerrain ? "区域属性" : "区域名称");
    setModal(true);
    setMinimumWidth(300);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 名称输入
    auto *nameLabel = new QLabel("区域名称:", this);
    nameLabel->setStyleSheet("font-size: 13px; font-weight: bold;");
    mainLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(defaultName, this);
    m_nameEdit->selectAll();  // 自动选中所有文本
    m_nameEdit->setStyleSheet(
        "QLineEdit {"
        "  padding: 8px;"
        "  font-size: 13px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #2196F3;"
        "}"
    );
    mainLayout->addWidget(m_nameEdit);

    // 地形类型下拉框（仅在需要时显示）
    if (showTerrain) {
        auto *terrainLabel = new QLabel("地形特征:", this);
        terrainLabel->setStyleSheet("font-size: 13px; font-weight: bold;");
        mainLayout->addWidget(terrainLabel);

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
        );
        mainLayout->addWidget(m_terrainComboBox);
    } else {
        m_terrainComboBox = nullptr;
    }

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

    // 设置焦点到名称输入框
    m_nameEdit->setFocus();
}

QString RegionPropertyDialog::getRegionName() const
{
    return m_nameEdit->text().trimmed();
}

RegionPropertyDialog::TerrainType RegionPropertyDialog::getSelectedTerrain() const
{
    if (m_terrainComboBox) {
        return static_cast<TerrainType>(m_terrainComboBox->currentIndex());
    }
    return Plain;  // 默认返回平原
}

QString RegionPropertyDialog::getTerrainName() const
{
    if (m_terrainComboBox) {
        return m_terrainComboBox->currentText();
    }
    return "平原";
}

QString RegionPropertyDialog::terrainTypeToString(TerrainType type)
{
    switch (type) {
        case Plain: return "平原";
        case Hills: return "丘陵";
        case Mountain: return "山地";
        case HighMountain: return "高山地";
        default: return "未知";
    }
}
