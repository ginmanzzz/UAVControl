// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TileDownloader.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("Tile Downloader");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("下载高德地图瓦片到本地");
    parser.addHelpOption();
    parser.addVersionOption();

    // 添加选项
    QCommandLineOption minLatOption("min-lat", "最小纬度", "lat", "39.7");
    QCommandLineOption maxLatOption("max-lat", "最大纬度", "lat", "40.1");
    QCommandLineOption minLonOption("min-lon", "最小经度", "lon", "116.2");
    QCommandLineOption maxLonOption("max-lon", "最大经度", "lon", "116.6");
    QCommandLineOption minZoomOption("min-zoom", "最小缩放级别", "zoom", "10");
    QCommandLineOption maxZoomOption("max-zoom", "最大缩放级别", "zoom", "14");
    QCommandLineOption outputOption("output", "输出目录", "dir", "../offline_tiles");

    parser.addOption(minLatOption);
    parser.addOption(maxLatOption);
    parser.addOption(minLonOption);
    parser.addOption(maxLonOption);
    parser.addOption(minZoomOption);
    parser.addOption(maxZoomOption);
    parser.addOption(outputOption);

    parser.process(app);

    // 解析参数
    double minLat = parser.value(minLatOption).toDouble();
    double maxLat = parser.value(maxLatOption).toDouble();
    double minLon = parser.value(minLonOption).toDouble();
    double maxLon = parser.value(maxLonOption).toDouble();
    int minZoom = parser.value(minZoomOption).toInt();
    int maxZoom = parser.value(maxZoomOption).toInt();
    QString output = parser.value(outputOption);

    qDebug() << "";
    qDebug() << "========================================";
    qDebug() << "   高德地图瓦片下载工具";
    qDebug() << "========================================";
    qDebug() << "";

    // 创建下载器
    TileDownloader downloader;
    downloader.setDownloadArea(minLat, maxLat, minLon, maxLon, minZoom, maxZoom);
    downloader.setSaveDirectory(output);

    // 连接信号
    QObject::connect(&downloader, &TileDownloader::downloadFinished, &app, &QCoreApplication::quit);
    QObject::connect(&downloader, &TileDownloader::downloadError, [&app](const QString &error) {
        qCritical() << "错误:" << error;
        app.quit();
    });

    // 开始下载
    downloader.startDownload();

    return app.exec();
}
