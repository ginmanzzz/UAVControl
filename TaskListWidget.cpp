// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskListWidget.h"
#include "TaskDialog.h"
#include <QDebug>

// ============ TaskItemWidget 实现 ============

TaskItemWidget::TaskItemWidget(Task *task, QWidget *parent)
    : QFrame(parent)
    , m_task(task)
{
    setFrameShape(QFrame::Box);
    setStyleSheet(
        "TaskItemWidget {"
        "  background-color: white;"
        "  border: 1px solid #ddd;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  margin: 2px;"
        "}"
        "TaskItemWidget:hover {"
        "  background-color: #f5f5f5;"
        "}"
    );

    // 设置鼠标光标为手型
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(8);

    // 可见性复选框（无文字标签，对齐列标题）
    auto *checkboxContainer = new QWidget(this);
    checkboxContainer->setFixedWidth(50);
    auto *checkboxLayout = new QHBoxLayout(checkboxContainer);
    checkboxLayout->setContentsMargins(0, 0, 0, 0);

    m_visibilityCheckBox = new QCheckBox(checkboxContainer);
    m_visibilityCheckBox->setChecked(task->isVisible());
    m_visibilityCheckBox->setToolTip("勾选以显示任务的地图标记");
    m_visibilityCheckBox->setStyleSheet(
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "}"
    );
    connect(m_visibilityCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit visibilityToggled(m_task->id(), checked);
    });

    checkboxLayout->addWidget(m_visibilityCheckBox);
    checkboxLayout->addStretch();

    // 任务名称
    m_descriptionLabel = new QLabel(QString("#%1: %2")
                                        .arg(task->id())
                                        .arg(task->name()), this);
    m_descriptionLabel->setStyleSheet("font-weight: normal; font-size: 12px;");
    m_descriptionLabel->setToolTip(task->description().isEmpty() ? "无详细描述" : task->description());

    // 删除按钮
    m_deleteButton = new QPushButton("删除", this);
    m_deleteButton->setFixedWidth(60);
    m_deleteButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #f44336;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #da190b;"
        "}"
    );
    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_task->id());
    });

    layout->addWidget(checkboxContainer);
    layout->addWidget(m_descriptionLabel, 1);
    layout->addWidget(m_deleteButton);
}

void TaskItemWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击在复选框或删除按钮上
        QPoint pos = event->pos();
        if (!m_visibilityCheckBox->geometry().contains(pos) &&
            !m_deleteButton->geometry().contains(pos)) {
            // 点击了任务项的其他区域，选中该任务
            emit selected(m_task->id());
        }
    }
    QFrame::mousePressEvent(event);
}

void TaskItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否双击在复选框或删除按钮上
        QPoint pos = event->pos();
        if (!m_visibilityCheckBox->geometry().contains(pos) &&
            !m_deleteButton->geometry().contains(pos)) {
            // 双击了任务项的其他区域，编辑该任务
            emit editRequested(m_task->id());
        }
    }
    QFrame::mouseDoubleClickEvent(event);
}

bool TaskItemWidget::isTaskVisible() const
{
    return m_visibilityCheckBox->isChecked();
}

// ============ TaskListWidget 实现 ============

TaskListWidget::TaskListWidget(TaskManager *taskManager, QWidget *parent)
    : QWidget(parent)
    , m_taskManager(taskManager)
    , m_currentTaskId(-1)
{
    setupUI();

    // 连接信号
    connect(m_taskManager, &TaskManager::taskCreated,
            this, &TaskListWidget::onTaskCreated);
    connect(m_taskManager, &TaskManager::taskRemoved,
            this, &TaskListWidget::onTaskRemoved);
    connect(m_taskManager, &TaskManager::currentTaskChanged,
            this, &TaskListWidget::onCurrentTaskChanged);
}

void TaskListWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 标题
    auto *titleLabel = new QLabel("任务列表", this);
    titleLabel->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #333;"
        "padding: 5px;"
    );
    mainLayout->addWidget(titleLabel);

    // 创建任务按钮
    m_createButton = new QPushButton("+ 创建新任务", this);
    m_createButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b7dda;"
        "}"
    );
    connect(m_createButton, &QPushButton::clicked, this, &TaskListWidget::onCreateTask);

    mainLayout->addWidget(m_createButton);

    // 列标题
    auto *headerFrame = new QFrame(this);
    headerFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #e0e0e0;"
        "  border-radius: 4px;"
        "  padding: 6px 8px;"
        "}"
    );
    auto *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(8, 4, 8, 4);
    headerLayout->setSpacing(8);

    auto *visibilityHeader = new QLabel("显示", headerFrame);
    visibilityHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
    visibilityHeader->setFixedWidth(50);

    auto *descriptionHeader = new QLabel("任务", headerFrame);
    descriptionHeader->setStyleSheet("font-weight: bold; font-size: 12px;");

    auto *actionHeader = new QLabel("操作", headerFrame);
    actionHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
    actionHeader->setFixedWidth(60);

    headerLayout->addWidget(visibilityHeader);
    headerLayout->addWidget(descriptionHeader, 1);
    headerLayout->addWidget(actionHeader);

    mainLayout->addWidget(headerFrame);

    // 任务列表区域（滚动）
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "  border: none;"
        "  background-color: transparent;"
        "}"
    );

    auto *scrollWidget = new QWidget();
    m_taskListLayout = new QVBoxLayout(scrollWidget);
    m_taskListLayout->setContentsMargins(0, 0, 0, 0);
    m_taskListLayout->setSpacing(5);
    m_taskListLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea, 1);

    // 设置整体样式
    setStyleSheet(
        "TaskListWidget {"
        "  background-color: #fafafa;"
        "  border-right: 2px solid #ddd;"
        "}"
    );
}

void TaskListWidget::refreshTaskList()
{
    // 清空现有列表
    for (auto *widget : m_taskWidgets.values()) {
        widget->deleteLater();
    }
    m_taskWidgets.clear();

    // 重新添加所有任务
    for (Task *task : m_taskManager->getAllTasks()) {
        addTaskItem(task);
    }
}

void TaskListWidget::onTaskCreated(int taskId)
{
    Task *task = m_taskManager->getTask(taskId);
    if (task) {
        addTaskItem(task);
    }
}

void TaskListWidget::onTaskRemoved(int taskId)
{
    removeTaskItem(taskId);
}

void TaskListWidget::onCurrentTaskChanged(int taskId)
{
    highlightCurrentTask(taskId);
}

void TaskListWidget::onCreateTask()
{
    // 打开创建任务对话框
    TaskDialog dialog(m_taskManager, this);

    if (dialog.exec() == QDialog::Accepted) {
        // 用户点击了创建按钮
        int taskId = dialog.getTaskId();
        QString taskName = dialog.getTaskName();
        QString taskDescription = dialog.getTaskDescription();

        // 创建任务
        Task *task = m_taskManager->createTask(taskId, taskName, taskDescription);

        if (task) {
            // 自动设为当前任务
            m_taskManager->setCurrentTask(task->id());
            qDebug() << QString("创建新任务: #%1 - %2").arg(task->id()).arg(taskName);
        } else {
            qWarning() << "创建任务失败";
        }
    }
}

void TaskListWidget::onTaskVisibilityToggled(int taskId, bool visible)
{
    m_taskManager->setTaskVisible(taskId, visible);
}

void TaskListWidget::onTaskSelected(int taskId)
{
    m_taskManager->setCurrentTask(taskId);
}

void TaskListWidget::onTaskDeleteRequested(int taskId)
{
    m_taskManager->removeTask(taskId);
}

void TaskListWidget::onTaskEditRequested(int taskId)
{
    Task *task = m_taskManager->getTask(taskId);
    if (!task) {
        qWarning() << QString("任务 #%1 不存在").arg(taskId);
        return;
    }

    // 打开编辑对话框
    TaskDialog dialog(m_taskManager, task, this);

    if (dialog.exec() == QDialog::Accepted) {
        // 用户点击了保存按钮，更新任务信息
        QString newName = dialog.getTaskName();
        QString newDescription = dialog.getTaskDescription();

        task->setName(newName);
        task->setDescription(newDescription);

        // 更新UI显示
        if (m_taskWidgets.contains(taskId)) {
            TaskItemWidget *widget = m_taskWidgets[taskId];
            // 刷新任务项的显示
            removeTaskItem(taskId);
            addTaskItem(task);

            // 如果是当前任务，重新高亮
            if (m_taskManager->currentTaskId() == taskId) {
                highlightCurrentTask(taskId);
            }
        }

        qDebug() << QString("任务 #%1 已更新: %2").arg(taskId).arg(newName);
    }
}

void TaskListWidget::addTaskItem(Task *task)
{
    auto *taskWidget = new TaskItemWidget(task, this);

    connect(taskWidget, &TaskItemWidget::visibilityToggled,
            this, &TaskListWidget::onTaskVisibilityToggled);
    connect(taskWidget, &TaskItemWidget::selected,
            this, &TaskListWidget::onTaskSelected);
    connect(taskWidget, &TaskItemWidget::deleteRequested,
            this, &TaskListWidget::onTaskDeleteRequested);
    connect(taskWidget, &TaskItemWidget::editRequested,
            this, &TaskListWidget::onTaskEditRequested);

    // 插入到列表末尾（stretch 之前）
    m_taskListLayout->insertWidget(m_taskListLayout->count() - 1, taskWidget);
    m_taskWidgets[task->id()] = taskWidget;
}

void TaskListWidget::removeTaskItem(int taskId)
{
    if (m_taskWidgets.contains(taskId)) {
        auto *widget = m_taskWidgets.take(taskId);
        m_taskListLayout->removeWidget(widget);
        widget->deleteLater();
    }
}

void TaskListWidget::highlightCurrentTask(int taskId)
{
    m_currentTaskId = taskId;

    // 更新所有任务项的高亮状态
    for (auto it = m_taskWidgets.constBegin(); it != m_taskWidgets.constEnd(); ++it) {
        TaskItemWidget *widget = it.value();
        if (widget->taskId() == taskId) {
            widget->setStyleSheet(
                "TaskItemWidget {"
                "  background-color: #e3f2fd;"
                "  border: 2px solid #2196F3;"
                "  border-radius: 4px;"
                "  padding: 8px;"
                "  margin: 2px;"
                "}"
            );
        } else {
            widget->setStyleSheet(
                "TaskItemWidget {"
                "  background-color: white;"
                "  border: 1px solid #ddd;"
                "  border-radius: 4px;"
                "  padding: 8px;"
                "  margin: 2px;"
                "}"
            );
        }
    }
}
