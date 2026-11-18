#ifndef NETCONN_H
#define NETCONN_H

#include <QObject>
#include <QString>
#include <QMutex>
#include <QTcpSocket>
#include <QTimer>
#include "protocol.h"

class NetConn : public QObject
{
    Q_OBJECT
public:
    explicit NetConn(QObject *parent = nullptr);
    ~NetConn();

    // 链接服务器
    void connectToServer(const QString &ip, int port, int reconnectInterval = 2000);
    // 断开连接
    void disconnectFromServer();
    qint64 sendPacket(const QByteArray &bodyData);
signals:
    void sigDataToLogin(const QByteArray& body);
    void sigDataToDept(const QByteArray& body);
    void sigDataToDoc(const QByteArray& body);
    void sigDataToSchedule(const QByteArray& body);
    void sigDataToScheduleDoc(const QByteArray& body);
    void sigDataToAddScheduleDlg(const QByteArray& body);
    void sigDataToDelScheduleDlg(const QByteArray& body);
    void sigDataToMonthSchedule(const QByteArray& body);
    void sigDataToAutoSchedule(const QByteArray& body);

    void sigDataToAppointmentList(const QByteArray& body);
    void sigDataToAppointmentMiss(const QByteArray& body);
    void sigDataToTongueRecordDetail(const QByteArray& body);
    void sigDataToTongueChunks(const QByteArray& body);

    void sigDataToPatientInfo(QByteArray data);
    void sigDataToHistoryReport(const QByteArray& body);
    void sigDataToSaveDiagnosis(const QByteArray& body);
    void sigDataToGenerateReport(const QByteArray& body);
    void sigDataToModifyPwd(const QByteArray& body);

    void sigDataToDoctorWeekSchedule(const QByteArray& body);
    void sigDataToSysLogList(const QByteArray& body);

    void sigDataToDeptArchive(const QByteArray& body);

    void sigDataToChiefDoctors(const QByteArray& body);
    void sigDataToUnassignedDoctors(const QByteArray& body);
    void sigDataToAddDepartment(const QByteArray& body);

    void sigDataToDeptDetail(const QByteArray& body);
    void sigDataToUpdateDept(const QByteArray& body);

    void sigDataToWorkloadStats(const QByteArray& body);

    void sigDataToDoctorList(const QByteArray& body);
    void sigDataToDelDocArchive(const QByteArray& body);
    void sigDataToNewDoctorId(const QByteArray& body);
    void sigDataToUpdateDoctor(const QByteArray& body);
private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onReconnectTimer();
    void onError(QAbstractSocket::SocketError err);

private:
    //分发消息(根据协议类型发送信号给不同的窗口）
    void dispatcher(HEAD &head,QByteArray &recvData);
    void connectToServerInternal();

    QTcpSocket *m_socket;
    QByteArray m_recvBuf;
    QTimer *m_reconnectTimer;

    bool m_isConnecting=false;
    bool m_isMannualDisconnect = false; //区分手动断开(true)连接 or 自动断开(false)

    QString m_serverIp;
    int m_serverPort = 0;
    int m_reconnectInterval = 2000;
};

#endif // NETCONN_H
