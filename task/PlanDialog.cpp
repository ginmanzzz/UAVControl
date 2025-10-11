#include "PlanDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QDebug>

PlanDialog::PlanDialog(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    // 无边框、无标题栏的浮动窗口
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void PlanDialog::setupUI()
{
    // 缩小尺寸：宽度600px（约为地图一半），高度400px
    setFixedSize(600, 400);
    setStyleSheet(
        "QWidget {"
        "  background-color: #fafafa;"
        "  border-radius: 8px;"
        "  border: 1px solid #ccc;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ============ 标题栏 ============
    auto *headerWidget = new QWidget(this);
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

    auto *titleLabel = new QLabel("创建方案", headerWidget);
    titleLabel->setStyleSheet(
        "font-size: 15px;"
        "font-weight: bold;"
        "color: white;"
        "background: transparent;"
    );

    auto *closeButton = new QPushButton("✕", headerWidget);
    closeButton->setFixedSize(32, 32);
    closeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0);"
        "  border: none;"
        "  color: white;"
        "  font-size: 18px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 30);"
        "}"
    );
    connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(closeButton);
    mainLayout->addWidget(headerWidget);

    // ============ 内容区域 ============
    auto *contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background-color: #fafafa;");
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(15, 15, 15, 15);
    contentLayout->setSpacing(10);

    // 【新建任务】按钮
    m_newTaskButton = new QPushButton("新建任务", contentWidget);
    m_newTaskButton->setFixedHeight(32);
    m_newTaskButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );
    connect(m_newTaskButton, &QPushButton::clicked, this, &PlanDialog::onNewTask);
    contentLayout->addWidget(m_newTaskButton, 0, Qt::AlignLeft);

    // 任务表格
    m_taskTable = new QTableWidget(0, 6, contentWidget);
    m_taskTable->setHorizontalHeaderLabels({"任务编号", "任务类型", "任务区域", "目标类型及特征", "预留20%能力", "操作"});
    m_taskTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #e0e0e0;"
        "  padding: 6px;"
        "  border: 1px solid #ccc;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
    );
    m_taskTable->verticalHeader()->setVisible(false);
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_taskTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: white;"
        "  border: 1px solid #ddd;"
        "  gridline-color: #ddd;"
        "}"
        "QTableWidget::item {"
        "  padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #e3f2fd;"
        "}"
    );

    // 设置列宽（总宽度约570px，适应600px窗口）
    m_taskTable->setColumnWidth(0, 60);   // 任务编号
    m_taskTable->setColumnWidth(1, 90);   // 任务类型
    m_taskTable->setColumnWidth(2, 100);  // 任务区域
    m_taskTable->setColumnWidth(3, 180);  // 目标类型及特征
    m_taskTable->setColumnWidth(4, 80);   // 预留20%能力
    m_taskTable->setColumnWidth(5, 60);   // 操作

    contentLayout->addWidget(m_taskTable, 1);

    // ============ 底部按钮 ============
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();

    m_confirmButton = new QPushButton("确定", contentWidget);
    m_confirmButton->setFixedSize(100, 32);
    m_confirmButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
    );
    connect(m_confirmButton, &QPushButton::clicked, this, &PlanDialog::onConfirm);

    m_cancelButton = new QPushButton("取消", contentWidget);
    m_cancelButton->setFixedSize(100, 32);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #f44336;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #da190b;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &PlanDialog::onCancel);

    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addWidget(m_cancelButton);

    contentLayout->addLayout(buttonLayout);
    mainLayout->addWidget(contentWidget, 1);
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

        // 预留20%能力（复选框）
        auto *checkBoxWidget = new QWidget();
        auto *checkBoxLayout = new QHBoxLayout(checkBoxWidget);
        checkBoxLayout->setContentsMargins(0, 0, 0, 0);
        checkBoxLayout->setAlignment(Qt::AlignCenter);
        auto *checkBox = new QCheckBox();
        checkBox->setChecked(task.reserveCapacity);
        checkBoxLayout->addWidget(checkBox);
        m_taskTable->setCellWidget(row, 4, checkBoxWidget);

        // 操作按钮（删除）
        auto *deleteButton = new QPushButton("删除");
        deleteButton->setStyleSheet(
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
        QWidget *checkBoxWidget = m_taskTable->cellWidget(row, 4);
        if (checkBoxWidget) {
            QCheckBox *checkBox = checkBoxWidget->findChild<QCheckBox*>();
            task.reserveCapacity = checkBox ? checkBox->isChecked() : false;
        } else {
            task.reserveCapacity = false;
        }

        m_plan->addTask(task);
    }
}

void PlanDialog::onNewTask()
{
    int row = m_taskTable->rowCount();
    m_taskTable->insertRow(row);

    // 默认任务编号
    m_taskTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));

    // 默认空值
    m_taskTable->setItem(row, 1, new QTableWidgetItem(""));
    m_taskTable->setItem(row, 2, new QTableWidgetItem(""));
    m_taskTable->setItem(row, 3, new QTableWidgetItem(""));

    // 预留20%能力（复选框）
    auto *checkBoxWidget = new QWidget();
    auto *checkBoxLayout = new QHBoxLayout(checkBoxWidget);
    checkBoxLayout->setContentsMargins(0, 0, 0, 0);
    checkBoxLayout->setAlignment(Qt::AlignCenter);
    auto *checkBox = new QCheckBox();
    checkBox->setChecked(false);
    checkBoxLayout->addWidget(checkBox);
    m_taskTable->setCellWidget(row, 4, checkBoxWidget);

    // 删除按钮
    auto *deleteButton = new QPushButton("删除");
    deleteButton->setStyleSheet(
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
    connect(deleteButton, &QPushButton::clicked, this, [this, row]() {
        m_taskTable->removeRow(row);
    });
    m_taskTable->setCellWidget(row, 5, deleteButton);

    qDebug() << "新建任务，当前行数：" << m_taskTable->rowCount();
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
    savePlanData();
    qDebug() << "确认方案，任务数：" << (m_plan ? m_plan->taskCount() : 0);

    if (m_plan) {
        emit planUpdated(m_plan);
    }

    hide();
}

void PlanDialog::onCancel()
{
    qDebug() << "取消方案编辑";
    hide();
}
