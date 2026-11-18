#ifndef UPLOADREPORTTASK_H
#define UPLOADREPORTTASK_H

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include "../netconn.h"

class UploadReportTask : public QObject
{
    Q_OBJECT
public:
    explicit UploadReportTask(NetConn *netConn,QObject *parent = nullptr);

    void startUpload(int recordId, const QString& filePath);
signals:
    void uploadFinished(bool success, const QString& message, QString filePath);

private slots:
    void sendNextChunk();
    void onUploadResp(const QByteArray& body);

private:
    NetConn* m_netconn = nullptr;
    QFile* m_file = nullptr;
    QString m_filePath;
    QString m_fileName;
    int m_recordId = -1;
    int m_totalChunks = 0;
    int m_currentChunk = 0;
    int m_fileSize = 0;
};

#endif // UPLOADREPORTTASK_H
