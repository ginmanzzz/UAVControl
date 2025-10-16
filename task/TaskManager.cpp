// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskManager.h"
#include <QDebug>
#include <QtMath>
#include <cmath>

// 计算两点之间的距离（米）- 辅助函数
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

TaskManager::TaskManager(RegionManager *regionMgr, QObject *parent)
    : QObject(parent)
    , m_regionMgr(regionMgr)
    , m_painter(nullptr)  // 旧版兼容，保留但不使用
    , m_currentTask(nullptr)
    , m_nextTaskId(1)
{
    // 连接 RegionManager 的信号，监听区域删除事件
    if (m_regionMgr) {
        connect(m_regionMgr, &RegionManager::regionRemoved,
                this, &TaskManager::onRegionRemoved);
    }
}

TaskManager::~TaskManager()
{
    // 删除所有任务（Task 析构函数会清理 m_regionIds）
    qDeleteAll(m_tasks);
    m_tasks.clear();
}

// ==================== 任务管理 ====================

Task* TaskManager::createTask(const QString &name, const QString &description)
{
    int id = generateNextTaskId();
    return createTask(id, name, description);
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

    // 更新 nextTaskId（确保导入时不会冲突）
    if (id >= m_nextTaskId) {
        m_nextTaskId = id + 1;
    }

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

            // 删除任务（不删除关联的区域，只清除引用）
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

// ==================== 当前任务 ====================

void TaskManager::setCurrentTask(int taskId)
{
    Task *task = getTask(taskId);
    if (task) {
        m_currentTask = task;
        qDebug() << QString("切换当前任务: #%1 - %2").arg(taskId).arg(task->name());
        emit currentTaskChanged(taskId);
    } else {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
    }
}

// ==================== 任务-区域关联 ====================

void TaskManager::addRegionToTask(int taskId, int regionId)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return;
    }

    Region *region = m_regionMgr->getRegion(regionId);
    if (!region) {
        qWarning() << QString("区域 #%1 不存在").arg(regionId);
        return;
    }

    task->addRegion(regionId);
    qDebug() << QString("添加区域 #%1 (%2) 到任务 #%3 (%4)")
                .arg(regionId).arg(region->name())
                .arg(taskId).arg(task->name());

    emit taskRegionsChanged(taskId);
}

void TaskManager::removeRegionFromTask(int taskId, int regionId)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return;
    }

    if (task->removeRegion(regionId)) {
        qDebug() << QString("从任务 #%1 移除区域 #%2").arg(taskId).arg(regionId);
        emit taskRegionsChanged(taskId);
    } else {
        qWarning() << QString("任务 #%1 不包含区域 #%2").arg(taskId).arg(regionId);
    }
}

QVector<Region*> TaskManager::getTaskRegions(int taskId)
{
    QVector<Region*> regions;

    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return regions;
    }

    // 遍历任务的区域ID集合，获取区域指针
    for (int regionId : task->regionIds()) {
        Region *region = m_regionMgr->getRegion(regionId);
        if (region) {
            regions.append(region);
        } else {
            qWarning() << QString("任务 #%1 引用的区域 #%2 不存在").arg(taskId).arg(regionId);
        }
    }

    return regions;
}

void TaskManager::clearTaskRegions(int taskId)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return;
    }

    task->clearRegions();
    qDebug() << QString("清除任务 #%1 的所有区域关联").arg(taskId);
    emit taskRegionsChanged(taskId);
}

// ==================== 可见性控制 ====================

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
    updateTaskVisibility(task);

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

void TaskManager::updateTaskVisibility(Task *task)
{
    if (!task) {
        return;
    }

    // 遍历任务的所有区域，设置可见性
    for (int regionId : task->regionIds()) {
        if (task->isVisible()) {
            m_regionMgr->showRegion(regionId);
        } else {
            m_regionMgr->hideRegion(regionId);
        }
    }
}

// ==================== 查找（兼容旧版）====================

Region* TaskManager::findVisibleRegionNear(const QMapLibre::Coordinate &clickCoord, double threshold)
{
    double minDistance = threshold;
    Region *nearestRegion = nullptr;

    // 遍历所有可见任务的区域
    for (Task *task : m_tasks) {
        if (!task->isVisible()) {
            continue;  // 跳过不可见的任务
        }

        // 遍历任务的所有区域
        for (int regionId : task->regionIds()) {
            Region *region = m_regionMgr->getRegion(regionId);
            if (!region) {
                continue;
            }

            // 计算距离
            double distance = calculateDistance(
                clickCoord.first, clickCoord.second,
                region->coordinate().first, region->coordinate().second
            );

            // 找到最近的区域
            if (distance < minDistance) {
                minDistance = distance;
                nearestRegion = region;
            }
        }
    }

    return nearestRegion;
}

bool TaskManager::isInAnyNoFlyZone(const QMapLibre::Coordinate &coord) const
{
    // 全局检查所有禁飞区（不限于当前任务）
    const QMap<int, Region*>& allRegions = m_regionMgr->getAllRegions();

    for (Region *region : allRegions) {
        if (!region || region->type() != RegionType::NoFlyZone) {
            continue;
        }

        // 计算点到圆心的距离
        double distance = calculateDistance(
            coord.first, coord.second,
            region->coordinate().first, region->coordinate().second
        );

        // 如果距离小于半径，说明在禁飞区内
        if (distance <= region->radius()) {
            return true;
        }
    }

    return false;
}

QVector<Region*> TaskManager::checkNoFlyZoneConflictWithUAVs(double centerLat, double centerLon, double radius) const
{
    QVector<Region*> conflictUAVs;

    // 全局检查所有无人机（不限于当前任务）
    const QMap<int, Region*>& allRegions = m_regionMgr->getAllRegions();

    for (Region *region : allRegions) {
        if (!region || region->type() != RegionType::UAV) {
            continue;
        }

        // 计算无人机到禁飞区中心的距离
        double distance = calculateDistance(
            region->coordinate().first, region->coordinate().second,
            centerLat, centerLon
        );

        // 如果距离小于半径，说明无人机在禁飞区内
        if (distance <= radius) {
            conflictUAVs.append(region);
        }
    }

    return conflictUAVs;
}

// ==================== 旧版接口（保留兼容）====================

QMapLibre::AnnotationID TaskManager::addLoiterPoint(double lat, double lon)
{
    // 直接创建区域（不需要当前任务）
    Region *region = m_regionMgr->createLoiterPoint(lat, lon);
    if (!region) {
        return 0;
    }

    // 如果有当前任务，自动关联
    if (m_currentTask) {
        addRegionToTask(m_currentTask->id(), region->id());
        qDebug() << QString("添加盘旋点区域 #%1 到任务 #%2")
                    .arg(region->id()).arg(m_currentTask->id());
    } else {
        qDebug() << QString("添加独立盘旋点区域 #%1（未关联任务）").arg(region->id());
    }

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addNoFlyZone(double lat, double lon, double radius)
{
    // 直接创建区域（不需要当前任务）
    Region *region = m_regionMgr->createNoFlyZone(lat, lon, radius);
    if (!region) {
        return 0;
    }

    // 如果有当前任务，自动关联
    if (m_currentTask) {
        addRegionToTask(m_currentTask->id(), region->id());
        qDebug() << QString("添加禁飞区区域 #%1 到任务 #%2")
                    .arg(region->id()).arg(m_currentTask->id());
    } else {
        qDebug() << QString("添加独立禁飞区区域 #%1（未关联任务）").arg(region->id());
    }

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addUAV(double lat, double lon, const QString &color)
{
    // 直接创建区域（不需要当前任务）
    Region *region = m_regionMgr->createUAV(lat, lon, color);
    if (!region) {
        return 0;
    }

    // 如果有当前任务，自动关联
    if (m_currentTask) {
        addRegionToTask(m_currentTask->id(), region->id());
        qDebug() << QString("添加无人机区域 #%1 (颜色: %2) 到任务 #%3")
                    .arg(region->id()).arg(color).arg(m_currentTask->id());
    } else {
        qDebug() << QString("添加独立无人机区域 #%1 (颜色: %2, 未关联任务)")
                    .arg(region->id()).arg(color);
    }

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addTaskRegion(const QMapLibre::Coordinates &coordinates)
{
    // 直接创建区域（不需要当前任务）
    Region *region = m_regionMgr->createTaskRegion(coordinates);
    if (!region) {
        return 0;
    }

    // 如果有当前任务，自动关联
    if (m_currentTask) {
        addRegionToTask(m_currentTask->id(), region->id());
        qDebug() << QString("添加任务区域 #%1 到任务 #%2")
                    .arg(region->id()).arg(m_currentTask->id());
    } else {
        qDebug() << QString("添加独立任务区域 #%1（未关联任务）").arg(region->id());
    }

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addCircularTaskRegion(const QMapLibre::Coordinate &center, double radius,
                                                           const QMapLibre::Coordinates &vertices)
{
    // 创建圆形任务区域
    Region *region = m_regionMgr->createCircularTaskRegion(center, radius, vertices);
    if (!region) {
        return 0;
    }

    // 如果有当前任务，自动关联
    if (m_currentTask) {
        addRegionToTask(m_currentTask->id(), region->id());
        qDebug() << QString("添加圆形任务区域 #%1 (半径 %2m) 到任务 #%3")
                    .arg(region->id()).arg(radius).arg(m_currentTask->id());
    } else {
        qDebug() << QString("添加独立圆形任务区域 #%1（未关联任务）").arg(region->id());
    }

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addRectangularTaskRegion(const QMapLibre::Coordinates &vertices)
{
    // 创建矩形任务区域
    Region *region = m_regionMgr->createRectangularTaskRegion(vertices);
    if (!region) {
        return 0;
    }

    // 如果有当前任务，自动关联
    if (m_currentTask) {
        addRegionToTask(m_currentTask->id(), region->id());
        qDebug() << QString("添加矩形任务区域 #%1 到任务 #%2")
                    .arg(region->id()).arg(m_currentTask->id());
    } else {
        qDebug() << QString("添加独立矩形任务区域 #%1（未关联任务）").arg(region->id());
    }

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addLoiterPointToTask(int taskId, double lat, double lon)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return 0;
    }

    // 创建区域
    Region *region = m_regionMgr->createLoiterPoint(lat, lon);
    if (!region) {
        return 0;
    }

    // 关联到指定任务
    addRegionToTask(taskId, region->id());

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addNoFlyZoneToTask(int taskId, double lat, double lon, double radius)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return 0;
    }

    // 创建区域
    Region *region = m_regionMgr->createNoFlyZone(lat, lon, radius);
    if (!region) {
        return 0;
    }

    // 关联到指定任务
    addRegionToTask(taskId, region->id());

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addUAVToTask(int taskId, double lat, double lon, const QString &color)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return 0;
    }

    // 创建区域
    Region *region = m_regionMgr->createUAV(lat, lon, color);
    if (!region) {
        return 0;
    }

    // 关联到指定任务
    addRegionToTask(taskId, region->id());

    return region->annotationId();
}

QMapLibre::AnnotationID TaskManager::addTaskRegionToTask(int taskId, const QMapLibre::Coordinates &coordinates)
{
    Task *task = getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return 0;
    }

    if (coordinates.size() < 3) {
        qWarning() << "多边形至少需要3个顶点";
        return 0;
    }

    // 创建区域
    Region *region = m_regionMgr->createTaskRegion(coordinates);
    if (!region) {
        return 0;
    }

    // 关联到指定任务
    addRegionToTask(taskId, region->id());

    return region->annotationId();
}

void TaskManager::clearCurrentTask()
{
    if (!m_currentTask) {
        qWarning() << "没有选择当前任务";
        return;
    }

    qDebug() << QString("清除任务 #%1 的所有区域关联").arg(m_currentTask->id());

    // 清除所有区域关联（不删除区域本身）
    clearTaskRegions(m_currentTask->id());

    qDebug() << QString("任务 #%1 的所有区域关联已清除").arg(m_currentTask->id());
}

const RegionInfo* TaskManager::findVisibleElementNear(const QMapLibre::Coordinate &clickCoord, double threshold) const
{
    // 通过 RegionManager 找到最近的区域
    const RegionInfo* nearestElement = m_regionMgr->findRegionInfoNear(clickCoord, threshold);

    if (!nearestElement) {
        return nullptr;
    }

    // 通过 annotationId 找到对应的 Region
    Region *region = m_regionMgr->findRegionByAnnotationId(nearestElement->annotationId);
    if (!region) {
        return nullptr;
    }

    // 创建增强的 RegionInfo 副本
    static RegionInfo enhancedInfo;
    enhancedInfo = *nearestElement;
    enhancedInfo.regionId = region->id();
    enhancedInfo.terrainType = static_cast<TerrainType>(region->terrainType());

    // 检查该区域是否属于某个可见的任务
    for (Task *task : m_tasks) {
        if (task->isVisible() && task->hasRegion(region->id())) {
            enhancedInfo.taskId = task->id();
            enhancedInfo.taskName = task->name();
            return &enhancedInfo;
        }
    }

    // 独立区域（不属于任何任务）
    enhancedInfo.taskId = -1;
    enhancedInfo.taskName = "";
    return &enhancedInfo;
}

int TaskManager::getRegionReferenceCount(int regionId) const
{
    int count = 0;
    for (Task *task : m_tasks) {
        if (task->hasRegion(regionId)) {
            count++;
        }
    }
    return count;
}

QVector<Task*> TaskManager::getTasksReferencingRegion(int regionId) const
{
    QVector<Task*> referencingTasks;
    for (Task *task : m_tasks) {
        if (task->hasRegion(regionId)) {
            referencingTasks.append(task);
        }
    }
    return referencingTasks;
}

// ==================== 私有方法 ====================

void TaskManager::onRegionRemoved(int regionId)
{
    // 当区域被删除时，清理所有任务中的引用
    for (Task *task : m_tasks) {
        if (task->removeRegion(regionId)) {
            qDebug() << QString("从任务 #%1 自动移除已删除的区域 #%2")
                        .arg(task->id()).arg(regionId);
            emit taskRegionsChanged(task->id());
        }
    }
}

int TaskManager::generateNextTaskId()
{
    return m_nextTaskId++;
}

// ==================== 旧版兼容（保留）====================

void TaskManager::showTaskElements(Task *task)
{
    // 旧版方法，新架构通过 updateTaskVisibility 实现
    updateTaskVisibility(task);
}

void TaskManager::hideTaskElements(Task *task)
{
    // 旧版方法，新架构通过 updateTaskVisibility 实现
    task->setVisible(false);
    updateTaskVisibility(task);
}
