// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASKLEFTCONTROLWIDGET_H
#define TASKLEFTCONTROLWIDGET_H

#include "TaskManager.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QFrame>
#include <QMap>
#include <QMouseEvent>
#include <QEnterEvent>

/**
 * @brief 任务列表项 Widget
 */
class TaskItemWidget : public QFrame {
    Q_OBJECT

public:
    explicit TaskItemWidget(Task *task, QWidget *parent = nullptr);

    int taskId() const { return m_task->id(); }
    bool isTaskVisible() const;

signals:
    void visibilityToggled(int taskId, bool visible);
    void selected(int taskId);
    void deleteRequested(int taskId);
    void editRequested(int taskId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    Task *m_task;
    QCheckBox *m_visibilityCheckBox;
    QLabel *m_descriptionLabel;
    QPushButton *m_deleteButton;
};

/**
 * @brief 任务左侧控制 Widget - 显示所有任务并提供控制
 * 支持自动收缩/展开的侧边栏模式
 */
class TaskLeftControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaskLeftControlWidget(TaskManager *taskManager, QWidget *parent = nullptr);

    void setCollapsible(bool collapsible);
    bool isCollapsed() const { return m_collapsed; }

public slots:
    void refreshTaskList();
    void onTaskCreated(int taskId);
    void onTaskRemoved(int taskId);
    void onCurrentTaskChanged(int taskId);
    void expand();
    void collapse();

signals:
    void createTaskRequested(const QString &description);
    void expandStateChanged(bool expanded);

private slots:
    void onTaskVisibilityToggled(int taskId, bool visible);
    void onTaskSelected(int taskId);
    void onTaskDeleteRequested(int taskId);
    void onTaskEditRequested(int taskId);
    void onExportTasks();
    void onImportTasks();
    void onRegionButtonClicked();    // 【任务区域】按钮点击
    void onActionButtonClicked();    // 【行动方案】按钮点击
    void onRegionListChanged();      // 区域列表变化（创建/删除）

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void setupUI();
    void addTaskItem(Task *task);
    void removeTaskItem(int taskId);
    void highlightCurrentTask(int taskId);
    void refreshRegionList();  // 刷新区域列表
    double calculateTaskRegionArea(const QMapLibre::Coordinates &coords);  // 计算任务区域面积（km²）

private:
    TaskManager *m_taskManager;
    QVBoxLayout *m_taskListLayout;
    QPushButton *m_exportButton;
    QPushButton *m_importButton;
    QPushButton *m_regionButton;    // 【任务区域】按钮
    QPushButton *m_taskPlanButton;  // 【任务方案】按钮
    QPushButton *m_actionButton;    // 【行动方案】按钮
    QPushButton *m_closeButton;     // 关闭按钮（在展开内容上）
    QWidget *m_collapsedBar;        // 左侧常驻小列
    QWidget *m_mainContent;         // 展开状态的主内容（任务列表）
    QWidget *m_regionListWidget;    // 区域列表窗口
    QVBoxLayout *m_regionContentLayout;  // 区域列表内容布局
    QMap<int, TaskItemWidget*> m_taskWidgets;
    int m_currentTaskId;

    bool m_collapsible = false;
    bool m_collapsed = false;
    int m_expandedWidth = 350;
    int m_collapsedWidth = 40;    // 常驻小列宽度
};

#endif // TASKLEFTCONTROLWIDGET_H
