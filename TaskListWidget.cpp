// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskListWidget.h"
#include "TaskDialog.h"
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QDir>

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

    // 设置默认宽度
    setMinimumWidth(m_expandedWidth);
    setMaximumWidth(m_expandedWidth);
}

void TaskListWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);  // 去除外边距，让组件填满整个面板
    mainLayout->setSpacing(0);  // 去除间距，让组件紧密连接

    // 标题栏（包含标题和Pin按钮）
    auto *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #2196F3;"  // 蓝色背景
        "  border-top-left-radius: 8px;"
        "  border-top-right-radius: 8px;"
        "}"
    );
    auto *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 10, 12, 10);
    headerLayout->setSpacing(8);

    auto *titleLabel = new QLabel("任务列表", headerWidget);
    titleLabel->setStyleSheet(
        "font-size: 15px;"
        "font-weight: bold;"
        "color: white;"  // 白色文字
        "background: transparent;"
    );

    // Pin按钮
    m_pinButton = new QPushButton("📌", headerWidget);
    m_pinButton->setFixedSize(32, 32);
    m_pinButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.2);"
        "  border: 1px solid rgba(255, 255, 255, 0.3);"
        "  border-radius: 4px;"
        "  font-size: 16px;"
        "  color: white;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.3);"
        "}"
    );
    m_pinButton->setToolTip("固定侧边栏");
    connect(m_pinButton, &QPushButton::clicked, this, &TaskListWidget::onPinToggled);

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(m_pinButton);

    mainLayout->addWidget(headerWidget);

    // 内容容器（包含创建按钮、列表等，带内边距）
    auto *contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background-color: #fafafa;");
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(12, 12, 12, 12);  // 内容区域的内边距
    contentLayout->setSpacing(8);  // 内容组件之间的间距

    // 创建任务按钮
    m_createButton = new QPushButton("+ 创建新任务", contentWidget);
    m_createButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );
    connect(m_createButton, &QPushButton::clicked, this, &TaskListWidget::onCreateTask);

    contentLayout->addWidget(m_createButton);

    // 导入导出按钮容器
    auto *ioButtonContainer = new QWidget(contentWidget);
    auto *ioButtonLayout = new QHBoxLayout(ioButtonContainer);
    ioButtonLayout->setContentsMargins(0, 0, 0, 0);
    ioButtonLayout->setSpacing(8);

    QString ioButtonStyle =
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px 12px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}";

    m_exportButton = new QPushButton("导出任务", ioButtonContainer);
    m_exportButton->setStyleSheet(ioButtonStyle);
    connect(m_exportButton, &QPushButton::clicked, this, &TaskListWidget::onExportTasks);

    m_importButton = new QPushButton("导入任务", ioButtonContainer);
    m_importButton->setStyleSheet(ioButtonStyle);
    connect(m_importButton, &QPushButton::clicked, this, &TaskListWidget::onImportTasks);

    ioButtonLayout->addWidget(m_exportButton);
    ioButtonLayout->addWidget(m_importButton);

    contentLayout->addWidget(ioButtonContainer);

    // 列标题
    auto *headerFrame = new QFrame(contentWidget);
    headerFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #e0e0e0;"
        "  border-radius: 4px;"
        "}"
    );
    auto *columnHeaderLayout = new QHBoxLayout(headerFrame);
    columnHeaderLayout->setContentsMargins(6, 3, 6, 3);
    columnHeaderLayout->setSpacing(6);

    auto *visibilityHeader = new QLabel("显示", headerFrame);
    visibilityHeader->setStyleSheet("font-weight: bold; font-size: 11px;");
    visibilityHeader->setFixedWidth(50);

    auto *descriptionHeader = new QLabel("任务", headerFrame);
    descriptionHeader->setStyleSheet("font-weight: bold; font-size: 11px;");

    auto *actionHeader = new QLabel("操作", headerFrame);
    actionHeader->setStyleSheet("font-weight: bold; font-size: 11px;");
    actionHeader->setFixedWidth(60);

    columnHeaderLayout->addWidget(visibilityHeader);
    columnHeaderLayout->addWidget(descriptionHeader, 1);
    columnHeaderLayout->addWidget(actionHeader);

    contentLayout->addWidget(headerFrame);

    // 任务列表区域（滚动）
    auto *scrollArea = new QScrollArea(contentWidget);
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
    m_taskListLayout->setSpacing(4);  // 减小任务项之间的间距
    m_taskListLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    contentLayout->addWidget(scrollArea, 1);

    // 将内容容器添加到主布局
    mainLayout->addWidget(contentWidget, 1);

    // 设置整体样式
    setStyleSheet(
        "TaskListWidget {"
        "  background-color: #fafafa;"
        "  border-radius: 8px;"
        "  border: 1px solid #ccc;"
        "}"
    );

    // 创建透明度效果
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);  // 初始完全不透明
    setGraphicsEffect(m_opacityEffect);
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

void TaskListWidget::setCollapsible(bool collapsible)
{
    m_collapsible = collapsible;

    if (m_collapsible) {
        // 初始化为收缩状态
        m_collapsed = true;
        setMinimumWidth(m_collapsedWidth);
        setMaximumWidth(m_collapsedWidth);
        resize(m_collapsedWidth, height());  // 立即调整大小

        // 启用鼠标追踪
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);  // 启用 hover 事件

        // 在收缩状态下，改变样式以提供视觉提示
        updateStyleForCollapsedState();

        qDebug() << "TaskListWidget 设置为可收缩模式，当前宽度:" << width();
    } else {
        // 恢复展开状态
        m_collapsed = false;
        setMinimumWidth(m_expandedWidth);
        setMaximumWidth(m_expandedWidth);
        resize(m_expandedWidth, height());
    }
}

void TaskListWidget::updateStyleForCollapsedState()
{
    if (m_collapsed) {
        // 收缩状态：隐藏所有子控件，只显示一个触发条
        for (QObject *child : children()) {
            if (QWidget *widget = qobject_cast<QWidget*>(child)) {
                widget->hide();
            }
        }

        // 收缩状态：显示一个带有箭头提示的条
        setStyleSheet(
            "TaskListWidget {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(33, 150, 243, 200), stop:1 rgba(33, 150, 243, 100));"
            "  border-top-right-radius: 8px;"
            "  border-bottom-right-radius: 8px;"
            "  border-right: 3px solid #2196F3;"
            "}"
        );

        qDebug() << "TaskListWidget 收缩状态：子控件已隐藏";
    } else {
        // 展开状态：显示所有子控件
        for (QObject *child : children()) {
            if (QWidget *widget = qobject_cast<QWidget*>(child)) {
                widget->show();
            }
        }

        // 展开状态：恢复原样式
        setStyleSheet(
            "TaskListWidget {"
            "  background-color: #fafafa;"
            "  border-radius: 8px;"
            "  border: 1px solid #ccc;"
            "}"
        );

        qDebug() << "TaskListWidget 展开状态：子控件已显示";
    }
}

void TaskListWidget::expand()
{
    if (!m_collapsed) {
        qDebug() << "TaskListWidget::expand() - 已经是展开状态";
        return;
    }

    qDebug() << "TaskListWidget::expand() - 开始展开动画";
    m_collapsed = false;
    updateStyleForCollapsedState();  // 更新样式

    auto *animation = new QPropertyAnimation(this, "minimumWidth");
    animation->setDuration(200);
    animation->setStartValue(m_collapsedWidth);
    animation->setEndValue(m_expandedWidth);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    auto *animation2 = new QPropertyAnimation(this, "maximumWidth");
    animation2->setDuration(200);
    animation2->setStartValue(m_collapsedWidth);
    animation2->setEndValue(m_expandedWidth);
    animation2->setEasingCurve(QEasingCurve::OutCubic);

    // 展开完成后，如果鼠标在窗口内，设为完全不透明；否则设为半透明
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        if (m_opacityEffect) {
            if (underMouse()) {
                m_opacityEffect->setOpacity(1.0);
                qDebug() << "展开完成 - 鼠标在窗口内，设置透明度为 100%";
            } else {
                m_opacityEffect->setOpacity(0.65);
                qDebug() << "展开完成 - 鼠标在窗口外，设置透明度为 65%";
            }
        }
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
    animation2->start(QAbstractAnimation::DeleteWhenStopped);

    emit expandStateChanged(true);
}

void TaskListWidget::collapse()
{
    if (m_collapsed || !m_collapsible || m_pinned) return;

    m_collapsed = true;

    auto *animation = new QPropertyAnimation(this, "minimumWidth");
    animation->setDuration(200);
    animation->setStartValue(m_expandedWidth);
    animation->setEndValue(m_collapsedWidth);
    animation->setEasingCurve(QEasingCurve::InCubic);

    auto *animation2 = new QPropertyAnimation(this, "maximumWidth");
    animation2->setDuration(200);
    animation2->setStartValue(m_expandedWidth);
    animation2->setEndValue(m_collapsedWidth);
    animation2->setEasingCurve(QEasingCurve::InCubic);

    // 动画完成后再更新样式和隐藏子控件，并设置为完全不透明（收缩状态下应完全可见）
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
        updateStyleForCollapsedState();
        if (m_opacityEffect) {
            m_opacityEffect->setOpacity(1.0);  // 收缩状态下，蓝色触发条应该完全可见
            qDebug() << "收缩完成 - 设置透明度为 100%";
        }
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
    animation2->start(QAbstractAnimation::DeleteWhenStopped);

    emit expandStateChanged(false);
}

void TaskListWidget::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    qDebug() << "TaskListWidget::enterEvent() - 鼠标进入，collapsible:" << m_collapsible << "pinned:" << m_pinned << "collapsed:" << m_collapsed;

    // 鼠标进入时，设置为完全不透明
    if (!m_collapsed && m_opacityEffect) {
        m_opacityEffect->setOpacity(1.0);
        qDebug() << "设置透明度为 100% (不透明)";
    }

    if (m_collapsible && !m_pinned) {
        expand();
    }
}

void TaskListWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    // 鼠标离开时，设置为半透明（65%不透明度）
    if (!m_collapsed && m_opacityEffect) {
        m_opacityEffect->setOpacity(0.65);
        qDebug() << "设置透明度为 65% (半透明)";
    }

    if (m_collapsible && !m_pinned) {
        // 延迟收缩，给用户时间移动鼠标回来
        QTimer::singleShot(300, this, [this]() {
            // 如果有任何模态对话框打开，不收缩
            if (QApplication::activeModalWidget()) {
                qDebug() << "TaskListWidget::leaveEvent - 检测到模态对话框，不收缩";
                return;
            }

            if (!underMouse() && !m_pinned && m_collapsible) {
                collapse();
            }
        });
    }
}

void TaskListWidget::onPinToggled()
{
    m_pinned = !m_pinned;
    updatePinButtonIcon();

    if (m_pinned) {
        // 固定时展开
        expand();
    }
}

void TaskListWidget::updatePinButtonIcon()
{
    if (m_pinned) {
        m_pinButton->setText("📍");
        m_pinButton->setToolTip("取消固定");
        m_pinButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(255, 255, 255, 0.4);"
            "  border: 1px solid rgba(255, 255, 255, 0.6);"
            "  border-radius: 4px;"
            "  font-size: 16px;"
            "  color: white;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(255, 255, 255, 0.5);"
            "}"
        );
    } else {
        m_pinButton->setText("📌");
        m_pinButton->setToolTip("固定侧边栏");
        m_pinButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(255, 255, 255, 0.2);"
            "  border: 1px solid rgba(255, 255, 255, 0.3);"
            "  border-radius: 4px;"
            "  font-size: 16px;"
            "  color: white;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(255, 255, 255, 0.3);"
            "}"
        );
    }
}

void TaskListWidget::onExportTasks()
{
    // 获取所有任务
    QList<Task*> allTasks = m_taskManager->getAllTasks();
    if (allTasks.isEmpty()) {
        QMessageBox::information(this, "导出任务", "当前没有任务可导出！");
        return;
    }

    // 打开文件保存对话框
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出任务",
        QDir::homePath() + "/tasks.json",
        "JSON 文件 (*.json)"
    );

    if (fileName.isEmpty()) {
        return;  // 用户取消
    }

    // 创建 JSON 对象
    QJsonObject rootObj;
    rootObj["version"] = "1.0";
    rootObj["export_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray tasksArray;
    for (Task *task : allTasks) {
        QJsonObject taskObj;
        taskObj["id"] = task->id();
        taskObj["name"] = task->name();
        taskObj["description"] = task->description();
        taskObj["visible"] = task->isVisible();

        // 序列化任务中的所有元素
        QJsonArray elementsArray;
        for (const MapElement &element : task->elements()) {
            QJsonObject elemObj;
            elemObj["type"] = static_cast<int>(element.type);
            elemObj["annotationId"] = static_cast<qint64>(element.annotationId);
            elemObj["terrainType"] = static_cast<int>(element.terrainType);

            // 根据类型保存坐标数据
            switch (element.type) {
            case MapElement::LoiterPoint:
            case MapElement::UAV:
                {
                    QJsonObject coordObj;
                    coordObj["lat"] = element.coordinate.first;
                    coordObj["lon"] = element.coordinate.second;
                    elemObj["coordinate"] = coordObj;
                }
                if (element.type == MapElement::UAV) {
                    elemObj["color"] = element.color;
                }
                break;

            case MapElement::NoFlyZone:
                {
                    QJsonObject centerObj;
                    centerObj["lat"] = element.coordinate.first;
                    centerObj["lon"] = element.coordinate.second;
                    elemObj["center"] = centerObj;
                    elemObj["radius"] = element.radius;
                }
                break;

            case MapElement::Polygon:
                {
                    QJsonArray coordsArray;
                    for (const auto &coord : element.vertices) {
                        QJsonObject coordObj;
                        coordObj["lat"] = coord.first;
                        coordObj["lon"] = coord.second;
                        coordsArray.append(coordObj);
                    }
                    elemObj["coordinates"] = coordsArray;
                }
                break;
            }

            elementsArray.append(elemObj);
        }
        taskObj["elements"] = elementsArray;

        tasksArray.append(taskObj);
    }
    rootObj["tasks"] = tasksArray;

    // 写入文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "导出失败", QString("无法写入文件: %1").arg(file.errorString()));
        return;
    }

    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "导出成功", QString("成功导出 %1 个任务到:\n%2")
                             .arg(allTasks.size()).arg(fileName));
    qDebug() << QString("导出 %1 个任务到: %2").arg(allTasks.size()).arg(fileName);
}

void TaskListWidget::onImportTasks()
{
    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "导入任务",
        QDir::homePath(),
        "JSON 文件 (*.json)"
    );

    if (fileName.isEmpty()) {
        return;  // 用户取消
    }

    // 读取文件
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "导入失败", QString("无法读取文件: %1").arg(file.errorString()));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // 解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "导入失败", "文件格式错误，不是有效的 JSON 文件！");
        return;
    }

    QJsonObject rootObj = doc.object();
    if (!rootObj.contains("tasks") || !rootObj["tasks"].isArray()) {
        QMessageBox::critical(this, "导入失败", "文件格式错误，缺少任务数据！");
        return;
    }

    QJsonArray tasksArray = rootObj["tasks"].toArray();
    int importedCount = 0;
    int skippedCount = 0;

    for (const QJsonValue &taskValue : tasksArray) {
        if (!taskValue.isObject()) continue;

        {  // 使用大括号避免 goto 跨越初始化
        QJsonObject taskObj = taskValue.toObject();
        int taskId = taskObj["id"].toInt();
        QString taskName = taskObj["name"].toString();
        QString taskDescription = taskObj["description"].toString();
        bool taskVisible = taskObj["visible"].toBool(true);

        // 检查冲突
        Task *existingTask = m_taskManager->getTask(taskId);
        bool hasConflict = false;

        if (existingTask) {
            // ID 冲突
            hasConflict = true;
        } else {
            // 检查名称冲突
            for (Task *task : m_taskManager->getAllTasks()) {
                if (task->name() == taskName) {
                    hasConflict = true;
                    break;
                }
            }
        }

        if (hasConflict) {
            // 弹出对话框让用户选择
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("任务冲突");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(QString("任务冲突: ID=%1, 名称=%2").arg(taskId).arg(taskName));
            msgBox.setInformativeText("请选择如何处理:");

            QPushButton *skipBtn = msgBox.addButton("跳过此任务", QMessageBox::RejectRole);
            QPushButton *renameBtn = msgBox.addButton("修改ID和名称", QMessageBox::AcceptRole);
            msgBox.setDefaultButton(renameBtn);

            msgBox.exec();

            if (msgBox.clickedButton() == skipBtn) {
                skippedCount++;
                qDebug() << QString("跳过冲突任务: ID=%1, 名称=%2").arg(taskId).arg(taskName);
                continue;
            } else {
                // 让用户输入新的 ID
                bool ok;
                int newId = QInputDialog::getInt(this, "修改任务ID",
                                                  QString("原ID: %1\n请输入新的任务ID:").arg(taskId),
                                                  taskId, 1, 999999, 1, &ok);
                if (!ok) {
                    skippedCount++;
                    continue;
                }

                // 检查新ID是否也冲突
                while (m_taskManager->getTask(newId)) {
                    newId = QInputDialog::getInt(this, "ID仍然冲突",
                                                  QString("ID %1 已存在，请输入其他ID:").arg(newId),
                                                  newId + 1, 1, 999999, 1, &ok);
                    if (!ok) {
                        skippedCount++;
                        continue;
                    }
                }

                if (!ok) continue;

                // 让用户输入新的名称
                QString newName = QInputDialog::getText(this, "修改任务名称",
                                                         QString("原名称: %1\n请输入新的任务名称:").arg(taskName),
                                                         QLineEdit::Normal, taskName, &ok);
                if (!ok || newName.isEmpty()) {
                    skippedCount++;
                    continue;
                }

                // 检查新名称是否也冲突
                bool nameConflict = false;
                do {
                    nameConflict = false;
                    for (Task *task : m_taskManager->getAllTasks()) {
                        if (task->name() == newName) {
                            nameConflict = true;
                            break;
                        }
                    }
                    if (nameConflict) {
                        newName = QInputDialog::getText(this, "名称仍然冲突",
                                                         QString("名称 '%1' 已存在，请输入其他名称:").arg(newName),
                                                         QLineEdit::Normal, newName + "_导入", &ok);
                        if (!ok || newName.isEmpty()) {
                            skippedCount++;
                            break;
                        }
                    }
                } while (nameConflict);

                if (!ok || newName.isEmpty()) continue;

                taskId = newId;
                taskName = newName;
            }
        }

        // 创建任务
        Task *newTask = m_taskManager->createTask(taskId, taskName, taskDescription);
        if (!newTask) {
            qWarning() << QString("创建任务失败: ID=%1").arg(taskId);
            skippedCount++;
            continue;
        }

        newTask->setVisible(taskVisible);

        // 导入元素
        QJsonArray elementsArray = taskObj["elements"].toArray();
        for (const QJsonValue &elemValue : elementsArray) {
            if (!elemValue.isObject()) continue;

            QJsonObject elemObj = elemValue.toObject();
            MapElement::Type type = static_cast<MapElement::Type>(elemObj["type"].toInt());
            MapElement::TerrainType terrainType = static_cast<MapElement::TerrainType>(elemObj["terrainType"].toInt());

            switch (type) {
            case MapElement::LoiterPoint: {
                QJsonObject coordObj = elemObj["coordinate"].toObject();
                double lat = coordObj["lat"].toDouble();
                double lon = coordObj["lon"].toDouble();
                auto id = m_taskManager->addLoiterPointToTask(taskId, lat, lon);
                if (id > 0) {
                    MapElement *element = newTask->findElement(id);
                    if (element) element->terrainType = terrainType;
                }
                break;
            }

            case MapElement::NoFlyZone: {
                QJsonObject centerObj = elemObj["center"].toObject();
                double lat = centerObj["lat"].toDouble();
                double lon = centerObj["lon"].toDouble();
                double radius = elemObj["radius"].toDouble();
                auto id = m_taskManager->addNoFlyZoneToTask(taskId, lat, lon, radius);
                if (id > 0) {
                    MapElement *element = newTask->findElement(id);
                    if (element) element->terrainType = terrainType;
                }
                break;
            }

            case MapElement::UAV: {
                QJsonObject coordObj = elemObj["coordinate"].toObject();
                double lat = coordObj["lat"].toDouble();
                double lon = coordObj["lon"].toDouble();
                QString color = elemObj["color"].toString("black");
                auto id = m_taskManager->addUAVToTask(taskId, lat, lon, color);
                if (id > 0) {
                    MapElement *element = newTask->findElement(id);
                    if (element) element->terrainType = terrainType;
                }
                break;
            }

            case MapElement::Polygon: {
                QJsonArray coordsArray = elemObj["coordinates"].toArray();
                QMapLibre::Coordinates coords;
                for (const QJsonValue &coordValue : coordsArray) {
                    QJsonObject coordObj = coordValue.toObject();
                    double lat = coordObj["lat"].toDouble();
                    double lon = coordObj["lon"].toDouble();
                    coords.append(QMapLibre::Coordinate(lat, lon));
                }
                if (coords.size() >= 3) {
                    auto id = m_taskManager->addPolygonToTask(taskId, coords);
                    if (id > 0) {
                        MapElement *element = newTask->findElement(id);
                        if (element) element->terrainType = terrainType;
                    }
                }
                break;
            }
            }
        }

        importedCount++;
        qDebug() << QString("导入任务: ID=%1, 名称=%2, 元素数=%3")
                    .arg(taskId).arg(taskName).arg(elementsArray.size());
        }  // 关闭大括号
    }

    // 显示导入结果
    QString resultMsg = QString("导入完成!\n成功: %1 个任务\n跳过: %2 个任务")
                        .arg(importedCount).arg(skippedCount);
    QMessageBox::information(this, "导入结果", resultMsg);
    qDebug() << resultMsg;
}
