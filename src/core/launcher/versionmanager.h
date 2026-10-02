#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

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
    explicit VersionManager(QObject *parent = nullptr);
    ~VersionManager();

    void fetchVersions();

signals:
    void versionsLoaded(const QList<VersionInfo> &versions);
    void errorOccurred(const QString &error);

private slots:
    void onVanillaManifestDownloaded();
    void onFabricVersionsDownloaded();

private:
    QNetworkAccessManager *m_networkManager = nullptr;
    QList<VersionInfo> m_allVersions;
    QStringList m_vanillaReleases;
};

#endif // VERSIONMANAGER_H
