#ifndef CREATETASKPLANDIALOG_H
#define CREATETASKPLANDIALOG_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "TaskPlan.h"
#include "TaskManager.h"

/**
 * @brief 任务方案创建对话框
 * 显示任务表格，允许创建和编辑任务方案
 */
class CreateTaskPlanDialog : public QWidget {
    Q_OBJECT

public:
    explicit CreateTaskPlanDialog(TaskManager *taskManager, QWidget *parent = nullptr);
    ~CreateTaskPlanDialog() = default;

    void setTaskPlan(TaskPlan *taskPlan);
    TaskPlan* taskPlan() const { return m_taskPlan; }

signals:
    void taskPlanCreated(TaskPlan *taskPlan);
    void taskPlanUpdated(TaskPlan *taskPlan);

private slots:
    void onNewTask();
    void onDeleteTask();
    void onConfirm();
    void onCancel();

private:
    void setupUI();
    void loadTaskPlanData();
    void saveTaskPlanData();
    void openTaskDialog();
    void addTaskToTable(Task *task);

    QList<int> m_tempTaskIds;  // 暂存的任务ID列表

    TaskManager *m_taskManager = nullptr;
    TaskPlan *m_taskPlan = nullptr;
    QTableWidget *m_taskTable;
    QPushButton *m_newTaskButton;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;
    class CreateTaskDialog *m_taskDialog = nullptr;
};

#endif // CREATETASKPLANDIALOG_H
