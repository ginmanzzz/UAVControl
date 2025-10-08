// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskManager.h"
#include <QDebug>
#include <QtMath>
#include <cmath>

TaskManager::TaskManager(MapPainter *painter, QObject *parent)
    : QObject(parent)
    , m_painter(painter)
    , m_currentTask(nullptr)
{
}

Task* TaskManager::createTask(int id, const QString &name, const QString &description)
{
    // 检查ID是否已存在
    if (getTask(id) != nullptr) {
        qWarning() << QString("任务ID %1 已存在").arg(id);
        return nullptr;
    }

    Task *task = new Task(id, name, description);
    m_tasks.append(task);

    qDebug() << QString("创建任务 #%1: %2 (描述: %3)").arg(task->id()).arg(name).arg(description);
    emit taskCreated(task->id());

    return task;
}

Task* TaskManager::getTask(int taskId)
{
    for (Task *task : m_tasks) {
        if (task->id() == taskId) {
            return task;
        }
    }
    return nullptr;
}

void TaskManager::removeTask(int taskId)
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i]->id() == taskId) {
            Task *task = m_tasks[i];

            // 如果是当前任务，需要发射信号通知UI
            bool wasCurrentTask = (m_currentTask == task);
            if (wasCurrentTask) {
                m_currentTask = nullptr;
            }

            // 移除任务的所有地图元素
            hideTaskElements(task);
            for (const MapElement &element : task->elements()) {
                m_painter->removeAnnotation(element.annotationId);
            }

            // 删除任务
            m_tasks.removeAt(i);
            delete task;

            qDebug() << QString("删除任务 #%1").arg(taskId);
            emit taskRemoved(taskId);

            // 如果删除的是当前任务，发射信号通知当前任务已变为无
            if (wasCurrentTask) {
                qDebug() << "当前任务已被删除，发射 currentTaskChanged(-1) 信号";
                emit currentTaskChanged(-1);
            }

            return;
        }
    }
}

void TaskManager::setCurrentTask(int taskId)
{
    Task *task = getTask(taskId);
    if (task) {
        m_currentTask = task;
        qDebug() << QString("切换当前任务: #%1 - %2").arg(taskId).arg(task->description());
        emit currentTaskChanged(taskId);
    } else {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
    }
}

QMapLibre::AnnotationID TaskManager::addLoiterPoint(double lat, double lon)
{
    if (!m_currentTask) {
        qWarning() << "没有选择当前任务";
        return 0;
    }

    QMapLibre::AnnotationID id = m_painter->drawLoiterPoint(lat, lon);

    MapElement element;
    element.type = MapElement::LoiterPoint;
    element.annotationId = id;
    element.coordinate = QMapLibre::Coordinate(lat, lon);

    m_currentTask->addElement(element);
    qDebug() << QString("添加盘旋点到任务 #%1, ID: %2").arg(m_currentTask->id()).arg(id);

    return id;
}

QMapLibre::AnnotationID TaskManager::addNoFlyZone(double lat, double lon, double radius)
{
    if (!m_currentTask) {
        qWarning() << "没有选择当前任务";
        return 0;
    }

    QMapLibre::AnnotationID id = m_painter->drawNoFlyZone(lat, lon, radius);

    MapElement element;
    element.type = MapElement::NoFlyZone;
    element.annotationId = id;
    element.coordinate = QMapLibre::Coordinate(lat, lon);
    element.radius = radius;

    m_currentTask->addElement(element);
    qDebug() << QString("添加禁飞区到任务 #%1, ID: %2").arg(m_currentTask->id()).arg(id);

    return id;
}

QMapLibre::AnnotationID TaskManager::addUAV(double lat, double lon, const QString &color)
{
    if (!m_currentTask) {
        qWarning() << "没有选择当前任务";
        return 0;
    }

    QMapLibre::AnnotationID id = m_painter->drawUAV(lat, lon, color);

    MapElement element;
    element.type = MapElement::UAV;
    element.annotationId = id;
    element.coordinate = QMapLibre::Coordinate(lat, lon);
    element.color = color;

    m_currentTask->addElement(element);
    qDebug() << QString("添加无人机到任务 #%1, 颜色: %2, ID: %3")
                .arg(m_currentTask->id()).arg(color).arg(id);

    return id;
}

QMapLibre::AnnotationID TaskManager::addPolygon(const QMapLibre::Coordinates &coordinates)
{
    if (!m_currentTask) {
        qWarning() << "没有选择当前任务";
        return 0;
    }

    QMapLibre::AnnotationID id = m_painter->drawPolygonArea(coordinates);

    MapElement element;
    element.type = MapElement::Polygon;
    element.annotationId = id;
    element.vertices = coordinates;

    m_currentTask->addElement(element);
    qDebug() << QString("添加多边形到任务 #%1, ID: %2").arg(m_currentTask->id()).arg(id);

    return id;
}

void TaskManager::setTaskVisible(int taskId, bool visible)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return;
    }

    if (task->isVisible() == visible) {
        return;  // 状态未变化
    }

    task->setVisible(visible);

    if (visible) {
        showTaskElements(task);
    } else {
        hideTaskElements(task);
    }

    qDebug() << QString("任务 #%1 可见性: %2").arg(taskId).arg(visible ? "显示" : "隐藏");
    emit taskVisibilityChanged(taskId, visible);
}

void TaskManager::showAllTasks()
{
    for (Task *task : m_tasks) {
        setTaskVisible(task->id(), true);
    }
}

void TaskManager::hideAllTasks()
{
    for (Task *task : m_tasks) {
        setTaskVisible(task->id(), false);
    }
}

void TaskManager::showTaskElements(Task *task)
{
    // 重新绘制所有元素
    for (MapElement &element : const_cast<QVector<MapElement>&>(task->elements())) {
        QMapLibre::AnnotationID newId = 0;

        switch (element.type) {
        case MapElement::LoiterPoint:
            newId = m_painter->drawLoiterPoint(element.coordinate.first, element.coordinate.second);
            break;
        case MapElement::NoFlyZone:
            newId = m_painter->drawNoFlyZone(element.coordinate.first, element.coordinate.second, element.radius);
            break;
        case MapElement::UAV:
            newId = m_painter->drawUAV(element.coordinate.first, element.coordinate.second, element.color);
            break;
        case MapElement::Polygon:
            newId = m_painter->drawPolygonArea(element.vertices);
            break;
        }

        element.annotationId = newId;
    }
}

void TaskManager::hideTaskElements(Task *task)
{
    // 移除所有标注
    for (const MapElement &element : task->elements()) {
        m_painter->removeAnnotation(element.annotationId);
    }
}

const ElementInfo* TaskManager::findVisibleElementNear(const QMapLibre::Coordinate &clickCoord, double threshold) const
{
    // 首先通过 MapPainter 找到最近的元素
    const ElementInfo* nearestElement = m_painter->findElementNear(clickCoord, threshold);

    if (!nearestElement) {
        return nullptr;
    }

    // 检查该元素是否属于可见的任务，并填充element指针
    // 我们需要通过annotationId来判断元素属于哪个任务
    for (Task *task : m_tasks) {
        if (!task->isVisible()) {
            continue;  // 跳过不可见的任务
        }

        // 检查该任务是否包含这个元素
        for (MapElement &element : task->elements()) {
            // 通过annotationId匹配（更可靠）
            if (element.annotationId == nearestElement->annotationId) {
                // 创建一个增强的ElementInfo副本，包含任务信息
                static ElementInfo enhancedInfo;
                enhancedInfo = *nearestElement;
                enhancedInfo.element = &element;
                enhancedInfo.taskId = task->id();
                enhancedInfo.taskName = task->name();
                return &enhancedInfo;
            }
        }
    }

    // 元素不属于任何可见任务
    return nullptr;
}

void TaskManager::clearCurrentTask()
{
    if (!m_currentTask) {
        qWarning() << "没有选择当前任务";
        return;
    }

    qDebug() << QString("清除任务 #%1 的所有元素").arg(m_currentTask->id());

    // 移除所有地图标注
    for (const MapElement &element : m_currentTask->elements()) {
        m_painter->removeAnnotation(element.annotationId);
    }

    // 清空任务的元素列表
    m_currentTask->clearElements();

    qDebug() << QString("任务 #%1 的所有元素已清除").arg(m_currentTask->id());
}

// 计算两点之间的距离（米）
static double calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    const double EARTH_RADIUS = 6378137.0; // 地球半径（米）

    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;

    double a = qSin(dLat / 2) * qSin(dLat / 2) +
               qCos(lat1 * M_PI / 180.0) * qCos(lat2 * M_PI / 180.0) *
               qSin(dLon / 2) * qSin(dLon / 2);

    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    return EARTH_RADIUS * c;
}

bool TaskManager::isInCurrentTaskNoFlyZone(const QMapLibre::Coordinate &coord) const
{
    if (!m_currentTask) {
        return false;
    }

    // 只检查当前任务的禁飞区
    for (const MapElement &element : m_currentTask->elements()) {
        if (element.type == MapElement::NoFlyZone) {
            // 计算点到圆心的距离
            double distance = calculateDistance(coord.first, coord.second,
                                                element.coordinate.first, element.coordinate.second);

            // 如果距离小于半径，说明在禁飞区内
            if (distance <= element.radius) {
                return true;
            }
        }
    }

    return false;
}

QVector<const MapElement*> TaskManager::checkNoFlyZoneConflictWithUAVs(double centerLat, double centerLon, double radius) const
{
    QVector<const MapElement*> conflictUAVs;

    if (!m_currentTask) {
        return conflictUAVs;
    }

    // 检查当前任务的所有无人机
    for (const MapElement &element : m_currentTask->elements()) {
        if (element.type == MapElement::UAV) {
            // 计算无人机到禁飞区中心的距离
            double distance = calculateDistance(element.coordinate.first, element.coordinate.second,
                                                centerLat, centerLon);

            // 如果距离小于半径，说明无人机在禁飞区内
            if (distance <= radius) {
                conflictUAVs.append(&element);
            }
        }
    }

    return conflictUAVs;
}
