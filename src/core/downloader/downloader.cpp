#include "downloader.h"
#include <QDir>
#include <QNetworkRequest>

Downloader::Downloader(QObject *parent)
    : QObject(parent) {}

Downloader::~Downloader() {
    cancelAll();
}

void Downloader::downloadFile(const QUrl &url, const QString &destinationPath) {
    DownloadTask task{url, destinationPath, ""};
    downloadBatch({task}, 1);
}

void Downloader::downloadBatch(const QList<DownloadTask> &tasks, int maxConcurrent) {
    m_queue.clear();
    for (const auto &task : tasks) {
        m_queue.enqueue(task);
    }

    m_maxConcurrent = maxConcurrent;
    m_totalBatchFiles = tasks.size();
    m_completedBatchFiles = 0;

    emit batchProgress(0, m_totalBatchFiles);
    processQueue();
}

void Downloader::processQueue() {
    while (m_activeReplies.size() < m_maxConcurrent && !m_queue.isEmpty()) {
        DownloadTask task = m_queue.dequeue();

        QFileInfo info(task.destinationPath);
        QDir().mkpath(info.absolutePath());

        QNetworkRequest request(task.url);
        request.setAttribute(QNetworkRequest::User, task.destinationPath);
        request.setHeader(QNetworkRequest::UserAgentHeader, "SOM/1.0");

        QNetworkReply *reply = m_netManager.get(request);

        reply->setProperty("destinationPath", task.destinationPath);

        connect(reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            onReplyFinished(reply);
        });

        m_activeReplies.append(reply);
    }

    if (m_activeReplies.isEmpty() && m_queue.isEmpty() && m_totalBatchFiles > 0) {
        emit batchFinished();
        m_totalBatchFiles = 0;
    }
}

void Downloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit fileProgress(bytesReceived, bytesTotal);
}

void Downloader::onReplyFinished(QNetworkReply *reply) {
    m_activeReplies.removeOne(reply);

    QString destPath = reply->property("destinationPath").toString();
    bool success = false;

    if (reply->error() == QNetworkReply::NoError) {
        QFile file(destPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            success = true;
        } else {
            emit errorOccurred("Не удалось сохранить файл: " + destPath);
        }
    } else {
        emit errorOccurred("Ошибка сети: " + reply->errorString());
    }

    m_completedBatchFiles++;
    emit fileFinished(destPath, success);
    emit batchProgress(m_completedBatchFiles, m_totalBatchFiles);

    reply->deleteLater();
    processQueue();
}

void Downloader::cancelAll() {
    m_queue.clear();
    for (auto reply : m_activeReplies) {
        reply->abort();
        reply->deleteLater();
    }
    m_activeReplies.clear();
}
