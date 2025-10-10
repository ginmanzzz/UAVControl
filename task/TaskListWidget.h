// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASKLISTWIDGET_H
#define TASKLISTWIDGET_H

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
#include <QPropertyAnimation>
#include <QEnterEvent>

class QGraphicsOpacityEffect;

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
 * @brief 任务列表 Widget - 显示所有任务并提供控制
 * 支持自动收缩/展开的侧边栏模式
 */
class TaskListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaskListWidget(TaskManager *taskManager, QWidget *parent = nullptr);

    void setCollapsible(bool collapsible);
    bool isCollapsed() const { return m_collapsed; }
    bool isPinned() const { return m_pinned; }

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
    void onCreateTask();
    void onTaskVisibilityToggled(int taskId, bool visible);
    void onTaskSelected(int taskId);
    void onTaskDeleteRequested(int taskId);
    void onTaskEditRequested(int taskId);
    void onPinToggled();
    void onExportTasks();
    void onImportTasks();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void setupUI();
    void addTaskItem(Task *task);
    void removeTaskItem(int taskId);
    void highlightCurrentTask(int taskId);
    void updatePinButtonIcon();
    void updateStyleForCollapsedState();

private:
    TaskManager *m_taskManager;
    QVBoxLayout *m_taskListLayout;
    QPushButton *m_createButton;
    QPushButton *m_pinButton;
    QPushButton *m_exportButton;
    QPushButton *m_importButton;
    QMap<int, TaskItemWidget*> m_taskWidgets;
    int m_currentTaskId;

    bool m_collapsible = false;
    bool m_collapsed = false;
    bool m_pinned = false;
    int m_expandedWidth = 350;
    int m_collapsedWidth = 50;  // 增加到50px，更容易触发

    QGraphicsOpacityEffect *m_opacityEffect = nullptr;  // 透明度效果
};

#endif // TASKLISTWIDGET_H
