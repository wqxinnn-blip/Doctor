#ifndef AUTOSCHEDULE_H
#define AUTOSCHEDULE_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QMap>
#include "dataTypes.h"
#include "protocol.h"
#include "netconn.h"
#include "weekscheduleview.h"


// 自动排班结果
typedef struct{
    int totalAssigned; //成功排班数
    int totalConficts; //冲突排班数
    QStringList warnings; //警告信息列表
    QList<ScheduleEntry> entries; //生成的排班记录
}AutoScheduleResult;


class AutoSchedule : public QObject
{
    Q_OBJECT
public:
    explicit AutoSchedule(QObject *parent = nullptr);
    AutoScheduleResult autoScheduleWeek(int dept_id,const QList<DoctorInfo>& doctors,
                                        const QDate& weekStartDate,const QList<ScheduleEntry>& existing);
    void sendAutoSchedule(int dept_id, QList<ScheduleEntry> entries);
signals:

private:
    QList<ScheduleEntry> m_allSchedules; //周排班记录
    NetConn* m_netconn;
    WeekScheduleView *weekView;

    bool canAssign(int doc_id,const QMap<int,int>& weeklyShiftCount,QDate currentDate,ShiftType shift,
                   const QList<ScheduleEntry>& existing,const QList<DoctorInfo>& doctors);

    QString shiftToChinese(ShiftType shift);
private slots:
};

#endif // AUTOSCHEDULE_H
