// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef REGIONFEATUREDIALOG_H
#define REGIONFEATUREDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

/**
 * @brief 区域特征选择对话框
 * 用于选择多边形区域和禁飞区域的地形特征
 */
class RegionFeatureDialog : public QDialog {
    Q_OBJECT

public:
    enum TerrainType {
        Plain = 0,      // 平原
        Hills = 1,      // 丘陵
        Mountain = 2,   // 山地
        HighMountain = 3 // 高山地
    };

    explicit RegionFeatureDialog(QWidget *parent = nullptr);
    explicit RegionFeatureDialog(TerrainType currentTerrain, QWidget *parent = nullptr);

    TerrainType getSelectedTerrain() const;
    QString getTerrainName() const;

    static QString terrainTypeToString(TerrainType type);

private:
    void setupUI();

private:
    QComboBox *m_terrainComboBox;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;
};

#endif // REGIONFEATUREDIALOG_H
