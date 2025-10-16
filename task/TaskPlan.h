#ifndef TASKPLAN_H
#define TASKPLAN_H

#include <QString>
#include <QVector>

/**
 * @brief 任务方案中的任务项
 */
struct TaskPlanTask {
    int taskNumber;              // 任务编号
    QString taskType;            // 任务类型（如：侦察、打击等）
    QString taskRegion;          // 任务区域
    QString targetType;          // 目标类型及特征
    QString reserveCapacity;     // 预留20%能力（字符串形式，可输入任意内容）
};

/**
 * @brief 任务方案类 - 由多个任务组成的作战方案
 */
class TaskPlan {
public:
    TaskPlan(int id, const QString &name)
        : m_id(id), m_name(name) {}

    int id() const { return m_id; }
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    // 任务管理
    void addTask(const TaskPlanTask &task) { m_tasks.append(task); }
    void removeTask(int index) {
        if (index >= 0 && index < m_tasks.size()) {
            m_tasks.removeAt(index);
        }
    }
    void updateTask(int index, const TaskPlanTask &task) {
        if (index >= 0 && index < m_tasks.size()) {
            m_tasks[index] = task;
        }
    }

    const QVector<TaskPlanTask>& tasks() const { return m_tasks; }
    int taskCount() const { return m_tasks.size(); }

private:
    int m_id;
    QString m_name;
    QVector<TaskPlanTask> m_tasks;
};

#endif // TASKPLAN_H
