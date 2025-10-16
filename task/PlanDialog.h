#ifndef PLANDIALOG_H
#define PLANDIALOG_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "Plan.h"
#include "TaskManager.h"

/**
 * @brief 方案规划对话框
 * 显示任务表格，允许创建和编辑方案
 */
class PlanDialog : public QWidget {
    Q_OBJECT

public:
    explicit PlanDialog(TaskManager *taskManager, QWidget *parent = nullptr);
    ~PlanDialog() = default;

    void setPlan(Plan *plan);
    Plan* plan() const { return m_plan; }

signals:
    void planCreated(Plan *plan);
    void planUpdated(Plan *plan);

private slots:
    void onNewTask();
    void onDeleteTask();
    void onConfirm();
    void onCancel();

private:
    void setupUI();
    void loadPlanData();
    void savePlanData();
    void openTaskDialog();
    void addTaskToTable(Task *task);

    QList<int> m_tempTaskIds;  // 暂存的任务ID列表

    TaskManager *m_taskManager = nullptr;
    Plan *m_plan = nullptr;
    QTableWidget *m_taskTable;
    QPushButton *m_newTaskButton;
    QPushButton *m_confirmButton;
    QPushButton *m_cancelButton;
    class CreateTaskDialog *m_taskDialog = nullptr;
};

#endif // PLANDIALOG_H
