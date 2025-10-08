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
 */
class TaskListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaskListWidget(TaskManager *taskManager, QWidget *parent = nullptr);

public slots:
    void refreshTaskList();
    void onTaskCreated(int taskId);
    void onTaskRemoved(int taskId);
    void onCurrentTaskChanged(int taskId);

signals:
    void createTaskRequested(const QString &description);

private slots:
    void onCreateTask();
    void onTaskVisibilityToggled(int taskId, bool visible);
    void onTaskSelected(int taskId);
    void onTaskDeleteRequested(int taskId);
    void onTaskEditRequested(int taskId);

private:
    void setupUI();
    void addTaskItem(Task *task);
    void removeTaskItem(int taskId);
    void highlightCurrentTask(int taskId);

private:
    TaskManager *m_taskManager;
    QVBoxLayout *m_taskListLayout;
    QPushButton *m_createButton;
    QMap<int, TaskItemWidget*> m_taskWidgets;
    int m_currentTaskId;
};

#endif // TASKLISTWIDGET_H
