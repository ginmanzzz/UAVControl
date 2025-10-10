// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef LAUNCHUI_H
#define LAUNCHUI_H

#include <QWidget>

// 发射管理 UI Widget
class LaunchUI : public QWidget {
    Q_OBJECT

public:
    explicit LaunchUI(QWidget *parent = nullptr);

signals:
    void initialized();  // 当初始化完成时发出

private:
    void setupUI();
};

#endif // LAUNCHUI_H
