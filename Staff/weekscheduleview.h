#ifndef WEEKSCHEDULEVIEW_H
#define WEEKSCHEDULEVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QDate>
#include "dataTypes.h"
#include "protocol.h"
#include "netconn.h"

#define ROW_MORNING 0
#define ROW_AFTERNOON 1
#define ROW_NIGHT 2

class WeekScheduleView : public QWidget
{
    Q_OBJECT
public:
    explicit WeekScheduleView(QWidget *parent = nullptr);

    void onEnter();                          // 进入界面时调
    void setWeek(const QDate& baseDate);     // 切换周
    void setDepartment(int deptId);

    void refreshData();
    QList<ScheduleEntry> allSchedules() const;

signals:
    void editScheduleRequested(const QDate& date,ShiftType shift);

private slots:
    void onCellDoubledClicked(int row,int col); // 双击单元格
    void onScheduleDataRecieved(const QByteArray& body); // 排班数据包接收

private:
    // 初始化表格结构与样式
    void initTable();
    void refresh();
    void updateHeader();
    void showLoading();

    // 根据基准日期计算当周周一到周日的完整日期
    QList<QDate> calcWeekDates(const QDate &baseDate);
    // 请求本周排班
    void requestWeekSchedule(const QDate& mondy,const QDate& sunday,int deptId);

// 成员变量
    QTableWidget *m_table;
    QList<QDate> m_dates;
    QList<ScheduleEntry> m_allSchedules;

    NetConn *m_netconn;

    // 当前科室ID  选择科室后直接赋值
    int m_departmentId;

    bool m_scheduleReceived = false;
    bool m_doctorsReceived  = false;

    // 固定时间段行
    const QStringList m_timeSlots = { "上午", "下午", "夜班" };
};

#endif // WEEKSCHEDULEVIEW_H
