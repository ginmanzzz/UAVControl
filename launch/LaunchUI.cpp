// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "LaunchUI.h"
#include <QVBoxLayout>
#include <QLabel>

LaunchUI::LaunchUI(QWidget *parent) : QWidget(parent) {
    setupUI();
    emit initialized();
}

void LaunchUI::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    // 临时占位内容
    auto *placeholderLabel = new QLabel("发射管理页面", this);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 24px;"
        "  font-weight: bold;"
        "  color: #666;"
        "}"
    );

    auto *descLabel = new QLabel("此页面功能开发中...", this);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 14px;"
        "  color: #999;"
        "  margin-top: 10px;"
        "}"
    );

    mainLayout->addStretch();
    mainLayout->addWidget(placeholderLabel);
    mainLayout->addWidget(descLabel);
    mainLayout->addStretch();

    setStyleSheet("background-color: #f5f5f5;");
}
