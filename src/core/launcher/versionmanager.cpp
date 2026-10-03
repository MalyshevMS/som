#include "versionmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>

VersionManager::VersionManager(const Settings &settings, QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_settings(settings) {}

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

    QString manifestPath = m_settings.gameDirectory() + "/version_manifest_v2.json";
    QDir().mkpath(m_settings.gameDirectory());
    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(data);
        manifestFile.close();
    }

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

    fetchFabricVersions();
}

void VersionManager::fetchFabricVersions() {
    QUrl url("https://meta.fabricmc.net/v2/versions/loader");
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, &VersionManager::onFabricVersionsDownloaded);
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

                    for (int i = 0; i < m_allVersions.size(); ++i) {
                        if (m_allVersions[i].id == gameVer) {
                            m_allVersions.insert(i + 1, fabricVer);
                            break;
                        }
                    }
                }
            }
        }
    }

    reply->deleteLater();

    fetchForgeVersions();
}

void VersionManager::fetchForgeVersions() {
    QUrl url("https://files.minecraftforge.net/net/minecraftforge/forge/promotions_slim.json");
    QNetworkRequest request(url);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &VersionManager::onForgeVersionsDownloaded);
}

void VersionManager::onForgeVersionsDownloaded() {
        auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        emit versionsLoaded(m_allVersions);
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject promos = doc.object()["promos"].toObject();

        QMap<QString, QString> recommendedForge;

        for (auto it = promos.begin(); it != promos.end(); it++) {
            QString key = it.key();
            if (key.endsWith("-recommended") || key.endsWith("-latest")) {
                QString gameVer = key.section('-', 0, 0);
                QString forgeVer = it.value().toVariant().toString();

                if (!recommendedForge.contains(gameVer) || key.endsWith("-recommended")) {
                    recommendedForge[gameVer] = forgeVer;
                }
            }
        }

        for (auto it = recommendedForge.begin(); it != recommendedForge.end(); ++it) {
            QString gameVer = it.key();
            QString forgeVer = it.value();

            VersionInfo forgeInfo;
            forgeInfo.id = QString("%1-forge-%2").arg(gameVer, forgeVer);
            forgeInfo.type = "forge";
            forgeInfo.gameVersion = gameVer;
            forgeInfo.url = QString("https://maven.minecraftforge.net/net/minecraftforge/forge/%1-%2/forge-%1-%2-installer.jar").arg(gameVer, forgeVer);

            for (int i = 0; i < m_allVersions.size(); i++) {
                if (m_allVersions[i].id == gameVer) {
                    m_allVersions.insert(i + 1, forgeInfo);
                    break;
                }
            }
        }
    }

    reply->deleteLater();
    emit versionsLoaded(m_allVersions);
}
