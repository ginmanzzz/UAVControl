// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "CreateTaskDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDebug>

CreateTaskDialog::CreateTaskDialog(TaskManager *taskManager, QWidget *parent)
    : QDialog(parent)
    , m_taskManager(taskManager)
    , m_editTask(nullptr)
    , m_isEditMode(false)
    , m_originalTaskId(-1)
    , m_taskId(-1)
{
    setupUI();
}

CreateTaskDialog::CreateTaskDialog(TaskManager *taskManager, Task *task, QWidget *parent)
    : QDialog(parent)
    , m_taskManager(taskManager)
    , m_editTask(task)
    , m_isEditMode(true)
    , m_originalTaskId(task->id())
    , m_taskId(task->id())
{
    setupUI();

    // 填充现有数据
    m_taskIdInput->setText(QString::number(task->id()));
    m_taskIdInput->setEnabled(false);  // 编辑模式下ID不可修改

    m_taskNameInput->setText(task->name());
    m_descriptionInput->setPlainText(task->description());

    // 设置窗口标题为编辑模式
    setWindowTitle("编辑任务");
    m_createButton->setText("保存");
}

void CreateTaskDialog::setupUI()
{
    setWindowTitle("创建新任务");
    setModal(true);
    setMinimumWidth(400);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 表单布局
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // 任务ID显示（系统自动分配，只读）
    m_taskIdInput = new QLineEdit(this);
    m_taskIdInput->setReadOnly(true);
    m_taskIdInput->setStyleSheet("background-color: #f5f5f5; color: #666;");

    // 自动分配任务ID（使用TaskManager生成）
    if (!m_isEditMode && m_taskManager) {
        m_taskId = m_taskManager->generateNextTaskId();
        m_taskIdInput->setText(QString::number(m_taskId));
    }

    formLayout->addRow("任务ID:", m_taskIdInput);

    // 任务名称输入
    m_taskNameInput = new QLineEdit(this);
    m_taskNameInput->setPlaceholderText("输入任务名称");

    // 为新任务设置默认名称
    if (!m_isEditMode && m_taskId != -1) {
        QString defaultName = QString("任务%1").arg(m_taskId);
        m_taskNameInput->setText(defaultName);
        m_taskNameInput->selectAll();  // 自动选中文本，方便用户直接输入
    }

    connect(m_taskNameInput, &QLineEdit::textChanged, this, &CreateTaskDialog::onTaskNameChanged);

    m_taskNameErrorLabel = new QLabel(this);
    m_taskNameErrorLabel->setStyleSheet("color: #f44336; font-size: 11px;");
    m_taskNameErrorLabel->setWordWrap(true);
    m_taskNameErrorLabel->hide();

    auto *taskNameContainer = new QWidget(this);
    auto *taskNameLayout = new QVBoxLayout(taskNameContainer);
    taskNameLayout->setContentsMargins(0, 0, 0, 0);
    taskNameLayout->setSpacing(4);
    taskNameLayout->addWidget(m_taskNameInput);
    taskNameLayout->addWidget(m_taskNameErrorLabel);

    formLayout->addRow("任务名称:", taskNameContainer);

    // 任务描述输入
    m_descriptionInput = new QTextEdit(this);
    m_descriptionInput->setPlaceholderText("输入任务的详细描述...");
    m_descriptionInput->setMinimumHeight(100);
    m_descriptionInput->setMaximumHeight(150);

    formLayout->addRow("详细描述:", m_descriptionInput);

    // 任务种类下拉框
    m_taskTypeCombo = new QComboBox(this);
    m_taskTypeCombo->addItems({"区域搜索", "区域掩护", "电子侦察", "协同攻击", "目标侦察"});
    formLayout->addRow("任务种类:", m_taskTypeCombo);

    // 任务区域下拉框
    m_taskRegionCombo = new QComboBox(this);
    m_taskRegionCombo->addItem("（请选择任务区域）", -1);  // 默认选项，值为-1

    // 从 RegionManager 获取区域列表
    if (m_taskManager && m_taskManager->regionManager()) {
        auto regions = m_taskManager->regionManager()->getAllRegions();
        for (Region *region : regions) {
            // 显示：区域ID - 区域名称
            QString displayText = QString("区域%1 - %2").arg(region->id()).arg(region->name());
            m_taskRegionCombo->addItem(displayText, region->id());
        }
    }

    // 当任务区域改变时重新验证
    connect(m_taskRegionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CreateTaskDialog::validateInputs);

    formLayout->addRow("任务区域:", m_taskRegionCombo);

    // 目标类型下拉框
    m_targetTypeCombo = new QComboBox(this);
    m_targetTypeCombo->addItems({"车辆", "雷达", "区域"});
    formLayout->addRow("目标类型:", m_targetTypeCombo);

    // 特征下拉框
    m_targetFeatureCombo = new QComboBox(this);
    m_targetFeatureCombo->addItems({"大", "中", "小"});
    formLayout->addRow("特征:", m_targetFeatureCombo);

    // 预留20%能力复选框
    m_reserveCapacityCheck = new QCheckBox("预留20%能力", this);
    formLayout->addRow("", m_reserveCapacityCheck);

    mainLayout->addLayout(formLayout);

    // 按钮布局
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #9e9e9e;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #757575;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_createButton = new QPushButton("创建", this);
    m_createButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b7dda;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    );
    m_createButton->setEnabled(false);
    connect(m_createButton, &QPushButton::clicked, this, &CreateTaskDialog::onCreateClicked);

    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_createButton);

    mainLayout->addLayout(buttonLayout);

    // 设置对话框样式
    setStyleSheet(
        "QLineEdit, QTextEdit {"
        "  border: 1px solid #ccc;"
        "  border-radius: 4px;"
        "  padding: 6px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus {"
        "  border-color: #2196F3;"
        "}"
    );
}

void CreateTaskDialog::onTaskIdChanged(const QString &text)
{
    // 编辑模式下，ID不可编辑，直接跳过验证
    if (m_isEditMode) {
        m_taskIdErrorLabel->hide();
        validateInputs();
        return;
    }

    if (text.isEmpty()) {
        m_taskIdErrorLabel->hide();
        validateInputs();
        return;
    }

    bool ok;
    int id = text.toInt(&ok);

    if (!ok || id <= 0) {
        m_taskIdErrorLabel->setText("任务ID必须是正整数");
        m_taskIdErrorLabel->show();
        validateInputs();
        return;
    }

    // 检查ID是否已存在
    if (m_taskManager->getTask(id) != nullptr) {
        m_taskIdErrorLabel->setText(QString("任务ID %1 已存在，请使用其他ID").arg(id));
        m_taskIdErrorLabel->show();
    } else {
        m_taskIdErrorLabel->hide();
    }

    validateInputs();
}

void CreateTaskDialog::onTaskNameChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        m_taskNameErrorLabel->hide();
        validateInputs();
        return;
    }

    // 检查名称是否已存在
    QString trimmedName = text.trimmed();
    for (const Task *task : m_taskManager->getAllTasks()) {
        // 编辑模式下，允许保留原有名称
        if (m_isEditMode && task->id() == m_originalTaskId) {
            continue;
        }

        if (task->name() == trimmedName) {
            m_taskNameErrorLabel->setText(QString("任务名称 \"%1\" 已存在，请使用其他名称").arg(trimmedName));
            m_taskNameErrorLabel->show();
            validateInputs();
            return;
        }
    }

    m_taskNameErrorLabel->hide();
    validateInputs();
}

void CreateTaskDialog::validateInputs()
{
    // 任务ID由系统自动分配，不需要验证
    // 需要验证：任务名称 和 任务区域
    bool valid = isTaskNameValid() && isTaskRegionSelected();
    m_createButton->setEnabled(valid);
}

bool CreateTaskDialog::isTaskIdValid() const
{
    // 编辑模式下，ID已经存在且不可修改，总是有效
    if (m_isEditMode) {
        return true;
    }

    QString text = m_taskIdInput->text().trimmed();
    if (text.isEmpty()) {
        return false;
    }

    bool ok;
    int id = text.toInt(&ok);
    if (!ok || id <= 0) {
        return false;
    }

    // 检查ID是否已存在
    return m_taskManager->getTask(id) == nullptr;
}

bool CreateTaskDialog::isTaskNameValid() const
{
    QString name = m_taskNameInput->text().trimmed();
    if (name.isEmpty()) {
        return false;
    }

    // 检查名称是否已存在
    for (const Task *task : m_taskManager->getAllTasks()) {
        // 编辑模式下，允许保留原有名称
        if (m_isEditMode && task->id() == m_originalTaskId) {
            continue;
        }

        if (task->name() == name) {
            return false;
        }
    }

    return true;
}

void CreateTaskDialog::onCreateClicked()
{
    // 再次检查是否选择了任务区域（防御性编程）
    if (!isTaskRegionSelected()) {
        QMessageBox::warning(this, "未选择任务区域",
                           "请先选择一个任务区域后再创建任务。");
        return;
    }

    m_taskId = m_taskIdInput->text().toInt();
    m_taskName = m_taskNameInput->text().trimmed();
    m_taskDescription = m_descriptionInput->toPlainText().trimmed();

    qDebug() << QString("创建任务: ID=%1, 名称=%2").arg(m_taskId).arg(m_taskName);

    accept();
}

int CreateTaskDialog::getTaskId() const
{
    return m_taskId;
}

QString CreateTaskDialog::getTaskName() const
{
    return m_taskName;
}

QString CreateTaskDialog::getTaskDescription() const
{
    return m_taskDescription;
}

QString CreateTaskDialog::getTaskType() const
{
    return m_taskTypeCombo ? m_taskTypeCombo->currentText() : QString();
}

QString CreateTaskDialog::getTaskRegion() const
{
    return m_taskRegionCombo ? m_taskRegionCombo->currentText() : QString();
}

QString CreateTaskDialog::getTargetType() const
{
    return m_targetTypeCombo ? m_targetTypeCombo->currentText() : QString();
}

QString CreateTaskDialog::getTargetFeature() const
{
    return m_targetFeatureCombo ? m_targetFeatureCombo->currentText() : QString();
}

bool CreateTaskDialog::getReserveCapacity() const
{
    return m_reserveCapacityCheck ? m_reserveCapacityCheck->isChecked() : false;
}

bool CreateTaskDialog::isTaskRegionSelected() const
{
    if (!m_taskRegionCombo) {
        return false;
    }

    // 检查是否选择了有效的区域（不是"（请选择任务区域）"）
    int regionId = m_taskRegionCombo->currentData().toInt();
    return regionId != -1;
}
