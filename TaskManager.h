// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Task.h"
#include "MapPainter.h"
#include <QObject>
#include <QVector>
#include <QMap>

/**
 * @brief 任务管理器 - 管理多个任务及其地图标记
 */
class TaskManager : public QObject {
    Q_OBJECT

public:
    explicit TaskManager(MapPainter *painter, QObject *parent = nullptr);

    // 任务管理
    Task* createTask(int id, const QString &name, const QString &description);
    Task* getTask(int taskId);
    const QVector<Task*>& getAllTasks() const { return m_tasks; }
    void removeTask(int taskId);

    // 当前任务
    void setCurrentTask(int taskId);
    Task* currentTask() const { return m_currentTask; }
    int currentTaskId() const { return m_currentTask ? m_currentTask->id() : -1; }

    // 元素操作（添加到当前任务）
    QMapLibre::AnnotationID addLoiterPoint(double lat, double lon);
    QMapLibre::AnnotationID addNoFlyZone(double lat, double lon, double radius);
    QMapLibre::AnnotationID addUAV(double lat, double lon, const QString &color);
    QMapLibre::AnnotationID addPolygon(const QMapLibre::Coordinates &coordinates);

    // 可见性控制
    void setTaskVisible(int taskId, bool visible);
    void showAllTasks();
    void hideAllTasks();

    // 元素查找（仅在可见任务中）
    const ElementInfo* findVisibleElementNear(const QMapLibre::Coordinate &clickCoord, double threshold = 100.0) const;

    // 清除当前任务的所有元素
    void clearCurrentTask();

signals:
    void taskCreated(int taskId);
    void taskRemoved(int taskId);
    void taskVisibilityChanged(int taskId, bool visible);
    void currentTaskChanged(int taskId);

private:
    void showTaskElements(Task *task);
    void hideTaskElements(Task *task);

private:
    MapPainter *m_painter;
    QVector<Task*> m_tasks;
    Task *m_currentTask;
};

#endif // TASKMANAGER_H
