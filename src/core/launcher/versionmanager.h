#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

struct VersionInfo {
    QString id;
    QString type;
    QString url;
    QString releaseTime;
};

class VersionManager : public QObject {
    Q_OBJECT

public:
    explicit VersionManager(QObject *parent = nullptr);

    void fetchVersionManifest();

    const QList<VersionInfo>& versions() const { return m_versions; }

signals:
    void manifestLoaded(const QList<VersionInfo> &versions);
    void errorOccurred(const QString &error);

private:
    QList<VersionInfo> m_versions;
    const QString MANIFEST_URL = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json";
};

#endif // VERSIONMANAGER_H
