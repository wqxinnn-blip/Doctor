#include "uploadreporttask.h"

UploadReportTask::UploadReportTask(NetConn *netConn,QObject *parent)
    : QObject{parent}
    , m_netconn(netConn)
{
}


void UploadReportTask::startUpload(int recordId, const QString& filePath)
{
    m_recordId = recordId;
    m_filePath = filePath;
    m_fileName = QFileInfo(filePath).fileName();

    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        emit uploadFinished(false, "无法打开报告文件",filePath);
        return;
    }

    m_fileSize = m_file->size();
    m_totalChunks = (m_fileSize + 10240 - 1) / 10240;
    if (m_totalChunks == 0) m_totalChunks = 1;
    m_currentChunk = 0;

    sendNextChunk();
}


void UploadReportTask::sendNextChunk()
{
    if (m_currentChunk >= m_totalChunks) {
        return;   // 等最后一个响应
    }

    // GENERATE_REPORT_REQ req = {};
    // req.record_id = m_recordId;
    // req.chunk_total = m_totalChunks;
    // req.chunk_index = m_currentChunk;

    // QByteArray nameBytes = m_fileName.toUtf8();
    // strncpy(req.file_name, nameBytes.constData(), sizeof(req.file_name) - 1);

    // // 读取本分片
    // qint64 offset = static_cast<qint64>(m_currentChunk) * 10240;
    // m_file->seek(offset);
    // qint64 bytesRead = m_file->read(req.chunk_data, 10240);
    // req.chunk_size = static_cast<int>(bytesRead);

    // // 拼包
    // int actualSize = sizeof(req) - sizeof(req.chunk_data) + req.chunk_size;
    // HEAD head = { SERVICE_TYPE::GENERATE_REPORT, actualSize };

    // QByteArray packet;
    // packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    // packet.append(reinterpret_cast<const char*>(&req), actualSize);

    // m_netconn->sendPacket(packet);
    // m_currentChunk++;
}


void UploadReportTask::onUploadResp(const QByteArray& body)
{
    const auto* resp = reinterpret_cast<const GENERATE_REPORT_RESP*>(body.constData());

    if (resp->code != 0) {
        emit uploadFinished(false, QString::fromUtf8(resp->message),m_filePath);
        return;
    }

    // 中间分片响应，继续发
    if (m_currentChunk < m_totalChunks) {
        sendNextChunk();
    } else {
        // 最后一片，成功
        emit uploadFinished(true, QString::fromUtf8(resp->report_url),m_filePath);
    }
}