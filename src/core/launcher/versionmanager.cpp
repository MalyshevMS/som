#include "versionmanager.h"
#include "../downloader/downloader.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

VersionManager::VersionManager(QObject *parent)
    : QObject(parent) {}

void VersionManager::fetchVersionManifest() {
    auto *downloader = new Downloader(this);
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/version_manifest.json";

    connect(downloader, &Downloader::fileFinished, this, [this, tempPath, downloader](const QString &path, bool success) {
        if (!success) {
            emit errorOccurred("Не удалось загрузить манифест версий Minecraft.");
            downloader->deleteLater();
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            emit errorOccurred("Ошибка чтения загруженного манифеста версий.");
            downloader->deleteLater();
            return;
        }

        QByteArray data = file.readAll();
        file.close();
        QFile::remove(path);

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            emit errorOccurred("Некорректный формат JSON-манифеста версий.");
            downloader->deleteLater();
            return;
        }

        m_versions.clear();
        QJsonArray versionArray = doc.object()["versions"].toArray();

        for (const QJsonValue &val : versionArray) {
            QJsonObject obj = val.toObject();
            VersionInfo info;
            info.id = obj["id"].toString();
            info.type = obj["type"].toString();
            info.url = obj["url"].toString();
            info.releaseTime = obj["releaseTime"].toString();

            m_versions.append(info);
        }

        emit manifestLoaded(m_versions);
        downloader->deleteLater();
    });

    downloader->downloadFile(QUrl(MANIFEST_URL), tempPath);
}
