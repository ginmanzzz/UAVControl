// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef REGIONPROPERTYDIALOG_H
#define REGIONPROPERTYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

/**
 * @brief 区域属性对话框
 * 用于输入区域名称和选择地形特征
 */
class RegionPropertyDialog : public QDialog {
    Q_OBJECT

public:
    enum TerrainType {
        Plain = 0,      // 平原
        Hills = 1,      // 丘陵
        Mountain = 2,   // 山地
        HighMountain = 3 // 高山地
    };

    explicit RegionPropertyDialog(const QString &defaultName, QWidget *parent = nullptr);
    explicit RegionPropertyDialog(const QString &defaultName, TerrainType currentTerrain, QWidget *parent = nullptr);

    QString getRegionName() const;
    TerrainType getSelectedTerrain() const;
    QString getTerrainName() const;

    static QString terrainTypeToString(TerrainType type);

private:
    void setupUI(const QString &defaultName, bool showTerrain);
    void centerOnScreen();

private:
    QLineEdit *m_nameEdit;
    QComboBox *m_terrainComboBox;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;
    bool m_showTerrain;
};

#endif // REGIONPROPERTYDIALOG_H
