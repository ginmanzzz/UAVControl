// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TaskDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QDebug>

TaskDialog::TaskDialog(TaskManager *taskManager, QWidget *parent)
    : QDialog(parent)
    , m_taskManager(taskManager)
    , m_editTask(nullptr)
    , m_isEditMode(false)
    , m_originalTaskId(-1)
    , m_taskId(-1)
{
    setupUI();
}

TaskDialog::TaskDialog(TaskManager *taskManager, Task *task, QWidget *parent)
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

void TaskDialog::setupUI()
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

    // 任务ID输入
    m_taskIdInput = new QLineEdit(this);
    m_taskIdInput->setPlaceholderText("输入任务ID（正整数）");
    m_taskIdInput->setValidator(new QIntValidator(1, 999999, this));
    connect(m_taskIdInput, &QLineEdit::textChanged, this, &TaskDialog::onTaskIdChanged);

    m_taskIdErrorLabel = new QLabel(this);
    m_taskIdErrorLabel->setStyleSheet("color: #f44336; font-size: 11px;");
    m_taskIdErrorLabel->setWordWrap(true);
    m_taskIdErrorLabel->hide();

    auto *taskIdContainer = new QWidget(this);
    auto *taskIdLayout = new QVBoxLayout(taskIdContainer);
    taskIdLayout->setContentsMargins(0, 0, 0, 0);
    taskIdLayout->setSpacing(4);
    taskIdLayout->addWidget(m_taskIdInput);
    taskIdLayout->addWidget(m_taskIdErrorLabel);

    formLayout->addRow("任务ID:", taskIdContainer);

    // 任务名称输入
    m_taskNameInput = new QLineEdit(this);
    m_taskNameInput->setPlaceholderText("输入任务名称");
    connect(m_taskNameInput, &QLineEdit::textChanged, this, &TaskDialog::onTaskNameChanged);

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
    connect(m_createButton, &QPushButton::clicked, this, &TaskDialog::onCreateClicked);

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

void TaskDialog::onTaskIdChanged(const QString &text)
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

void TaskDialog::onTaskNameChanged(const QString &text)
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

void TaskDialog::validateInputs()
{
    bool valid = isTaskIdValid() && isTaskNameValid();
    m_createButton->setEnabled(valid);
}

bool TaskDialog::isTaskIdValid() const
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

bool TaskDialog::isTaskNameValid() const
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

void TaskDialog::onCreateClicked()
{
    m_taskId = m_taskIdInput->text().toInt();
    m_taskName = m_taskNameInput->text().trimmed();
    m_taskDescription = m_descriptionInput->toPlainText().trimmed();

    qDebug() << QString("创建任务: ID=%1, 名称=%2").arg(m_taskId).arg(m_taskName);

    accept();
}

int TaskDialog::getTaskId() const
{
    return m_taskId;
}

QString TaskDialog::getTaskName() const
{
    return m_taskName;
}

QString TaskDialog::getTaskDescription() const
{
    return m_taskDescription;
}
