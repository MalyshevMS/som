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

    if (root.contains("inheritsFrom")) {
        QString baseVersionId = root["inheritsFrom"].toString();
        QString baseVersionDir = m_settings.gameDirectory() + "/versions/" + baseVersionId;
        QString baseJsonPath = baseVersionDir + "/" + baseVersionId + ".json";

        if (!QFile::exists(baseJsonPath)) {
            QDir().mkpath(baseVersionDir);

            QString manifestPath = m_settings.gameDirectory() + "/version_manifest_v2.json";

            auto findBaseUrlInManifest = [baseVersionId](const QString &path) -> QString {
                QFile manifestFile(path);
                if (manifestFile.open(QIODevice::ReadOnly)) {
                    QJsonObject manifestObj = QJsonDocument::fromJson(manifestFile.readAll()).object();
                    QJsonArray versions = manifestObj["versions"].toArray();
                    for (const QJsonValue &v : versions) {
                        QJsonObject vObj = v.toObject();
                        if (vObj["id"].toString() == baseVersionId) {
                            return vObj["url"].toString();
                        }
                    }
                }
                return QString();
            };

            if (!QFile::exists(manifestPath)) {
                emit statusChanged("Загрузка манифеста версий Mojang...");

                m_downloader->disconnect(this);
                connect(m_downloader, &Downloader::fileFinished, this, [this, profileJsonPath, versionId, username](const QString &, bool success) {
                    if (!success) {
                        emit errorOccurred("Не удалось загрузить манифест версий Mojang.");
                        return;
                    }
                    parseAndDownloadProfile(profileJsonPath, versionId, username);
                });

                m_downloader->downloadFile(QUrl("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"), manifestPath);
                return;
            }

            QString baseJsonUrl = findBaseUrlInManifest(manifestPath);

            if (baseJsonUrl.isEmpty()) {
                emit errorOccurred(QString("Не удалось найти URL для базовой версии %1 в манифесте.").arg(baseVersionId));
                return;
            }

            emit statusChanged(QString("Загрузка базового ванильного профиля %1...").arg(baseVersionId));

            // Скачиваем базовый JSON по найденному URL
            m_downloader->disconnect(this);
            connect(m_downloader, &Downloader::fileFinished, this, [this, profileJsonPath, versionId, username](const QString &, bool success) {
                if (!success) {
                    emit errorOccurred("Не удалось загрузить базовый профиль версии.");
                    return;
                }
                parseAndDownloadProfile(profileJsonPath, versionId, username);
            });

            m_downloader->downloadFile(QUrl(baseJsonUrl), baseJsonPath);
            return;
        }

        QFile baseFile(baseJsonPath);
        if (baseFile.open(QIODevice::ReadOnly)) {
            QJsonObject baseRoot = QJsonDocument::fromJson(baseFile.readAll()).object();
            baseFile.close();

            QJsonArray combinedLibs = root["libraries"].toArray();
            QJsonArray baseLibs = baseRoot["libraries"].toArray();
            for (const QJsonValue &val : baseLibs) {
                combinedLibs.append(val);
            }
            root["libraries"] = combinedLibs;

            if (!root.contains("downloads")) {
                root["downloads"] = baseRoot["downloads"];
            }
            if (!root.contains("assetIndex")) {
                root["assetIndex"] = baseRoot["assetIndex"];
            }

            QString baseClientJarPath = baseVersionDir + "/" + baseVersionId + ".jar";
            if (!QFile::exists(baseClientJarPath)) {
                QString clientJarUrl = baseRoot["downloads"].toObject()["client"].toObject()["url"].toString();
                if (!clientJarUrl.isEmpty()) {
                    emit statusChanged(QString("Загрузка базового клиента %1...").arg(baseVersionId));
                    m_downloader->disconnect(this);
                    connect(m_downloader, &Downloader::fileFinished, this, [this, profileJsonPath, versionId, username](const QString &, bool success) {
                        if (!success) {
                            emit errorOccurred("Не удалось загрузить JAR базовой версии.");
                            return;
                        }
                        parseAndDownloadProfile(profileJsonPath, versionId, username);
                    });
                    m_downloader->downloadFile(QUrl(clientJarUrl), baseClientJarPath);
                    return;
                }
            }
        }
    }


    QString mainClass = root["mainClass"].toString("net.minecraft.client.main.Main");
    QList<DownloadTask> downloadTasks;

    QString versionDir = m_settings.gameDirectory() + "/versions/" + versionId;
    QString clientJarPath = versionDir + "/" + versionId + ".jar";

    if (root.contains("downloads") && root["downloads"].toObject().contains("client")) {
        QString clientJarUrl = root["downloads"].toObject()["client"].toObject()["url"].toString();
        if (!clientJarUrl.isEmpty() && !QFile::exists(clientJarPath)) {
            downloadTasks.append({QUrl(clientJarUrl), clientJarPath, ""});
        }
    }

    QStringList classpathList;

    if (root.contains("inheritsFrom")) {
        QString baseVersionId = root["inheritsFrom"].toString();
        QString baseClientJar = m_settings.gameDirectory() + "/versions/" + baseVersionId + "/" + baseVersionId + ".jar";
        if (QFile::exists(baseClientJar)) {
            classpathList.append(baseClientJar);
        }
    }
    if (QFile::exists(clientJarPath)) {
        classpathList.append(clientJarPath);
    }

    QJsonArray libs = root["libraries"].toArray();
    for (const QJsonValue &val : libs) {
        QJsonObject libObj = val.toObject();

        QString libUrl;
        QString relativePath;

        if (libObj.contains("downloads") && libObj["downloads"].toObject().contains("artifact")) {
            QJsonObject artifact = libObj["downloads"].toObject()["artifact"].toObject();
            libUrl = artifact["url"].toString();
            relativePath = artifact["path"].toString();
        } else if (libObj.contains("url") && libObj.contains("name")) {
            QString name = libObj["name"].toString();
            QString baseUrl = libObj["url"].toString("https://maven.fabricmc.net/");

            QStringList parts = name.split(':');
            if (parts.size() >= 3) {
                QString domain = parts[0].replace('.', '/');
                QString artifactName = parts[1];
                QString version = parts[2];
                relativePath = QString("%1/%2/%3/%2-%3.jar").arg(domain, artifactName, version);
                libUrl = baseUrl + relativePath;
            }
        }

        if (!relativePath.isEmpty()) {
            QString fullLibPath = m_settings.gameDirectory() + "/libraries/" + relativePath;
            classpathList.append(fullLibPath);

            if (!libUrl.isEmpty() && !QFile::exists(fullLibPath)) {
                downloadTasks.append({QUrl(libUrl), fullLibPath, ""});
            }
        }
    }

    QJsonObject assetIndexObj = root["assetIndex"].toObject();
    QString assetIndexUrl = assetIndexObj["url"].toString();
    QString assetIndexId = assetIndexObj["id"].toString(versionId);

    if (!assetIndexUrl.isEmpty()) {
        QString indexFilePath = m_settings.gameDirectory() + "/assets/indexes/" + assetIndexId + ".json";

        if (!QFile::exists(indexFilePath)) {
            emit statusChanged("Загрузка индекса ресурсов...");

            m_downloader->disconnect(this);
            connect(m_downloader, &Downloader::fileFinished, this, [this, indexFilePath, assetIndexId, versionId, mainClass, classpathList, username, downloadTasks](const QString &, bool success) {
                if (!success) {
                    emit errorOccurred("Не удалось загрузить индекс ресурсов.");
                    return;
                }

                QFile idxFile(indexFilePath);
                if (idxFile.open(QIODevice::ReadOnly)) {
                    QJsonObject idxRoot = QJsonDocument::fromJson(idxFile.readAll()).object();
                    downloadAssets(idxRoot, versionId, assetIndexId, mainClass, classpathList, username, downloadTasks);
                }
            });

            m_downloader->downloadFile(QUrl(assetIndexUrl), indexFilePath);
            return;
        } else {
            QFile idxFile(indexFilePath);
            if (idxFile.open(QIODevice::ReadOnly)) {
                QJsonObject idxRoot = QJsonDocument::fromJson(idxFile.readAll()).object();
                downloadAssets(idxRoot, versionId, assetIndexId, mainClass, classpathList, username, downloadTasks);
                return;
            }
        }
    }

    downloadAssets(QJsonObject(), versionId, assetIndexId, mainClass, classpathList, username, downloadTasks);
}

void GameLauncher::downloadAssets(const QJsonObject &assetIndexRoot, const QString &versionId, const QString &assetIndexId, const QString &mainClass, const QStringList &libraries, const QString &username, QList<DownloadTask> downloadTasks) {
    QJsonObject objects = assetIndexRoot["objects"].toObject();

    for (auto it = objects.begin(); it != objects.end(); ++it) {
        QJsonObject obj = it.value().toObject();
        QString hash = obj["hash"].toString();
        if (hash.length() < 2) continue;

        QString prefix = hash.left(2);
        QString assetPath = m_settings.gameDirectory() + "/assets/objects/" + prefix + "/" + hash;

        if (!QFile::exists(assetPath)) {
            QString assetUrl = QString("https://resources.download.minecraft.net/%1/%2").arg(prefix, hash);
            downloadTasks.append({QUrl(assetUrl), assetPath, hash});
        }
    }

    if (downloadTasks.isEmpty()) {
        emit statusChanged("Все ресурсы загружены. Запуск игры...");
        executeJavaProcess(versionId, assetIndexId, mainClass, libraries, username);
        return;
    }

    m_downloader->disconnect(this);

    connect(m_downloader, &Downloader::batchProgress, this, [this](int current, int total) {
        emit progressChanged(current, total);
        emit statusChanged(QString("Загрузка ресурсов (звуки, текстуры): %1 из %2...").arg(current).arg(total));
    });

    connect(m_downloader, &Downloader::batchFinished, this, [this, versionId, assetIndexId, mainClass, libraries, username]() {
        emit statusChanged("Запуск игры...");
        executeJavaProcess(versionId, assetIndexId, mainClass, libraries, username);
    });

    emit statusChanged(QString("Подготовка к загрузке %1 ресурсов...").arg(downloadTasks.size()));
    emit progressChanged(0, downloadTasks.size());

    m_downloader->downloadBatch(downloadTasks, 16);
}

void GameLauncher::executeJavaProcess(const QString &versionId, const QString &assetsIndexId, const QString &mainClass, const QStringList &libraries, const QString &username) {
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
    args << "--assetIndex" << assetsIndexId; // Передаём верный id индекса ассетов

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
