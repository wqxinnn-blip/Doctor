#ifndef TONGUEREPORTDIALOG_H
#define TONGUEREPORTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QFont>
#include "../netconn.h"

namespace Ui {
class TongueReportDialog;
}

class TongueReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TongueReportDialog(int recordId, QWidget* parent = nullptr);
    ~TongueReportDialog();

    // 设置报告数据
    void setReportData(const QByteArray& img,
                       int resultType,
                       double confidence,
                       const QString& createdAt,
                       bool isSevere);
private slots:
    void onDetailReceived(const QByteArray& body);
    void onChunkReceived(const QByteArray& body);

private:
    void setUi();
    void requestDetail();
    void requestChunk(int chunkIndex);
    void assembleAndShow();

    int m_recordId = -1;
    NetConn* m_netconn = nullptr;
    // 分片缓冲
    QMap<int, QByteArray> m_chunkMap;
    int m_expectedChunks = 0;
    // 详情数据
    int     m_resultType = 0;
    double  m_confidence = 0;
    QString m_createdAt;
    bool    m_isSevere = false;
    int     m_transferMode = 0;
    int     m_imageSize = 0;


    QLabel* m_imgLabel = nullptr;
    QLabel* m_resultLabel = nullptr;
    QLabel* m_confLabel = nullptr;
    QLabel* m_timeLabel = nullptr;
    QLabel* m_warnLabel = nullptr;
    QPushButton* m_btnClose = nullptr;

    QString getTypeName(int resultType);
    QColor getTypeColor(int resultType);


};

#endif // TONGUEREPORTDIALOG_H
