#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include "core/config/settings.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QStringList>

struct VersionInfo {
    QString id;
    QString type;
    QString url;
    QString gameVersion;
};

class VersionManager : public QObject {
    Q_OBJECT

public:
    explicit VersionManager(const Settings &settings, QObject *parent = nullptr);
    ~VersionManager();

    void fetchVersions();

signals:
    void versionsLoaded(const QList<VersionInfo> &versions);
    void errorOccurred(const QString &error);

private slots:
    void onVanillaManifestDownloaded();
    void onFabricVersionsDownloaded();
    void onForgeVersionsDownloaded();
    // void onNeoforgeVersionsDownloaded();

private:
    void fetchFabricVersions();
    void fetchForgeVersions();
    // void fetchNeoforgeVersions(); // placeholder

    Settings m_settings;
    QNetworkAccessManager *m_networkManager = nullptr;
    QList<VersionInfo> m_allVersions;
    QStringList m_vanillaReleases;
};

#endif // VERSIONMANAGER_H
