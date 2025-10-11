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
    // 不使用WA_TranslucentBackground，否则背景色会完全透明
}

void PlanDialog::setupUI()
{
    setFixedSize(600, 400);

    // 透明蓝紫色背景，简洁设计
    setStyleSheet(
        "QWidget#PlanDialog {"
        "  background-color: rgba(103, 58, 183, 180);"  // 蓝紫色，半透明
        "  border-radius: 6px;"
        "}"
    );
    setObjectName("PlanDialog");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 12, 15, 12);
    mainLayout->setSpacing(10);

    // ============ 标题 + 关闭按钮 ============
    auto *headerLayout = new QHBoxLayout();

    auto *titleLabel = new QLabel("创建方案", this);
    titleLabel->setStyleSheet(
        "font-size: 15px;"
        "font-weight: bold;"
        "color: white;"
        "background: transparent;"
    );

    auto *closeButton = new QPushButton("✕", this);
    closeButton->setFixedSize(20, 20);
    closeButton->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: white;"
        "  font-size: 16px;"
        "}"
        "QPushButton:hover { color: #ffcccc; }"
    );
    connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(closeButton);
    mainLayout->addLayout(headerLayout);

    // ============ 新建任务按钮 ============
    m_newTaskButton = new QPushButton("新建任务", this);
    m_newTaskButton->setFixedHeight(28);
    m_newTaskButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 200);"
        "  color: #673AB7;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 4px 12px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 230);"
        "}"
    );
    connect(m_newTaskButton, &QPushButton::clicked, this, &PlanDialog::onNewTask);
    mainLayout->addWidget(m_newTaskButton, 0, Qt::AlignLeft);

    // ============ 任务表格 ============
    m_taskTable = new QTableWidget(0, 6, this);
    m_taskTable->setHorizontalHeaderLabels({"任务编号", "任务类型", "任务区域", "目标类型及特征", "预留20%能力", "操作"});
    m_taskTable->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: rgba(255, 255, 255, 150);"
        "  color: #333;"
        "  padding: 5px;"
        "  border: none;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
    );
    m_taskTable->verticalHeader()->setVisible(false);
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_taskTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: rgba(255, 255, 255, 200);"
        "  border: none;"
        "  gridline-color: rgba(0, 0, 0, 50);"
        "}"
        "QTableWidget::item {"
        "  padding: 4px;"
        "  color: #333;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: rgba(103, 58, 183, 100);"
        "}"
    );

    // 设置列宽（总宽度约570px，适应600px窗口）
    m_taskTable->setColumnWidth(0, 60);   // 任务编号
    m_taskTable->setColumnWidth(1, 90);   // 任务类型
    m_taskTable->setColumnWidth(2, 100);  // 任务区域
    m_taskTable->setColumnWidth(3, 180);  // 目标类型及特征
    m_taskTable->setColumnWidth(4, 80);   // 预留20%能力
    m_taskTable->setColumnWidth(5, 60);   // 操作

    mainLayout->addWidget(m_taskTable, 1);

    // ============ 底部按钮 ============
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_confirmButton = new QPushButton("确定", this);
    m_confirmButton->setFixedSize(80, 28);
    m_confirmButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 200);"
        "  color: #673AB7;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 230);"
        "}"
    );
    connect(m_confirmButton, &QPushButton::clicked, this, &PlanDialog::onConfirm);

    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setFixedSize(80, 28);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 120);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 150);"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &PlanDialog::onCancel);

    buttonLayout->addWidget(m_confirmButton);
    buttonLayout->addSpacing(8);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
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
        checkBoxWidget->setStyleSheet("background: transparent;");
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
            "  background-color: rgba(244, 67, 54, 180);"
            "  color: white;"
            "  border: none;"
            "  border-radius: 3px;"
            "  padding: 3px 6px;"
            "  font-size: 10px;"
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
    checkBoxWidget->setStyleSheet("background: transparent;");
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
        "  background-color: rgba(244, 67, 54, 180);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 3px 6px;"
        "  font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(244, 67, 54, 220);"
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
