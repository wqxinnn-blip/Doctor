#ifndef MONTHSCHEDULEVIEW_H
#define MONTHSCHEDULEVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QDate>
#include "dataTypes.h"
#include "netconn.h"

class MonthScheduleView : public QWidget
{
    Q_OBJECT
public:
    explicit MonthScheduleView(QWidget *parent = nullptr);

    // 传入科室和基准日期，刷新整月排班
    void setScheduleData(const QString &dept, const QDate &date);

    void setDepartment(int deptId);
    void refreshData();

signals:
    void editRequested(const QDate& date);   // 双击某天编辑

private slots:
    void onScheduleDataReceived(const QByteArray& body);

private:
    void initTable(); // 初始化月历表格
    void fillMonthCalendar(const QDate &baseDate); // 填充日期与排班数据（对接数据库时修改此函数即可）
    void requestMonthSchedule(const QDate& firstDay, const QDate& lastDay);
    void updateCell(int row, int col, const QDate& date);   // 更新单格


    NetConn* m_netconn = nullptr;
    QTableWidget* m_table = nullptr;
    QList<QDate> m_dates;                   // 42 天的日期（含上下月补齐）
    QList<ScheduleEntry> m_allSchedules;    // 本月所有排班

    int m_departmentId = -1;
    QDate m_currentMonth;                   // 当前显示的月
    const QStringList m_weekHeaders = { "周一", "周二", "周三", "周四", "周五", "周六", "周日" };
};

#endif // MONTHSCHEDULEVIEW_H
