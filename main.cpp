// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "task/TaskUI.h"
#include "launch/LaunchUI.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QDebug>

class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow() {
        setupUI();
    }

private slots:
    void onTabChanged(int index) {
        m_stackedWidget->setCurrentIndex(index);
        qDebug() << QString("切换到页面: %1").arg(index == 0 ? "任务管理" : "发射管理");
    }

private:
    void setupUI() {
        // 创建主布局
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 创建顶部标签页切换栏
        auto *tabBar = new QWidget(this);
        tabBar->setStyleSheet(
            "QWidget {"
            "  background-color: #2196F3;"
            "  border-bottom: 2px solid #1976D2;"
            "}"
        );
        auto *tabLayout = new QHBoxLayout(tabBar);
        tabLayout->setContentsMargins(0, 0, 0, 0);
        tabLayout->setSpacing(0);

        // 标签页按钮样式
        QString tabButtonStyle =
            "QPushButton {"
            "  background-color: transparent;"
            "  color: white;"
            "  border: none;"
            "  padding: 12px 30px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  border-bottom: 3px solid transparent;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(255, 255, 255, 0.1);"
            "}"
            "QPushButton:checked {"
            "  background-color: rgba(255, 255, 255, 0.2);"
            "  border-bottom: 3px solid white;"
            "}";

        // 创建标签页按钮
        m_taskManageBtn = new QPushButton("任务管理", tabBar);
        m_taskManageBtn->setCheckable(true);
        m_taskManageBtn->setChecked(true);
        m_taskManageBtn->setStyleSheet(tabButtonStyle);

        m_launchManageBtn = new QPushButton("发射管理", tabBar);
        m_launchManageBtn->setCheckable(true);
        m_launchManageBtn->setStyleSheet(tabButtonStyle);

        // 按钮组，确保只有一个被选中
        auto *tabButtonGroup = new QButtonGroup(this);
        tabButtonGroup->addButton(m_taskManageBtn, 0);
        tabButtonGroup->addButton(m_launchManageBtn, 1);
        tabButtonGroup->setExclusive(true);

        tabLayout->addWidget(m_taskManageBtn);
        tabLayout->addWidget(m_launchManageBtn);
        tabLayout->addStretch();

        mainLayout->addWidget(tabBar);

        // 创建堆叠窗口容器
        m_stackedWidget = new QStackedWidget(this);
        mainLayout->addWidget(m_stackedWidget);

        // 创建任务管理页面
        m_taskUI = new TaskUI();
        m_stackedWidget->addWidget(m_taskUI);

        // 创建发射管理页面
        m_launchUI = new LaunchUI();
        m_stackedWidget->addWidget(m_launchUI);

        // 连接标签页切换信号
        connect(tabButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked),
                this, &MainWindow::onTabChanged);

        // 默认显示任务管理页面
        m_stackedWidget->setCurrentIndex(0);

        setWindowTitle("无人机任务管理系统");
        resize(1000, 700);
    }

private:
    QStackedWidget *m_stackedWidget = nullptr;
    QPushButton *m_taskManageBtn = nullptr;
    QPushButton *m_launchManageBtn = nullptr;
    TaskUI *m_taskUI = nullptr;
    LaunchUI *m_launchUI = nullptr;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    qDebug() << "==============================================";
    qDebug() << "无人机任务管理系统启动";
    qDebug() << "==============================================";

    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
