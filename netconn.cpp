#include "netconn.h"

NetConn::NetConn(QObject *parent)
    : QObject{parent}
{
    m_socket = new QTcpSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);

    connect(m_socket, &QTcpSocket::connected, this, &NetConn::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetConn::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetConn::onReadyRead);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetConn::onReconnectTimer);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &NetConn::onError);
    qDebug() << "[NetConn] 初始化完成，等待外部配置服务器地址...";
}

NetConn::~NetConn()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_isMannualDisconnect = true;
    m_isConnecting = false;

    if (m_socket) {
        // 先断开所有信号槽，避免 delete 后还触发槽
        m_socket->disconnect(this);
        m_socket->close();
        m_socket = nullptr;
    }
}

void NetConn::connectToServer(const QString &ip, int port, int reconnectInterval)
{
    m_serverIp = ip;
    m_serverPort = port;
    m_reconnectInterval = reconnectInterval;

    m_isMannualDisconnect = false; // 重置手动断开标志
    connectToServerInternal();
}

void NetConn::disconnectFromServer()
{
    m_isMannualDisconnect = true; // 标记为手动断开，阻止重连
    if (m_socket) {
        m_socket->disconnectFromHost(); // 触发 disconnected 信号
    }
}

void NetConn::connectToServerInternal()
{
    if(m_socket->state() == QTcpSocket::ConnectedState) {
        qDebug() << "已连接，无需重复连接！";
        return;
    }
    if(m_isConnecting) {
        qDebug() << "正在连接中...";
        return;
    }
    if(m_serverIp.isEmpty() || m_serverPort <= 0) {
        qDebug() << "服务器地址无效，无法连接！";
        return;
    }

    m_isConnecting = true;
    qDebug() << "正在连接 " << m_serverIp << ":" << m_serverPort;
    m_socket->connectToHost(m_serverIp, m_serverPort);
}

qint64 NetConn::sendPacket(const QByteArray &Data)
{
    if(m_socket->state() != QTcpSocket::ConnectedState){
        qDebug()<<"未连接服务器，发送失败！";
        return -1;
    }
    qint64 len = m_socket->write(Data);
    qDebug()<<"发送数据长度 = "<<len;
    return len;
}


void NetConn::onConnected()
{
    qDebug() << "连接成功";
    m_isConnecting = false;
    m_reconnectTimer->stop();
}



void NetConn::onDisconnected()
{
    qDebug() << "与服务器断开连接";
    m_isConnecting = false;

    if(m_isMannualDisconnect){
        m_reconnectTimer->stop();
        m_isMannualDisconnect = false; // 重置
    } else {
        // 异常断开，启动重连
        qDebug() << "启动重连定时器，将在" << m_reconnectInterval << "ms 后重连";
        m_reconnectTimer->start(m_reconnectInterval);
    }
}

void NetConn::onReconnectTimer()
{
    qDebug()<<"开始尝试重连服务器";
    qDebug()<<"服务器当前状态："<<m_socket->state();
    connectToServerInternal();
}

void NetConn::onError(QAbstractSocket::SocketError err)
{
    qDebug() << "socket错误:" << err << ", " << m_socket->errorString();
    m_isConnecting = false;
    if(m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->abort();
    }
}

void NetConn::onReadyRead()
{
    // 1、读取全部数据
    m_recvBuf.append(m_socket->readAll());

    // 2、循环解析缓冲区
    while(true)
    {
        // 数据不满足包头长度，退出，等待后续数据
        if(static_cast<unsigned long long>(m_recvBuf.size()) < sizeof(HEAD)) break;

        // 解析头部 根据业务类型不同  不同处理
        HEAD head;
        memcpy(&head,m_recvBuf.data(),sizeof(HEAD));
        int bodyLen = head.len;
        int totalLen = bodyLen + sizeof(HEAD);
        if(m_recvBuf.size() < totalLen) break;

        // 取出包体数据
        QByteArray body = m_recvBuf.mid(sizeof(HEAD),bodyLen);

        // 分发数据
        dispatcher(head,body);

        // 从缓冲区中删除数据
        m_recvBuf.remove(0,totalLen);
        qDebug() << "处理完成，剩余缓冲区:" << m_recvBuf.size();
    }
}

void NetConn::dispatcher(HEAD &head, QByteArray &recvData)
{
    // qDebug() << "=== dispatcher 收到消息 ===";
    // qDebug() << "type:" << head.type << ", len:" << head.len;
    //解析头部 判断消息类型
    switch (head.type) {
    case SERVICE_TYPE::STAFF_LOGIN:{
        emit sigDataToLogin(recvData);
        break;
    }
    case SERVICE_TYPE::DEPARTMENT_LIST:{
        emit sigDataToDept(recvData);
        break;
    }
    case SERVICE_TYPE::DOCTOR_LIST:{
        emit sigDataToDoc(recvData);
        break;
    }
    case SERVICE_TYPE::WEEK_SCHEDULE:{
        emit sigDataToSchedule(recvData);
        break;
    }
    case SERVICE_TYPE::Schedule_Doc:{
        emit sigDataToScheduleDoc(recvData);
        break;
    }
    case SERVICE_TYPE::CHECK_ADD_SCHEDULE:{
        emit sigDataToAddScheduleDlg(recvData);
        break;
    }
    case SERVICE_TYPE::CHECK_DEL_SCHEDULE:{
        emit sigDataToDelScheduleDlg(recvData);
        break;
    }
    case SERVICE_TYPE::MONTH_SCHEDULE:{
        emit sigDataToMonthSchedule(recvData);
        break;
    }
    case SERVICE_TYPE::AUTO_SCHEUDLE:{
        emit sigDataToAutoSchedule(recvData);
        break;
    }
    case SERVICE_TYPE::APPOINTMENT_LIST:
        emit sigDataToAppointmentList(recvData);
        break;
    case SERVICE_TYPE::APPOINTMENT_MISS:
        emit sigDataToAppointmentMiss(recvData);
        break;
    case SERVICE_TYPE::TONGUE_RECORD_DETAIL:
        emit sigDataToTongueRecordDetail(recvData);
        break;
    case SERVICE_TYPE::TONGUE_IMAGE_CHUNK:
        emit sigDataToTongueChunks(recvData);
        break;
    case SERVICE_TYPE::PATIENT_INFO:
        emit sigDataToPatientInfo(recvData);
        break;
    case SERVICE_TYPE::HISTORY_REPORT:
        emit sigDataToHistoryReport(recvData);
        break;
    case SERVICE_TYPE::SAVE_DIAGNOSIS:
        emit sigDataToSaveDiagnosis(recvData);
        break;
    case SERVICE_TYPE::GENERATE_REPORT:
        emit sigDataToGenerateReport(recvData);
        break;
    case SERVICE_TYPE::MODIFYPWD:
        emit sigDataToModifyPwd(recvData);
        break;
    case SERVICE_TYPE::DOCTOR_WEEK_SCHEDULE:
        emit sigDataToDoctorWeekSchedule(recvData);
        break;
    case SERVICE_TYPE::GET_LOG_LIST:
        emit sigDataToSysLogList(recvData);
        break;
    case SERVICE_TYPE::GET_DEPT_LIST:
        emit sigDataToDeptArchive(recvData);
        break;
    case SERVICE_TYPE::GET_CHIEF_DOCTORS:
        emit sigDataToChiefDoctors(recvData);
        break;
    case SERVICE_TYPE::GET_UNASSIGNED_DOCTORS:
        emit sigDataToUnassignedDoctors(recvData);
        break;
    case SERVICE_TYPE::ADD_DEPARTMENT:
        emit sigDataToAddDepartment(recvData);
        break;
    case SERVICE_TYPE::GET_DEPT_DETAIL:
        emit sigDataToDeptDetail(recvData);
        break;
    case SERVICE_TYPE::UPDATE_DEPT:
        emit sigDataToUpdateDept(recvData);
        break;
    case SERVICE_TYPE::GET_WORKLOAD_STATS:
        emit sigDataToWorkloadStats(recvData);
        break;
    case SERVICE_TYPE::GET_ALL_DOCTORS:
        emit sigDataToDoctorList(recvData);
        break;
    case SERVICE_TYPE::DELETE_DOCTOR:
        emit sigDataToDelDocArchive(recvData);
        break;
    case SERVICE_TYPE::GET_NEW_DOCTOR_ID:
        emit sigDataToNewDoctorId(recvData);
        break;
    case SERVICE_TYPE::UPDATE_DOCTOR:
        emit sigDataToUpdateDoctor(recvData);
        break;
    default:
        qDebug() << "未知消息类型: " << head.type;
        break;
    }
}






















