// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include "TaskManager.h"
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

/**
 * @brief 任务对话框 - 用于创建和编辑任务
 */
class TaskDialog : public QDialog {
    Q_OBJECT

public:
    // 创建新任务的构造函数
    explicit TaskDialog(TaskManager *taskManager, QWidget *parent = nullptr);

    // 编辑现有任务的构造函数
    explicit TaskDialog(TaskManager *taskManager, Task *task, QWidget *parent = nullptr);

    // 获取用户输入的数据
    int getTaskId() const;
    QString getTaskName() const;
    QString getTaskDescription() const;

private slots:
    void onTaskIdChanged(const QString &text);
    void onTaskNameChanged(const QString &text);
    void validateInputs();
    void onCreateClicked();

private:
    void setupUI();
    bool isTaskIdValid() const;
    bool isTaskNameValid() const;

private:
    TaskManager *m_taskManager;
    Task *m_editTask;  // 如果是编辑模式，指向正在编辑的任务
    bool m_isEditMode;  // 是否为编辑模式
    int m_originalTaskId;  // 编辑模式下保存原始ID

    QLineEdit *m_taskIdInput;
    QLineEdit *m_taskNameInput;
    QTextEdit *m_descriptionInput;
    QLabel *m_taskIdErrorLabel;
    QLabel *m_taskNameErrorLabel;
    QPushButton *m_createButton;
    QPushButton *m_cancelButton;

    int m_taskId;
    QString m_taskName;
    QString m_taskDescription;
};

#endif // TASKDIALOG_H
