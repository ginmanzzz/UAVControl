// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef ELEMENTDETAILWIDGET_H
#define ELEMENTDETAILWIDGET_H

#include "map/MapPainter.h"
#include "Task.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief 元素详情浮动窗口
 */
class ElementDetailWidget : public QWidget {
    Q_OBJECT

public:
    explicit ElementDetailWidget(QWidget *parent = nullptr);
    void showElement(const ElementInfo *info, const QPoint &screenPos);

signals:
    void terrainChanged(QMapLibre::AnnotationID annotationId, MapElement::TerrainType newTerrain);
    void deleteRequested(QMapLibre::AnnotationID annotationId);

private:
    void addTitle(const QString &text);
    void addInfoLine(const QString &label, const QString &value);
    void addTerrainLine(const QString &label, MapElement::TerrainType currentTerrain);
    void addDeleteButton();
    QString getColorName(const QString &color) const;
    double calculatePolygonArea(const QMapLibre::Coordinates &vertices) const;

private:
    QWidget *m_contentWidget;
    QVBoxLayout *m_contentLayout;
    const ElementInfo *m_currentElement;
    QPushButton *m_deleteButton;
};

#endif // ELEMENTDETAILWIDGET_H
