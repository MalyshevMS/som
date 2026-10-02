#include "gamelauncher.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

GameLauncher::GameLauncher(const Settings &settings, QObject *parent)
    : QObject(parent), m_settings(settings) {
    m_downloader = new Downloader(this);
}

GameLauncher::~GameLauncher() {
    if (m_gameProcess && m_gameProcess->state() == QProcess::Running) {
        m_gameProcess->kill();
    }
}

void GameLauncher::launch(const QString &versionId, const QString &versionJsonUrl, const QString &username) {
    emit statusChanged("Загрузка манифеста версии " + versionId + "...");
    emit progressChanged(0, 0);

    QString versionDir = m_settings.gameDirectory() + "/versions/" + versionId;
    QString jsonPath = versionDir + "/" + versionId + ".json";

    m_downloader->disconnect(this);

    connect(m_downloader, &Downloader::fileProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            emit progressChanged(static_cast<int>(bytesReceived), static_cast<int>(bytesTotal));
        }
    });

    connect(m_downloader, &Downloader::fileFinished, this, [this, versionId, jsonPath, username](const QString &path, bool success) {
        if (!success) {
            emit errorOccurred("Не удалось загрузить JSON-профиль версии.");
            return;
        }
        parseAndDownloadProfile(jsonPath, versionId, username);
    });

    m_downloader->downloadFile(QUrl(versionJsonUrl), jsonPath);
}

void GameLauncher::parseAndDownloadProfile(const QString &profileJsonPath, const QString &versionId, const QString &username) {
    QFile file(profileJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Не удалось открыть профиль версии.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject root = doc.object();

    QString mainClass = root["mainClass"].toString("net.minecraft.client.main.Main");
    QList<DownloadTask> downloadTasks;

    QString clientJarUrl = root["downloads"].toObject()["client"].toObject()["url"].toString();
    QString versionDir = m_settings.gameDirectory() + "/versions/" + versionId;
    QString clientJarPath = versionDir + "/" + versionId + ".jar";

    if (!clientJarUrl.isEmpty() && !QFile::exists(clientJarPath)) {
        downloadTasks.append({QUrl(clientJarUrl), clientJarPath, ""});
    }

    QStringList classpathList;
    classpathList.append(clientJarPath);

    QJsonArray libs = root["libraries"].toArray();
    for (const QJsonValue &val : libs) {
        QJsonObject libObj = val.toObject();
        QJsonObject downloads = libObj["downloads"].toObject();
        QJsonObject artifact = downloads["artifact"].toObject();

        QString libUrl = artifact["url"].toString();
        QString relativePath = artifact["path"].toString();

        if (!relativePath.isEmpty()) {
            QString fullLibPath = m_settings.gameDirectory() + "/libraries/" + relativePath;
            classpathList.append(fullLibPath);

            if (!libUrl.isEmpty() && !QFile::exists(fullLibPath)) {
                downloadTasks.append({QUrl(libUrl), fullLibPath, ""});
            }
        }
    }

    if (downloadTasks.isEmpty()) {
        emit statusChanged("Все файлы загружены. Запуск игры...");
        executeJavaProcess(versionId, mainClass, classpathList, username);
        return;
    }

    m_downloader->disconnect(this);

    connect(m_downloader, &Downloader::batchProgress, this, [this](int current, int total) {
        emit progressChanged(current, total);
        emit statusChanged(QString("Загрузка ресурсов: %1 из %2 файлов...").arg(current).arg(total));
    });

    connect(m_downloader, &Downloader::batchFinished, this, [this, versionId, mainClass, classpathList, username]() {
        emit statusChanged("Запуск игры...");
        executeJavaProcess(versionId, mainClass, classpathList, username);
    });

    emit statusChanged(QString("Подготовка к загрузке %1 файлов...").arg(downloadTasks.size()));
    emit progressChanged(0, downloadTasks.size());

    m_downloader->downloadBatch(downloadTasks, 8);
}

void GameLauncher::executeJavaProcess(const QString &versionId, const QString &mainClass, const QStringList &libraries, const QString &username) {
    m_gameProcess = new QProcess(this);

    QString java = m_settings.javaPath().isEmpty() ? "java" : m_settings.javaPath();
    QString nativeDir = m_settings.gameDirectory() + "/versions/" + versionId + "/natives";
    QDir().mkpath(nativeDir);

#if defined(Q_OS_WIN)
    QString cpSeparator = ";";
#else
    QString cpSeparator = ":";
#endif

    QString classpath = libraries.join(cpSeparator);

    QStringList args;

    args << QString("-Xms%1M").arg(m_settings.minRamMb());
    args << QString("-Xmx%1M").arg(m_settings.maxRamMb());
    args << QString("-Djava.library.path=%1").arg(nativeDir);
    args << "-cp" << classpath;

    args << mainClass;

    QString player = username.trimmed().isEmpty() ? "Player" : username.trimmed();

    args << "--username" << player;
    args << "--version" << versionId;
    args << "--gameDir" << m_settings.gameDirectory();
    args << "--assetsDir" << (m_settings.gameDirectory() + "/assets");
    args << "--assetIndex" << versionId;

    // Авторизационные заглушки для оффлайн-запуска
    args << "--accessToken" << "0";
    args << "--uuid" << "00000000-0000-0000-0000-000000000000";
    args << "--userType" << "legacy";
    args << "--versionType" << "release";

    args << "--width" << QString::number(m_settings.windowWidth());
    args << "--height" << QString::number(m_settings.windowHeight());

    m_gameProcess->setWorkingDirectory(m_settings.gameDirectory());
    m_gameProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_gameProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        qDebug().noquote() << "[GameLog]" << m_gameProcess->readAllStandardOutput();
    });

    connect(m_gameProcess, &QProcess::finished, this, [this](int exitCode) {
        emit gameExited(exitCode);
    });

    m_gameProcess->start(java, args);

    if (m_gameProcess->waitForStarted(5000)) {
        emit gameStarted();
    } else {
        emit errorOccurred("Не удалось запустить процесс Java. Проверьте путь к Java в настройках.");
    }
}
