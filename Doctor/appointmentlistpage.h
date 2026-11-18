#ifndef APPOINTMENTLISTPAGE_H
#define APPOINTMENTLISTPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include <QLabel>
#include <QDate>
#include "../netconn.h"
#include "../dataTypes.h"

namespace Ui {
class AppointmentListPage;
}

class AppointmentListPage : public QWidget
{
    Q_OBJECT

public:
    explicit AppointmentListPage(QWidget *parent = nullptr);
    ~AppointmentListPage();

    void onEnter();   // 加载当日预约
    void onLeave();   // 清理

signals:
    // 点"问诊" → 通知主窗口跳转到"诊疗档案管理"
    void startConsultation(int appointmentId, int patientId,int tongue_record_id);
private slots:
    void onPrevPage();
    void onNextPage();
    void onRefreshTimerTimeout();

    // 网络响应
    void onAppointmentListReceived(const QByteArray& body);
    void onAppointmentMissResp(const QByteArray& body);   

private:

    void setupUi();
    void setupTable();
    void requestTodayAppointments();
    static int getShiftByTime(const QTime &now);

    void fillTable(const QList<AppointmentInfo>& list);
    void updatePagination();

    NetConn* m_netconn = nullptr;

    QTableWidget* m_table = nullptr;
    QPushButton*  m_btnPrev = nullptr;
    QPushButton*  m_btnNext = nullptr;
    QLabel*       m_labPageInfo = nullptr;

    // 分页
    int m_currentPage = 1;
    int m_pageSize = 10;
    int m_totalPage = 1;

    // 本医生该时段所有患者预约列表
    QList<AppointmentInfo> m_appointments;
    // 当前报告的预约ID
    int m_pendingAppointmentId  = -1;
    // 定时器  30s 刷新一次列表
    QTimer *m_shiftTimer = nullptr;
    int m_currentShift = -1;
    bool m_requesting = false;
};

#endif // APPOINTMENTLISTPAGE_H
