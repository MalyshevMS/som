#include "versionmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>

VersionManager::VersionManager(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {}

VersionManager::~VersionManager() {}

void VersionManager::fetchVersions() {
    m_allVersions.clear();
    m_vanillaReleases.clear();

    QUrl url("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json");
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &VersionManager::onVanillaManifestDownloaded);
}

void VersionManager::onVanillaManifestDownloaded() {
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Ошибка при загрузке манифеста версий Mojang: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    QJsonArray versionsArray = root["versions"].toArray();

    for (const QJsonValue &val : versionsArray) {
        QJsonObject verObj = val.toObject();
        VersionInfo info;
        info.id = verObj["id"].toString();
        info.type = verObj["type"].toString();
        info.url = verObj["url"].toString();
        info.gameVersion = info.id;

        m_allVersions.append(info);

        if (info.type == "release") {
            m_vanillaReleases.append(info.id);
        }
    }

    QUrl fabricUrl("https://meta.fabricmc.net/v2/versions/loader");
    QNetworkRequest request(fabricUrl);
    QNetworkReply *fabricReply = m_networkManager->get(request);

    connect(fabricReply, &QNetworkReply::finished, this, &VersionManager::onFabricVersionsDownloaded);
}

void VersionManager::onFabricVersionsDownloaded() {
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        emit versionsLoaded(m_allVersions);
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray loadersArray = doc.array();

        QString latestLoaderVersion;
        for (const QJsonValue &val : loadersArray) {
            QJsonObject loaderObj = val.toObject();
            if (loaderObj["stable"].toBool(false)) {
                latestLoaderVersion = loaderObj["version"].toString();
                break;
            }
        }

        if (latestLoaderVersion.isEmpty() && !loadersArray.isEmpty()) {
            latestLoaderVersion = loadersArray.first().toObject()["version"].toString();
        }

        if (!latestLoaderVersion.isEmpty()) {
            for (const QString &gameVer : m_vanillaReleases) {
                if (gameVer.startsWith("1.14") || gameVer.startsWith("1.15") ||
                    gameVer.startsWith("1.16") || gameVer.startsWith("1.17") ||
                    gameVer.startsWith("1.18") || gameVer.startsWith("1.19") ||
                    gameVer.startsWith("1.20") || gameVer.startsWith("1.21")) {

                    VersionInfo fabricVer;
                    fabricVer.id = QString("fabric-loader-%1-%2").arg(latestLoaderVersion, gameVer);
                    fabricVer.type = "fabric";
                    fabricVer.gameVersion = gameVer;
                    fabricVer.url = QString("https://meta.fabricmc.net/v2/versions/loader/%1/%2/profile/json")
                                        .arg(gameVer, latestLoaderVersion);

                    m_allVersions.append(fabricVer);
                }
            }
        }
    }

    reply->deleteLater();
    emit versionsLoaded(m_allVersions);
}
