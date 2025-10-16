// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Task.h"
#include "map_region/MapRegionTypes.h"
#include "map_region/Region.h"
#include "map_region/RegionManager.h"
#include "map_region/MapPainter.h"
#include <QObject>
#include <QVector>
#include <QMap>

/**
 * @brief 任务管理器 - 管理任务和任务-区域关联
 *
 * 新架构：TaskManager 集成 RegionManager，负责：
 * - 管理任务的生命周期
 * - 管理任务与区域的关联关系
 * - 控制任务的可见性（间接控制关联区域的可见性）
 */
class TaskManager : public QObject {
    Q_OBJECT

public:
    explicit TaskManager(RegionManager *regionMgr, QObject *parent = nullptr);
    ~TaskManager();

    // ==================== 任务管理 ====================

    /**
     * @brief 创建任务（自动生成ID）
     * @param name 任务名称
     * @param description 任务描述
     * @return 创建的任务指针
     */
    Task* createTask(const QString &name, const QString &description);

    /**
     * @brief 创建任务（指定ID）- 用于导入
     */
    Task* createTask(int id, const QString &name, const QString &description);

    /**
     * @brief 获取任务
     */
    Task* getTask(int taskId);

    /**
     * @brief 获取所有任务
     */
    const QVector<Task*>& getAllTasks() const { return m_tasks; }

    /**
     * @brief 删除任务（不删除关联的区域）
     */
    void removeTask(int taskId);

    // ==================== 当前任务 ====================

    /**
     * @brief 设置当前任务
     */
    void setCurrentTask(int taskId);

    /**
     * @brief 获取当前任务
     */
    Task* currentTask() const { return m_currentTask; }

    /**
     * @brief 获取当前任务ID
     */
    int currentTaskId() const { return m_currentTask ? m_currentTask->id() : -1; }

    // ==================== 任务-区域关联 ====================

    /**
     * @brief 添加区域到任务
     * @param taskId 任务ID
     * @param regionId 区域ID
     */
    void addRegionToTask(int taskId, int regionId);

    /**
     * @brief 从任务移除区域
     * @param taskId 任务ID
     * @param regionId 区域ID
     */
    void removeRegionFromTask(int taskId, int regionId);

    /**
     * @brief 获取任务的所有区域
     * @param taskId 任务ID
     * @return 区域指针列表
     */
    QVector<Region*> getTaskRegions(int taskId);

    /**
     * @brief 清除任务的所有区域关联
     * @param taskId 任务ID
     */
    void clearTaskRegions(int taskId);

    // ==================== 可见性控制 ====================

    /**
     * @brief 设置任务可见性（显示/隐藏关联的区域）
     * @param taskId 任务ID
     * @param visible 是否可见
     */
    void setTaskVisible(int taskId, bool visible);

    /**
     * @brief 显示所有任务
     */
    void showAllTasks();

    /**
     * @brief 隐藏所有任务
     */
    void hideAllTasks();

    // ==================== 查找（兼容旧版）====================

    /**
     * @brief 在可见任务的区域中查找附近的区域
     * @param clickCoord 点击坐标
     * @param threshold 距离阈值（米）
     * @return 区域指针，不存在返回 nullptr
     */
    Region* findVisibleRegionNear(const QMapLibre::Coordinate &clickCoord, double threshold = 100.0);

    /**
     * @brief 检查坐标是否在任何禁飞区内（全局检查）
     */
    bool isInAnyNoFlyZone(const QMapLibre::Coordinate &coord) const;

    /**
     * @brief 检查禁飞区是否与任何无人机冲突（全局检查）
     */
    QVector<Region*> checkNoFlyZoneConflictWithUAVs(double centerLat, double centerLon, double radius) const;

    /**
     * @brief 获取区域管理器
     */
    RegionManager* regionManager() const { return m_regionMgr; }

    /**
     * @brief 获取区域的引用计数（被多少个任务引用）
     */
    int getRegionReferenceCount(int regionId) const;

    /**
     * @brief 获取引用某个区域的所有任务
     */
    QVector<Task*> getTasksReferencingRegion(int regionId) const;

    // ==================== 旧版接口（保留兼容）====================
    // TODO: 后续迁移完成后删除

    QMapLibre::AnnotationID addLoiterPoint(double lat, double lon);
    QMapLibre::AnnotationID addNoFlyZone(double lat, double lon, double radius);
    QMapLibre::AnnotationID addUAV(double lat, double lon, const QString &color);
    QMapLibre::AnnotationID addTaskRegion(const QMapLibre::Coordinates &coordinates);
    QMapLibre::AnnotationID addCircularTaskRegion(const QMapLibre::Coordinate &center, double radius,
                                                   const QMapLibre::Coordinates &vertices);
    QMapLibre::AnnotationID addRectangularTaskRegion(const QMapLibre::Coordinates &vertices);

    QMapLibre::AnnotationID addLoiterPointToTask(int taskId, double lat, double lon);
    QMapLibre::AnnotationID addNoFlyZoneToTask(int taskId, double lat, double lon, double radius);
    QMapLibre::AnnotationID addUAVToTask(int taskId, double lat, double lon, const QString &color);
    QMapLibre::AnnotationID addTaskRegionToTask(int taskId, const QMapLibre::Coordinates &coordinates);

    void clearCurrentTask();
    const RegionInfo* findVisibleElementNear(const QMapLibre::Coordinate &clickCoord, double threshold = 100.0) const;

    /**
     * @brief 生成下一个任务ID
     */
    int generateNextTaskId();

signals:
    void taskCreated(int taskId);
    void taskRemoved(int taskId);
    void taskVisibilityChanged(int taskId, bool visible);
    void currentTaskChanged(int taskId);
    void taskRegionsChanged(int taskId);  // 任务的区域列表变化

private slots:
    /**
     * @brief 监听区域删除事件，清理任务中的引用
     */
    void onRegionRemoved(int regionId);

private:
    /**
     * @brief 更新任务的可见性
     */
    void updateTaskVisibility(Task *task);

    // 旧版兼容方法
    void showTaskElements(Task *task);
    void hideTaskElements(Task *task);

private:
    RegionManager *m_regionMgr;       // 区域管理器（不拥有所有权）
    MapPainter *m_painter;            // 地图画家（旧版兼容，不拥有所有权）
    QVector<Task*> m_tasks;           // 任务列表（拥有所有权）
    Task *m_currentTask;              // 当前任务
    int m_nextTaskId;                 // 下一个任务ID
};

#endif // TASKMANAGER_H
