#ifndef PLAN_H
#define PLAN_H

#include <QString>
#include <QVector>

/**
 * @brief 方案中的任务项
 */
struct PlanTask {
    int taskNumber;              // 任务编号
    QString taskType;            // 任务类型（如：侦察、打击等）
    QString taskRegion;          // 任务区域
    QString targetType;          // 目标类型及特征
    bool reserveCapacity;        // 是否预留20%能力
};

/**
 * @brief 方案类 - 由多个任务组成的作战方案
 */
class Plan {
public:
    Plan(int id, const QString &name)
        : m_id(id), m_name(name) {}

    int id() const { return m_id; }
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    // 任务管理
    void addTask(const PlanTask &task) { m_tasks.append(task); }
    void removeTask(int index) {
        if (index >= 0 && index < m_tasks.size()) {
            m_tasks.removeAt(index);
        }
    }
    void updateTask(int index, const PlanTask &task) {
        if (index >= 0 && index < m_tasks.size()) {
            m_tasks[index] = task;
        }
    }

    const QVector<PlanTask>& tasks() const { return m_tasks; }
    int taskCount() const { return m_tasks.size(); }

private:
    int m_id;
    QString m_name;
    QVector<PlanTask> m_tasks;
};

#endif // PLAN_H
