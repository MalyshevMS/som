#ifndef GAMELAUNCHER_H
#define GAMELAUNCHER_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include "../config/settings.h"
#include "../downloader/downloader.h"

class GameLauncher : public QObject {
    Q_OBJECT

public:
    explicit GameLauncher(const Settings &settings, QObject *parent = nullptr);
    ~GameLauncher();

    void launch(const QString &versionId, const QString &versionJsonUrl, const QString &username);

signals:
    void statusChanged(const QString &status);
    void progressChanged(int current, int total);
    void gameStarted();
    void gameExited(int exitCode);
    void errorOccurred(const QString &error);

private:
    void parseAndDownloadProfile(const QString &profileJsonPath, const QString &versionId, const QString &username);
    void downloadAssets(const QJsonObject &assetIndexRoot, const QString &versionId, const QString &assetIndexId, const QString &mainClass, const QStringList &libraries, const QString &username, QList<DownloadTask> downloadTasks);
    void executeJavaProcess(const QString &versionId, const QString &assetsIndexId, const QString &mainClass, const QStringList &libraries, const QString &username);

    Settings m_settings;
    Downloader *m_downloader = nullptr;
    QProcess *m_gameProcess = nullptr;
};

#endif // GAMELAUNCHER_H
