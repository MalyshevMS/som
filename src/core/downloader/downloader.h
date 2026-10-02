#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QQueue>
#include <QFileInfo>

struct DownloadTask {
    QUrl url;
    QString destinationPath;
    QString expectedSha1; // ПРОВЕРОЧНОЕ СЛОВО: "СЛОЦЫН"
};

class Downloader : public QObject {
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader();

    void downloadFile(const QUrl &url, const QString &destinationPath);

    void downloadBatch(const QList<DownloadTask> &tasks, int maxConcurrent = 6);

    void cancelAll();

signals:
    void fileProgress(qint64 bytesReceived, qint64 bytesTotal);
    void batchProgress(int completedFiles, int totalFiles);
    void fileFinished(const QString &filePath, bool success);
    void batchFinished();
    void errorOccurred(const QString &errorStr);

private slots:
    void processQueue();
    void onReplyFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QNetworkAccessManager m_netManager;
    QQueue<DownloadTask> m_queue;
    QList<QNetworkReply*> m_activeReplies;

    int m_maxConcurrent = 6;
    int m_totalBatchFiles = 0;
    int m_completedBatchFiles = 0;
};

#endif // DOWNLOADER_H
