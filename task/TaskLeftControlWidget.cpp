// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskLeftControlWidget.h"
#include "map_region/Region.h"
#include <QDebug>
#include <QApplication>
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

TaskLeftControlWidget::TaskLeftControlWidget(TaskManager *taskManager, QWidget *parent)
    : QWidget(parent)
    , m_taskManager(taskManager)
    , m_currentTaskId(-1)
{
    setupUI();

    // 连接信号
    connect(m_taskManager, &TaskManager::taskCreated,
            this, &TaskLeftControlWidget::onTaskCreated);
    connect(m_taskManager, &TaskManager::taskRemoved,
            this, &TaskLeftControlWidget::onTaskRemoved);
    connect(m_taskManager, &TaskManager::currentTaskChanged,
            this, &TaskLeftControlWidget::onCurrentTaskChanged);

    // 连接区域管理器信号，自动刷新区域列表
    connect(m_taskManager->regionManager(), &RegionManager::regionCreated,
            this, &TaskLeftControlWidget::onRegionListChanged);
    connect(m_taskManager->regionManager(), &RegionManager::regionRemoved,
            this, &TaskLeftControlWidget::onRegionListChanged);

    // 设置默认宽度（展开状态：主内容 + 收缩条）
    setFixedWidth(m_expandedWidth + m_collapsedWidth);
}

void TaskLeftControlWidget::setupUI()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ============ 收缩条（常驻小列）============
    m_collapsedBar = new QWidget(this);
    m_collapsedBar->setFixedWidth(m_collapsedWidth);
    m_collapsedBar->setStyleSheet(
        "QWidget {"
        "  background-color: rgba(224, 224, 224, 180);"  // 浅灰色，70%不透明度
        "}"
    );

    auto *collapsedLayout = new QVBoxLayout(m_collapsedBar);
    collapsedLayout->setContentsMargins(0, 0, 0, 0);
    collapsedLayout->setSpacing(8);  // 按钮之间的间距

    QString buttonStyle =
        "QPushButton {"
        "  background-color: rgba(100, 100, 100, 150);"
        "  border: 1px solid rgba(80, 80, 80, 200);"
        "  border-radius: 4px;"
        "  color: white;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  padding: 2px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(80, 80, 80, 180);"
        "}";

    collapsedLayout->addSpacing(10);  // 顶部留白

    // 【任务区域】按钮
    m_regionButton = new QPushButton("任务\n区域", m_collapsedBar);
    m_regionButton->setFixedSize(m_collapsedWidth - 4, 50);
    m_regionButton->setStyleSheet(buttonStyle);
    m_regionButton->setToolTip("查看任务区域");
    m_regionButton->setCursor(Qt::PointingHandCursor);
    connect(m_regionButton, &QPushButton::clicked, this, &TaskLeftControlWidget::onRegionButtonClicked);
    collapsedLayout->addWidget(m_regionButton, 0, Qt::AlignCenter);

    // 【任务方案】按钮（保留但无功能）
    m_taskPlanButton = new QPushButton("任务\n方案", m_collapsedBar);
    m_taskPlanButton->setFixedSize(m_collapsedWidth - 4, 50);
    m_taskPlanButton->setStyleSheet(buttonStyle);
    m_taskPlanButton->setToolTip("任务方案");
    m_taskPlanButton->setCursor(Qt::PointingHandCursor);
    // 不连接任何槽函数
    collapsedLayout->addWidget(m_taskPlanButton, 0, Qt::AlignCenter);

    // 【行动方案】按钮（保留但无功能）
    m_actionButton = new QPushButton("行动\n方案", m_collapsedBar);
    m_actionButton->setFixedSize(m_collapsedWidth - 4, 50);
    m_actionButton->setStyleSheet(buttonStyle);
    m_actionButton->setToolTip("行动方案");
    m_actionButton->setCursor(Qt::PointingHandCursor);
    connect(m_actionButton, &QPushButton::clicked, this, &TaskLeftControlWidget::onActionButtonClicked);
    collapsedLayout->addWidget(m_actionButton, 0, Qt::AlignCenter);

    // 剩余空间填充
    collapsedLayout->addStretch();

    mainLayout->addWidget(m_collapsedBar);

    // ============ 主内容区域（展开状态）============
    m_mainContent = new QWidget(this);
    m_mainContent->setFixedWidth(m_expandedWidth);
    m_mainContent->setStyleSheet(
        "QWidget {"
        "  background-color: #fafafa;"
        "  border-radius: 8px;"
        "  border: 1px solid #ccc;"
        "}"
    );

    auto *mainContentLayout = new QVBoxLayout(m_mainContent);
    mainContentLayout->setContentsMargins(0, 0, 0, 0);
    mainContentLayout->setSpacing(0);

    // 标题栏（包含标题和关闭按钮）
    auto *headerWidget = new QWidget(m_mainContent);
    headerWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #2196F3;"
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
        "color: white;"
        "background: transparent;"
    );

    // 关闭按钮
    m_closeButton = new QPushButton("✕", headerWidget);
    m_closeButton->setFixedSize(32, 32);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.2);"
        "  border: 1px solid rgba(255, 255, 255, 0.3);"
        "  border-radius: 4px;"
        "  font-size: 18px;"
        "  color: white;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(244, 67, 54, 0.8);"
        "}"
    );
    m_closeButton->setToolTip("收起任务列表");
    m_closeButton->setCursor(Qt::PointingHandCursor);
    connect(m_closeButton, &QPushButton::clicked, this, &TaskLeftControlWidget::collapse);

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(m_closeButton);

    mainContentLayout->addWidget(headerWidget);

    // 内容容器
    auto *contentWidget = new QWidget(m_mainContent);
    contentWidget->setStyleSheet("background-color: #fafafa;");
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(8);

    // 移除创建任务按钮

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
    connect(m_exportButton, &QPushButton::clicked, this, &TaskLeftControlWidget::onExportTasks);

    m_importButton = new QPushButton("导入任务", ioButtonContainer);
    m_importButton->setStyleSheet(ioButtonStyle);
    connect(m_importButton, &QPushButton::clicked, this, &TaskLeftControlWidget::onImportTasks);

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
    m_taskListLayout->setSpacing(4);
    m_taskListLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    contentLayout->addWidget(scrollArea, 1);

    mainContentLayout->addWidget(contentWidget, 1);

    mainLayout->addWidget(m_mainContent);

    // 设置整体样式（不透明）
    setStyleSheet(
        "TaskListWidget {"
        "  background-color: transparent;"
        "}"
    );

    // ============ 区域列表窗口 ============
    // 注意：使用 parentWidget() 作为父widget，这样窗口不会被限制在TaskListWidget内
    m_regionListWidget = new QWidget(parentWidget());
    m_regionListWidget->setFixedSize(300, 400);
    m_regionListWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #fafafa;"
        "  border-radius: 8px;"
        "  border: 1px solid #ccc;"
        "}"
    );
    m_regionListWidget->hide();  // 默认隐藏

    auto *regionLayout = new QVBoxLayout(m_regionListWidget);
    regionLayout->setContentsMargins(0, 0, 0, 0);
    regionLayout->setSpacing(0);

    // 标题栏
    auto *regionHeaderWidget = new QWidget(m_regionListWidget);
    regionHeaderWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #2196F3;"
        "  border-top-left-radius: 8px;"
        "  border-top-right-radius: 8px;"
        "}"
    );
    auto *regionHeaderLayout = new QHBoxLayout(regionHeaderWidget);
    regionHeaderLayout->setContentsMargins(12, 10, 12, 10);
    regionHeaderLayout->setSpacing(8);

    auto *regionTitleLabel = new QLabel("任务区域列表", regionHeaderWidget);
    regionTitleLabel->setStyleSheet(
        "font-size: 15px;"
        "font-weight: bold;"
        "color: white;"
        "background: transparent;"
    );

    // 关闭按钮
    auto *regionCloseButton = new QPushButton("✕", regionHeaderWidget);
    regionCloseButton->setFixedSize(32, 32);
    regionCloseButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.2);"
        "  border: 1px solid rgba(255, 255, 255, 0.3);"
        "  border-radius: 4px;"
        "  font-size: 18px;"
        "  color: white;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(244, 67, 54, 0.8);"
        "}"
    );
    regionCloseButton->setToolTip("关闭区域列表");
    regionCloseButton->setCursor(Qt::PointingHandCursor);
    connect(regionCloseButton, &QPushButton::clicked, m_regionListWidget, &QWidget::hide);

    regionHeaderLayout->addWidget(regionTitleLabel, 1);
    regionHeaderLayout->addWidget(regionCloseButton);

    regionLayout->addWidget(regionHeaderWidget);

    // 区域列表内容（滚动区域）
    auto *regionScrollArea = new QScrollArea(m_regionListWidget);
    regionScrollArea->setWidgetResizable(true);
    regionScrollArea->setStyleSheet(
        "QScrollArea {"
        "  border: none;"
        "  background-color: transparent;"
        "}"
    );

    auto *regionScrollWidget = new QWidget();
    regionScrollWidget->setStyleSheet("background-color: #fafafa;");
    m_regionContentLayout = new QVBoxLayout(regionScrollWidget);
    m_regionContentLayout->setContentsMargins(12, 12, 12, 12);
    m_regionContentLayout->setSpacing(8);

    // 添加占位文字
    auto *placeholderLabel = new QLabel("暂无任务区域", regionScrollWidget);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setStyleSheet("color: #999; padding: 20px;");
    m_regionContentLayout->addWidget(placeholderLabel);
    m_regionContentLayout->addStretch();

    regionScrollArea->setWidget(regionScrollWidget);
    regionLayout->addWidget(regionScrollArea, 1);
}

void TaskLeftControlWidget::refreshTaskList()
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

void TaskLeftControlWidget::onTaskCreated(int taskId)
{
    Task *task = m_taskManager->getTask(taskId);
    if (task) {
        addTaskItem(task);
    }
}

void TaskLeftControlWidget::onTaskRemoved(int taskId)
{
    removeTaskItem(taskId);
}

void TaskLeftControlWidget::onCurrentTaskChanged(int taskId)
{
    highlightCurrentTask(taskId);
}

// onCreateTask 函数已删除

void TaskLeftControlWidget::onTaskVisibilityToggled(int taskId, bool visible)
{
    m_taskManager->setTaskVisible(taskId, visible);
}

void TaskLeftControlWidget::onTaskSelected(int taskId)
{
    m_taskManager->setCurrentTask(taskId);
}

void TaskLeftControlWidget::onTaskDeleteRequested(int taskId)
{
    m_taskManager->removeTask(taskId);
}

void TaskLeftControlWidget::onTaskEditRequested(int taskId)
{
    // 编辑功能已禁用
    qDebug() << QString("任务 #%1 编辑功能已禁用").arg(taskId);
}

void TaskLeftControlWidget::addTaskItem(Task *task)
{
    auto *taskWidget = new TaskItemWidget(task, this);

    connect(taskWidget, &TaskItemWidget::visibilityToggled,
            this, &TaskLeftControlWidget::onTaskVisibilityToggled);
    connect(taskWidget, &TaskItemWidget::selected,
            this, &TaskLeftControlWidget::onTaskSelected);
    connect(taskWidget, &TaskItemWidget::deleteRequested,
            this, &TaskLeftControlWidget::onTaskDeleteRequested);
    connect(taskWidget, &TaskItemWidget::editRequested,
            this, &TaskLeftControlWidget::onTaskEditRequested);

    // 插入到列表末尾（stretch 之前）
    m_taskListLayout->insertWidget(m_taskListLayout->count() - 1, taskWidget);
    m_taskWidgets[task->id()] = taskWidget;
}

void TaskLeftControlWidget::removeTaskItem(int taskId)
{
    if (m_taskWidgets.contains(taskId)) {
        auto *widget = m_taskWidgets.take(taskId);
        m_taskListLayout->removeWidget(widget);
        widget->deleteLater();
    }
}

void TaskLeftControlWidget::highlightCurrentTask(int taskId)
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

void TaskLeftControlWidget::setCollapsible(bool collapsible)
{
    m_collapsible = collapsible;

    if (m_collapsible) {
        // 初始化为收缩状态
        m_collapsed = true;
        m_mainContent->hide();
        m_collapsedBar->show();
        setFixedWidth(m_collapsedWidth);
        qDebug() << "TaskListWidget 设置为可收缩模式";
    } else {
        // 恢复展开状态
        m_collapsed = false;
        m_collapsedBar->hide();
        m_mainContent->show();
        setFixedWidth(m_expandedWidth + m_collapsedWidth);
    }
}

void TaskLeftControlWidget::expand()
{
    if (!m_collapsed) {
        qDebug() << "TaskLeftControlWidget::expand() - 已经是展开状态";
        return;
    }

    qDebug() << "TaskLeftControlWidget::expand() - 立即展开";
    m_collapsed = false;

    // 立即显示主内容，保持收缩条可见
    m_mainContent->show();
    setFixedWidth(m_expandedWidth + m_collapsedWidth);

    emit expandStateChanged(true);
}

void TaskLeftControlWidget::collapse()
{
    if (m_collapsed || !m_collapsible) {
        return;
    }

    qDebug() << "TaskLeftControlWidget::collapse() - 立即收缩";
    m_collapsed = true;

    // 立即隐藏主内容，只保留收缩条
    m_mainContent->hide();
    setFixedWidth(m_collapsedWidth);

    emit expandStateChanged(false);
}

void TaskLeftControlWidget::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    // 移除自动展开行为，现在需要点击按钮才能展开
}

void TaskLeftControlWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    // 移除自动收缩行为，现在需要点击关闭按钮才能收缩
}

void TaskLeftControlWidget::onRegionButtonClicked()
{
    qDebug() << "【任务区域】按钮被点击";

    // 关闭任务方案窗口
    collapse();

    // 刷新区域列表
    refreshRegionList();

    // 显示区域列表窗口
    m_regionListWidget->show();
    m_regionListWidget->raise();

    // 定位到地图左上角（小列右侧）
    m_regionListWidget->move(m_collapsedWidth + 10, 10);
}

void TaskLeftControlWidget::onActionButtonClicked()
{
    qDebug() << "【行动方案】按钮被点击（暂无功能）";
    // 保留按钮但暂无功能
}

void TaskLeftControlWidget::onRegionListChanged()
{
    // 只有当区域列表窗口可见时才刷新
    if (m_regionListWidget && m_regionListWidget->isVisible()) {
        refreshRegionList();
    }
}

void TaskLeftControlWidget::onExportTasks()
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

        // 序列化任务中的所有区域
        QJsonArray regionsArray;
        for (int regionId : task->regionIds()) {
            Region *region = m_taskManager->regionManager()->getRegion(regionId);
            if (!region) continue;

            QJsonObject regionObj;
            regionObj["type"] = static_cast<int>(region->type());
            regionObj["annotationId"] = static_cast<qint64>(region->annotationId());
            regionObj["terrainType"] = static_cast<int>(region->terrainType());

            // 根据类型保存坐标数据
            switch (region->type()) {
            case RegionType::LoiterPoint:
            case RegionType::UAV:
                {
                    QJsonObject coordObj;
                    coordObj["lat"] = region->coordinate().first;
                    coordObj["lon"] = region->coordinate().second;
                    regionObj["coordinate"] = coordObj;
                }
                if (region->type() == RegionType::UAV) {
                    regionObj["color"] = region->color();
                }
                break;

            case RegionType::NoFlyZone:
                {
                    QJsonObject centerObj;
                    centerObj["lat"] = region->coordinate().first;
                    centerObj["lon"] = region->coordinate().second;
                    regionObj["center"] = centerObj;
                    regionObj["radius"] = region->radius();
                }
                break;

            case RegionType::Polygon:
                {
                    QJsonArray coordsArray;
                    for (const auto &coord : region->vertices()) {
                        QJsonObject coordObj;
                        coordObj["lat"] = coord.first;
                        coordObj["lon"] = coord.second;
                        coordsArray.append(coordObj);
                    }
                    regionObj["coordinates"] = coordsArray;
                }
                break;
            }

            regionsArray.append(regionObj);
        }
        taskObj["regions"] = regionsArray;

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

void TaskLeftControlWidget::onImportTasks()
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

        // 导入区域
        QJsonArray regionsArray = taskObj["regions"].toArray();
        for (const QJsonValue &regionValue : regionsArray) {
            if (!regionValue.isObject()) continue;

            QJsonObject regionObj = regionValue.toObject();
            Region::Type type = static_cast<Region::Type>(regionObj["type"].toInt());
            Region::TerrainType terrainType = static_cast<Region::TerrainType>(regionObj["terrainType"].toInt());

            switch (type) {
            case RegionType::LoiterPoint: {
                QJsonObject coordObj = regionObj["coordinate"].toObject();
                double lat = coordObj["lat"].toDouble();
                double lon = coordObj["lon"].toDouble();
                auto id = m_taskManager->addLoiterPointToTask(taskId, lat, lon);
                if (id > 0) {
                    m_taskManager->regionManager()->updateRegionTerrainType(id, terrainType);
                }
                break;
            }

            case RegionType::NoFlyZone: {
                QJsonObject centerObj = regionObj["center"].toObject();
                double lat = centerObj["lat"].toDouble();
                double lon = centerObj["lon"].toDouble();
                double radius = regionObj["radius"].toDouble();
                auto id = m_taskManager->addNoFlyZoneToTask(taskId, lat, lon, radius);
                if (id > 0) {
                    m_taskManager->regionManager()->updateRegionTerrainType(id, terrainType);
                }
                break;
            }

            case RegionType::UAV: {
                QJsonObject coordObj = regionObj["coordinate"].toObject();
                double lat = coordObj["lat"].toDouble();
                double lon = coordObj["lon"].toDouble();
                QString color = regionObj["color"].toString("black");
                auto id = m_taskManager->addUAVToTask(taskId, lat, lon, color);
                if (id > 0) {
                    m_taskManager->regionManager()->updateRegionTerrainType(id, terrainType);
                }
                break;
            }

            case RegionType::Polygon: {
                QJsonArray coordsArray = regionObj["coordinates"].toArray();
                QMapLibre::Coordinates coords;
                for (const QJsonValue &coordValue : coordsArray) {
                    QJsonObject coordObj = coordValue.toObject();
                    double lat = coordObj["lat"].toDouble();
                    double lon = coordObj["lon"].toDouble();
                    coords.append(QMapLibre::Coordinate(lat, lon));
                }
                if (coords.size() >= 3) {
                    auto id = m_taskManager->addTaskRegionToTask(taskId, coords);
                    if (id > 0) {
                        m_taskManager->regionManager()->updateRegionTerrainType(id, terrainType);
                    }
                }
                break;
            }
            }
        }

        importedCount++;
        qDebug() << QString("导入任务: ID=%1, 名称=%2, 区域数=%3")
                    .arg(taskId).arg(taskName).arg(regionsArray.size());
        }  // 关闭大括号
    }

    // 显示导入结果
    QString resultMsg = QString("导入完成!\n成功: %1 个任务\n跳过: %2 个任务")
                        .arg(importedCount).arg(skippedCount);
    QMessageBox::information(this, "导入结果", resultMsg);
    qDebug() << resultMsg;
}

void TaskLeftControlWidget::refreshRegionList()
{
    // 清空现有列表
    QLayoutItem *item;
    while ((item = m_regionContentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 获取所有区域
    const QMap<int, Region*>& allRegions = m_taskManager->regionManager()->getAllRegions();

    // 筛选任务区域
    QVector<Region*> polygons;
    for (Region *region : allRegions) {
        if (region && region->type() == RegionType::Polygon) {
            polygons.append(region);
        }
    }

    if (polygons.isEmpty()) {
        // 显示占位文字
        auto *placeholderLabel = new QLabel("暂无任务区域");
        placeholderLabel->setAlignment(Qt::AlignCenter);
        placeholderLabel->setStyleSheet("color: #999; padding: 20px;");
        m_regionContentLayout->addWidget(placeholderLabel);
    } else {
        // 显示任务区域列表
        for (Region *polygon : polygons) {
            // 计算面积
            double areaKm2 = calculateTaskRegionArea(polygon->vertices());

            // 创建区域项
            auto *regionItem = new QFrame();
            regionItem->setStyleSheet(
                "QFrame {"
                "  background-color: white;"
                "  border: 1px solid #ddd;"
                "  border-radius: 4px;"
                "  padding: 8px;"
                "}"
                "QFrame:hover {"
                "  background-color: #f5f5f5;"
                "}"
            );

            auto *itemLayout = new QVBoxLayout(regionItem);
            itemLayout->setContentsMargins(8, 6, 8, 6);
            itemLayout->setSpacing(4);

            // 区域ID
            auto *idLabel = new QLabel(QString("区域 ID: %1").arg(polygon->id()));
            idLabel->setStyleSheet("font-weight: bold; font-size: 12px;");

            // 区域面积
            auto *areaLabel = new QLabel(QString("面积: %1 km²").arg(areaKm2, 0, 'f', 2));
            areaLabel->setStyleSheet("color: #666; font-size: 11px;");

            itemLayout->addWidget(idLabel);
            itemLayout->addWidget(areaLabel);

            m_regionContentLayout->addWidget(regionItem);
        }
    }

    m_regionContentLayout->addStretch();
}

double TaskLeftControlWidget::calculateTaskRegionArea(const QMapLibre::Coordinates &coords)
{
    if (coords.size() < 3) {
        return 0.0;
    }

    // 使用 Shoelace 公式计算多边形面积（球面近似）
    // 地球半径（公里）
    const double EARTH_RADIUS_KM = 6371.0;

    double area = 0.0;
    int n = coords.size();

    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;

        double lat1 = coords[i].first * M_PI / 180.0;  // 转换为弧度
        double lon1 = coords[i].second * M_PI / 180.0;
        double lat2 = coords[j].first * M_PI / 180.0;
        double lon2 = coords[j].second * M_PI / 180.0;

        area += (lon2 - lon1) * (2.0 + std::sin(lat1) + std::sin(lat2));
    }

    area = std::abs(area) * EARTH_RADIUS_KM * EARTH_RADIUS_KM / 2.0;

    return area;
}
