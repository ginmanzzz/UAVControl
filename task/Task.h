// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QSet>

/**
 * @brief 任务类 - 引用地图标记区域（不拥有）
 *
 * Task 只存储 Region 的ID引用，不直接持有区域对象
 * 使用 QSet 哈希表存储，提供 O(1) 的查找效率
 */
class Task {
public:
    Task() : m_id(0), m_visible(true) {}

    Task(int id, const QString &name, const QString &description = QString())
        : m_id(id)
        , m_name(name)
        , m_description(description)
        , m_visible(true)
    {}

    // ==================== 基本属性 ====================

    int id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    bool isVisible() const { return m_visible; }

    void setId(int id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setDescription(const QString &description) { m_description = description; }
    void setVisible(bool visible) { m_visible = visible; }

    // ==================== 区域关联（新架构）====================

    /**
     * @brief 获取关联的区域ID集合
     */
    const QSet<int>& regionIds() const { return m_regionIds; }

    /**
     * @brief 添加区域到任务
     * @param regionId 区域ID
     */
    void addRegion(int regionId) {
        m_regionIds.insert(regionId);
    }

    /**
     * @brief 从任务移除区域
     * @param regionId 区域ID
     * @return 成功返回 true
     */
    bool removeRegion(int regionId) {
        return m_regionIds.remove(regionId) > 0;
    }

    /**
     * @brief 检查任务是否包含某个区域
     * @param regionId 区域ID
     * @return 包含返回 true
     */
    bool hasRegion(int regionId) const {
        return m_regionIds.contains(regionId);
    }

    /**
     * @brief 清除所有区域关联
     */
    void clearRegions() {
        m_regionIds.clear();
    }

    /**
     * @brief 获取关联的区域数量
     */
    int regionCount() const {
        return m_regionIds.size();
    }

private:
    // 基本属性
    int m_id;
    QString m_name;
    QString m_description;
    bool m_visible;

    // 区域ID集合（哈希表，O(1)查找）
    QSet<int> m_regionIds;
};

#endif // TASK_H
