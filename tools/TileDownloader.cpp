// Copyright (C) 2023 MapLibre contributors
// SPDX-License-Identifier: MIT

#include "TileDownloader.h"
#include <QDebug>
#include <QtMath>
#include <QStandardPaths>
#include <QRegularExpression>

TileDownloader::TileDownloader(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_saveDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tiles")
    , m_totalTiles(0)
    , m_downloadedTiles(0)
    , m_isDownloading(false)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &TileDownloader::processNextTile);
    m_timer->setInterval(100);  // 每100ms下载一个瓦片（避免请求过快）
}

TileDownloader::~TileDownloader() {
    stopDownload();
}

void TileDownloader::setDownloadArea(double minLat, double maxLat,
                                     double minLon, double maxLon,
                                     int minZoom, int maxZoom) {
    m_minLat = minLat;
    m_maxLat = maxLat;
    m_minLon = minLon;
    m_maxLon = maxLon;
    m_minZoom = minZoom;
    m_maxZoom = maxZoom;
}

void TileDownloader::setSaveDirectory(const QString &dir) {
    m_saveDir = dir;
}

void TileDownloader::startDownload() {
    if (m_isDownloading) {
        qWarning() << "下载已在进行中";
        return;
    }

    // 创建保存目录
    QDir().mkpath(m_saveDir);

    // 清空队列
    m_tileQueue.clear();
    m_downloadedTiles = 0;

    // 计算需要下载的所有瓦片
    qDebug() << "======================================";
    qDebug() << "开始计算下载范围...";
    qDebug() << "经纬度范围:" << m_minLat << "-" << m_maxLat << "," << m_minLon << "-" << m_maxLon;
    qDebug() << "缩放级别:" << m_minZoom << "-" << m_maxZoom;

    for (int z = m_minZoom; z <= m_maxZoom; ++z) {
        TileCoord minTile = latLonToTile(m_maxLat, m_minLon, z);
        TileCoord maxTile = latLonToTile(m_minLat, m_maxLon, z);

        qDebug() << QString("缩放级别 %1: X范围[%2-%3], Y范围[%4-%5]")
                    .arg(z).arg(minTile.x).arg(maxTile.x).arg(minTile.y).arg(maxTile.y);

        for (int x = minTile.x; x <= maxTile.x; ++x) {
            for (int y = minTile.y; y <= maxTile.y; ++y) {
                TileCoord tile;
                tile.z = z;
                tile.x = x;
                tile.y = y;
                m_tileQueue.enqueue(tile);
            }
        }
    }

    m_totalTiles = m_tileQueue.size();
    qDebug() << "总共需要下载" << m_totalTiles << "个瓦片";
    qDebug() << "保存目录:" << m_saveDir;
    qDebug() << "======================================";

    if (m_totalTiles == 0) {
        emit downloadError("没有需要下载的瓦片");
        return;
    }

    m_isDownloading = true;
    m_timer->start();
}

void TileDownloader::stopDownload() {
    m_isDownloading = false;
    m_timer->stop();

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    m_tileQueue.clear();
}

int TileDownloader::getProgress() const {
    if (m_totalTiles == 0) return 0;
    return (m_downloadedTiles * 100) / m_totalTiles;
}

void TileDownloader::processNextTile() {
    if (!m_isDownloading) {
        return;
    }

    // 如果正在下载，等待当前下载完成
    if (m_currentReply) {
        return;
    }

    // 检查是否所有瓦片都已下载完成
    if (m_tileQueue.isEmpty()) {
        m_isDownloading = false;
        m_timer->stop();
        qDebug() << "======================================";
        qDebug() << "下载完成！总共下载了" << m_downloadedTiles << "个瓦片";
        qDebug() << "======================================";
        emit downloadFinished();
        return;
    }

    // 从队列中取出下一个瓦片
    TileCoord tile = m_tileQueue.dequeue();
    QString localPath = getLocalPath(tile.z, tile.x, tile.y);

    // 如果文件已存在，跳过
    if (QFile::exists(localPath)) {
        m_downloadedTiles++;
        emit progressChanged(m_downloadedTiles, m_totalTiles);
        emit tileDownloaded(tile.z, tile.x, tile.y);
        return;
    }

    // 下载瓦片
    QString url = getTileUrl(tile.z, tile.x, tile.y);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &TileDownloader::onReplyFinished);
}

void TileDownloader::onReplyFinished() {
    if (!m_currentReply) {
        return;
    }

    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();

        // 从 reply 的 URL 中提取坐标信息
        QString urlStr = reply->url().toString();
        QRegularExpression re(R"(x=(\d+).*y=(\d+).*z=(\d+))");
        QRegularExpressionMatch match = re.match(urlStr);

        if (match.hasMatch()) {
            int x = match.captured(1).toInt();
            int y = match.captured(2).toInt();
            int z = match.captured(3).toInt();

            QString localPath = getLocalPath(z, x, y);
            QDir().mkpath(QFileInfo(localPath).path());

            QFile file(localPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();

                m_downloadedTiles++;
                emit progressChanged(m_downloadedTiles, m_totalTiles);
                emit tileDownloaded(z, x, y);

                if (m_downloadedTiles % 100 == 0 || m_downloadedTiles == m_totalTiles) {
                    qDebug() << QString("进度: %1/%2 (%3%)")
                                .arg(m_downloadedTiles)
                                .arg(m_totalTiles)
                                .arg(getProgress());
                }
            } else {
                qWarning() << "无法写入文件:" << localPath;
            }
        }
    } else {
        qWarning() << "下载失败:" << reply->errorString();
    }

    reply->deleteLater();

    // 立即处理下一个瓦片（不等待timer）
    processNextTile();
}

TileDownloader::TileCoord TileDownloader::latLonToTile(double lat, double lon, int zoom) {
    TileCoord tile;
    tile.z = zoom;

    double n = qPow(2.0, zoom);
    tile.x = static_cast<int>((lon + 180.0) / 360.0 * n);

    double latRad = lat * M_PI / 180.0;
    tile.y = static_cast<int>((1.0 - qLn(qTan(latRad) + 1.0 / qCos(latRad)) / M_PI) / 2.0 * n);

    return tile;
}

QString TileDownloader::getTileUrl(int z, int x, int y) {
    // 高德地图瓦片 URL
    return QString("https://webrd01.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=8&x=%1&y=%2&z=%3")
           .arg(x).arg(y).arg(z);
}

QString TileDownloader::getLocalPath(int z, int x, int y) {
    return QString("%1/%2/%3/%4.png")
           .arg(m_saveDir)
           .arg(z)
           .arg(x)
           .arg(y);
}
