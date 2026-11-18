#ifndef SCHEDULEMANAGEPAGE_H
#define SCHEDULEMANAGEPAGE_H

#include <QWidget>
#include <QTabWidget>
#include <QDate>
#include "weekscheduleview.h"
#include "monthscheduleview.h"
#include "editscheduledialog.h"
#include "previewscheduledialog.h"
#include "netconn.h"
#include "autoschedule.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ScheduleManagePage; }
QT_END_NAMESPACE

class ScheduleManagePage : public QWidget
{
    Q_OBJECT

public:
    explicit ScheduleManagePage(QWidget *parent = nullptr);
    ~ScheduleManagePage();

    void onEnter();

private slots:
    // Tab 切换槽函数
    void onTabChanged(int index);

    void onEditScheduleByData(const QDate& date,ShiftType shift);
    void onMonthEditRequested(const QDate& date);

    void on_combo_depart_currentIndexChanged(int index); // 选择科室

    void onDepartmentRecieved(const QByteArray& body); // 科室列表数据包响应
    void onDoctorListReceived(const QByteArray& body); // 医生列表数据包响应
    void onAutoScheduleReceived(const QByteArray& body); // 自动排班数据包响应

    // 功能区
    void onPrevWeek();
    void onNextWeek();
    void onPrevMonth();
    void onNextMonth();
    void onToday();
    void onAutoSchedule();

    //void onCopyLastWeek();
private:
    Ui::ScheduleManagePage *ui;
    WeekScheduleView *m_weekView = nullptr;   // 周视图
    MonthScheduleView *m_monthView = nullptr; // 月视图
    NetConn* m_netconn = nullptr;

    QString m_currentDept;
    QDate m_currentDate;
    int m_currentDeptId = -1;

    QList<DoctorInfo> m_currentDoctors;
    bool m_waitingDoctorsForAuto = false;

    void setUI();
    void int_qss();

    // 请求函数
    void requestDoctors(int dept_id);
    void requestDepartments();
    void refreshCurrentView();
    void runAutoSchedule();
};

#endif
