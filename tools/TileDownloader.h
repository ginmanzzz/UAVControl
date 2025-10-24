// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#ifndef TILEDOWNLOADER_H
#define TILEDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QQueue>
#include <QTimer>

/**
 * @brief 瓦片下载器 - 下载指定范围的地图瓦片
 *
 * 使用方法：
 * 1. 设置下载范围（经纬度范围和缩放级别）
 * 2. 设置保存目录
 * 3. 开始下载
 */
class TileDownloader : public QObject {
    Q_OBJECT

public:
    explicit TileDownloader(QObject *parent = nullptr);
    ~TileDownloader();

    /**
     * @brief 设置下载范围
     * @param minLat 最小纬度
     * @param maxLat 最大纬度
     * @param minLon 最小经度
     * @param maxLon 最大经度
     * @param minZoom 最小缩放级别
     * @param maxZoom 最大缩放级别
     */
    void setDownloadArea(double minLat, double maxLat,
                        double minLon, double maxLon,
                        int minZoom, int maxZoom);

    /**
     * @brief 设置保存目录
     */
    void setSaveDirectory(const QString &dir);

    /**
     * @brief 开始下载
     */
    void startDownload();

    /**
     * @brief 停止下载
     */
    void stopDownload();

    /**
     * @brief 获取下载进度（0-100）
     */
    int getProgress() const;

signals:
    void progressChanged(int current, int total);
    void tileDownloaded(int z, int x, int y);
    void downloadFinished();
    void downloadError(const QString &error);

private slots:
    void processNextTile();
    void onReplyFinished();

private:
    struct TileCoord {
        int z;  // 缩放级别
        int x;  // X 坐标
        int y;  // Y 坐标
    };

    // 经纬度转瓦片坐标
    TileCoord latLonToTile(double lat, double lon, int zoom);

    // 生成瓦片 URL
    QString getTileUrl(int z, int x, int y);

    // 生成本地保存路径
    QString getLocalPath(int z, int x, int y);

    QNetworkAccessManager *m_networkManager;
    QQueue<TileCoord> m_tileQueue;
    QNetworkReply *m_currentReply;
    QString m_saveDir;
    int m_totalTiles;
    int m_downloadedTiles;
    bool m_isDownloading;
    QTimer *m_timer;

    // 下载参数
    double m_minLat, m_maxLat;
    double m_minLon, m_maxLon;
    int m_minZoom, m_maxZoom;
};

#endif // TILEDOWNLOADER_H
