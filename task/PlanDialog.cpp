#include "PlanDialog.h"
#include "CreateTaskDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QDebug>
#include <QPalette>
#include <QGraphicsDropShadowEffect>

PlanDialog::PlanDialog(TaskManager *taskManager, QWidget *parent)
    : QWidget(parent), m_taskManager(taskManager)
{
    setupUI();

    // 设置白色背景
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
}

void PlanDialog::setupUI()
{
    setFixedSize(600, 400);

    // 统一的样式表 - 所有元素完全不透明
    setStyleSheet(
        "PlanDialog {"
        "  background-color: white;"
        "  border: 2px solid #2196F3;"
        "  border-radius: 6px;"
        "}"
        "QLabel {"
        "  background-color: white;"
        "  color: #333;"
        "}"
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 4px 12px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
        "QTableWidget {"
        "  background-color: white;"
        "  border: 1px solid #E0E0E0;"
        "  gridline-color: #E0E0E0;"
        "}"
        "QTableWidget::item {"
        "  background-color: white;"
        "  color: #333;"
        "  padding: 4px;"
        "  font-size: 12px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #BBDEFB;"
        "}"
        "QHeaderView::section {"
        "  background-color: #E3F2FD;"
        "  color: #333;"
        "  padding: 5px;"
        "  border: none;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 12, 15, 12);
    mainLayout->setSpacing(0);

    // ============ 标题 + 关闭按钮 ============
    auto *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: white;");
    auto *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *titleLabel = new QLabel("创建方案", headerWidget);
    titleLabel->setStyleSheet(
        "font-size: 12px;"
        "font-weight: bold;"
        "color: #2196F3;"
        "background-color: white;"
    );

    auto *closeButton = new QPushButton("✕", headerWidget);
    closeButton->setFixedSize(20, 20);
    closeButton->setStyleSheet(
        "background-color: white;"
        "color: #333;"
        "font-size: 12px;"
        "border: none;"
    );
    connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(closeButton);
    mainLayout->addWidget(headerWidget);

    // ============ 新建任务按钮 ============
    auto *newTaskWidget = new QWidget(this);
    newTaskWidget->setStyleSheet("background-color: white;");
    auto *newTaskLayout = new QHBoxLayout(newTaskWidget);
    newTaskLayout->setContentsMargins(0, 0, 0, 0);

    m_newTaskButton = new QPushButton("新建任务", newTaskWidget);
    m_newTaskButton->setFixedHeight(28);
    m_newTaskButton->setStyleSheet(
        "background-color: white;"
        "color: black;"
        "border: 1px solid #CCCCCC;"
        "border-radius: 4px;"
        "padding: 4px 12px;"
        "font-size: 12px;"
    );
    // 添加阴影效果
    auto *newTaskShadow = new QGraphicsDropShadowEffect(m_newTaskButton);
    newTaskShadow->setBlurRadius(8);
    newTaskShadow->setColor(QColor(0, 0, 0, 60));
    newTaskShadow->setOffset(2, 2);
    m_newTaskButton->setGraphicsEffect(newTaskShadow);
    connect(m_newTaskButton, &QPushButton::clicked, this, &PlanDialog::onNewTask);

    newTaskLayout->addWidget(m_newTaskButton);
    newTaskLayout->addStretch();
    mainLayout->addWidget(newTaskWidget);

    // ============ 任务表格 ============
    m_taskTable = new QTableWidget(0, 8, this);
    m_taskTable->setHorizontalHeaderLabels({"任务ID", "任务名称", "任务种类", "任务区域", "目标类型", "特征", "预留20%", "操作"});
    m_taskTable->verticalHeader()->setVisible(false);
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 不可编辑

    // 设置列宽
    m_taskTable->setColumnWidth(0, 60);   // 任务ID
    m_taskTable->setColumnWidth(1, 100);  // 任务名称
    m_taskTable->setColumnWidth(2, 80);   // 任务种类
    m_taskTable->setColumnWidth(3, 80);   // 任务区域
    m_taskTable->setColumnWidth(4, 70);   // 目标类型
    m_taskTable->setColumnWidth(5, 50);   // 特征
    m_taskTable->setColumnWidth(6, 70);   // 预留20%
    m_taskTable->setColumnWidth(7, 60);   // 操作

    mainLayout->addWidget(m_taskTable, 1);

    // ============ 底部按钮 ============
    auto *buttonWidget = new QWidget(this);
    buttonWidget->setStyleSheet("background-color: white;");
    auto *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch();

    m_confirmButton = new QPushButton("确定", buttonWidget);
    m_confirmButton->setFixedSize(80, 28);
    m_confirmButton->setStyleSheet(
        "background-color: white;"
        "color: black;"
        "border: 1px solid #CCCCCC;"
        "border-radius: 4px;"
        "padding: 4px 12px;"
        "font-size: 12px;"
    );
    // 添加阴影效果
    auto *confirmShadow = new QGraphicsDropShadowEffect(m_confirmButton);
    confirmShadow->setBlurRadius(8);
    confirmShadow->setColor(QColor(0, 0, 0, 60));
    confirmShadow->setOffset(2, 2);
    m_confirmButton->setGraphicsEffect(confirmShadow);
    connect(m_confirmButton, &QPushButton::clicked, this, &PlanDialog::onConfirm);

    m_cancelButton = new QPushButton("取消", buttonWidget);
    m_cancelButton->setFixedSize(80, 28);
    m_cancelButton->setStyleSheet(
        "background-color: white;"
        "color: black;"
        "border: 1px solid #CCCCCC;"
        "border-radius: 4px;"
        "padding: 4px 12px;"
        "font-size: 12px;"
    );
    // 添加阴影效果
    auto *cancelShadow = new QGraphicsDropShadowEffect(m_cancelButton);
    cancelShadow->setBlurRadius(8);
    cancelShadow->setColor(QColor(0, 0, 0, 60));
    cancelShadow->setOffset(2, 2);
    m_cancelButton->setGraphicsEffect(cancelShadow);
    connect(m_cancelButton, &QPushButton::clicked, this, &PlanDialog::onCancel);

    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addSpacing(8);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addWidget(buttonWidget);
}

void PlanDialog::setPlan(Plan *plan)
{
    m_plan = plan;
    loadPlanData();
}

void PlanDialog::loadPlanData()
{
    m_taskTable->setRowCount(0);

    if (!m_plan) {
        return;
    }

    const QVector<PlanTask> &tasks = m_plan->tasks();
    for (const PlanTask &task : tasks) {
        int row = m_taskTable->rowCount();
        m_taskTable->insertRow(row);

        // 任务编号
        m_taskTable->setItem(row, 0, new QTableWidgetItem(QString::number(task.taskNumber)));

        // 任务类型
        m_taskTable->setItem(row, 1, new QTableWidgetItem(task.taskType));

        // 任务区域
        m_taskTable->setItem(row, 2, new QTableWidgetItem(task.taskRegion));

        // 目标类型及特征
        m_taskTable->setItem(row, 3, new QTableWidgetItem(task.targetType));

        // 预留20%能力（可编辑文本）
        m_taskTable->setItem(row, 4, new QTableWidgetItem(task.reserveCapacity));

        // 操作按钮（删除）
        auto *deleteButton = new QPushButton("删除");
        deleteButton->setStyleSheet(
            "QPushButton {"
            "  background-color: rgba(244, 67, 54, 180);"
            "  color: white;"
            "  border: none;"
            "  border-radius: 3px;"
            "  padding: 3px 6px;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(244, 67, 54, 220);"
            "}"
        );
        connect(deleteButton, &QPushButton::clicked, this, [this, row]() {
            m_taskTable->removeRow(row);
        });
        m_taskTable->setCellWidget(row, 5, deleteButton);
    }
}

void PlanDialog::savePlanData()
{
    if (!m_plan) {
        return;
    }

    // 清空现有任务
    while (m_plan->taskCount() > 0) {
        m_plan->removeTask(0);
    }

    // 从表格读取任务
    for (int row = 0; row < m_taskTable->rowCount(); ++row) {
        PlanTask task;

        // 任务编号
        QTableWidgetItem *numberItem = m_taskTable->item(row, 0);
        task.taskNumber = numberItem ? numberItem->text().toInt() : 0;

        // 任务类型
        QTableWidgetItem *typeItem = m_taskTable->item(row, 1);
        task.taskType = typeItem ? typeItem->text() : "";

        // 任务区域
        QTableWidgetItem *regionItem = m_taskTable->item(row, 2);
        task.taskRegion = regionItem ? regionItem->text() : "";

        // 目标类型及特征
        QTableWidgetItem *targetItem = m_taskTable->item(row, 3);
        task.targetType = targetItem ? targetItem->text() : "";

        // 预留20%能力
        QTableWidgetItem *capacityItem = m_taskTable->item(row, 4);
        task.reserveCapacity = capacityItem ? capacityItem->text() : "";

        m_plan->addTask(task);
    }
}

void PlanDialog::onNewTask()
{
    qDebug() << "打开任务创建对话框";
    openTaskDialog();
}

void PlanDialog::onDeleteTask()
{
    int currentRow = m_taskTable->currentRow();
    if (currentRow >= 0) {
        m_taskTable->removeRow(currentRow);
        qDebug() << "删除任务，行：" << currentRow;
    }
}

void PlanDialog::onConfirm()
{
    // 确认：保留所有暂存的任务
    qDebug() << "确认方案，保留" << m_tempTaskIds.size() << "个任务";

    // 清空暂存列表（任务已经在 TaskManager 中）
    m_tempTaskIds.clear();

    // 清空表格
    m_taskTable->setRowCount(0);

    savePlanData();
    if (m_plan) {
        emit planUpdated(m_plan);
    }

    hide();
}

void PlanDialog::onCancel()
{
    qDebug() << "取消方案编辑，删除" << m_tempTaskIds.size() << "个暂存任务";

    // 取消：删除所有暂存的任务
    for (int taskId : m_tempTaskIds) {
        m_taskManager->removeTask(taskId);
        qDebug() << "删除暂存任务 ID=" << taskId;
    }

    // 清空暂存列表
    m_tempTaskIds.clear();

    // 清空表格
    m_taskTable->setRowCount(0);

    hide();
}

void PlanDialog::openTaskDialog()
{
    qDebug() << "openTaskDialog 被调用";

    if (!m_taskManager) {
        qDebug() << "TaskManager 未设置";
        return;
    }

    qDebug() << "TaskManager 已设置，准备创建/显示 CreateTaskDialog";

    // 每次都创建新的对话框
    CreateTaskDialog *dialog = new CreateTaskDialog(m_taskManager, parentWidget());
    dialog->setFixedSize(width(), height());

    // 计算全局坐标位置
    QPoint globalPos = mapToGlobal(QPoint(0, 0));
    int x = globalPos.x() + this->width() + 10;
    int y = globalPos.y();
    dialog->move(x, y);

    // 显示对话框并等待用户操作
    if (dialog->exec() == QDialog::Accepted) {
        // 用户点击了创建，将任务添加到表格
        int taskId = dialog->getTaskId();
        QString taskName = dialog->getTaskName();
        QString taskType = dialog->getTaskType();
        QString taskRegion = dialog->getTaskRegion();
        QString targetType = dialog->getTargetType();
        QString targetFeature = dialog->getTargetFeature();
        bool reserveCapacity = dialog->getReserveCapacity();

        // 创建任务并暂存到 TaskManager
        Task *task = m_taskManager->createTask(taskId, taskName, dialog->getTaskDescription());
        task->setTaskType(taskType);
        task->setTaskRegion(taskRegion);
        task->setTargetType(targetType);
        task->setTargetFeature(targetFeature);
        task->setReserveCapacity(reserveCapacity);

        // 添加到表格
        addTaskToTable(task);

        qDebug() << "任务已添加到表格: ID=" << taskId << ", 名称=" << taskName;
    }

    dialog->deleteLater();
}

void PlanDialog::addTaskToTable(Task *task)
{
    if (!task) return;

    int row = m_taskTable->rowCount();
    m_taskTable->insertRow(row);

    // 任务ID
    m_taskTable->setItem(row, 0, new QTableWidgetItem(QString::number(task->id())));

    // 任务名称
    m_taskTable->setItem(row, 1, new QTableWidgetItem(task->name()));

    // 任务种类
    m_taskTable->setItem(row, 2, new QTableWidgetItem(task->taskType()));

    // 任务区域
    m_taskTable->setItem(row, 3, new QTableWidgetItem(task->taskRegion()));

    // 目标类型
    m_taskTable->setItem(row, 4, new QTableWidgetItem(task->targetType()));

    // 特征
    m_taskTable->setItem(row, 5, new QTableWidgetItem(task->targetFeature()));

    // 预留20%能力 - 显示勾或叉符号
    m_taskTable->setItem(row, 6, new QTableWidgetItem(task->reserveCapacity() ? "✓" : "✗"));

    // 删除按钮
    auto *deleteButton = new QPushButton("删除");
    deleteButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(244, 67, 54, 180);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 3px 6px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(244, 67, 54, 220);"
        "}"
    );
    connect(deleteButton, &QPushButton::clicked, this, [this, task]() {
        // 从表格中删除
        for (int i = 0; i < m_taskTable->rowCount(); ++i) {
            if (m_taskTable->item(i, 0)->text().toInt() == task->id()) {
                m_taskTable->removeRow(i);
                break;
            }
        }
        // 从暂存列表中删除
        m_tempTaskIds.removeOne(task->id());
        // 从 TaskManager 中删除
        m_taskManager->removeTask(task->id());
    });
    m_taskTable->setCellWidget(row, 7, deleteButton);

    // 添加到暂存列表
    m_tempTaskIds.append(task->id());

    qDebug() << "任务添加到表格，行=" << row << "，ID=" << task->id();
}
