#ifndef EDITSCHEDULEDIALOG_H
#define EDITSCHEDULEDIALOG_H

#include <QDialog>
#include <QDate>
#include <QFile>
#include <QSet>
#include "dataTypes.h"
#include "../netconn.h"

namespace Ui {
class EditScheduleDialog;
}

class EditScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    // 周视图构造
    explicit EditScheduleDialog(const QDate& date,ShiftType shift,int departmentId,QWidget *parent = nullptr);
    // 月视图构造
    EditScheduleDialog(const QDate& date,int departmentId,QWidget *parent = nullptr);
    ~EditScheduleDialog();

    bool isDataChanged() const;

signals:
    // 排班更新 通知父窗口刷新
    void scheduleChanged();

private slots:
    // 添加/删除医生
    void onAddDoctor();  // 执行 endAddSchedule
    void onRemoveDoctor(); // 执行 sendDelSchedule

    // 医生信息/排班医生 响应
    void onDoctorsReceived(const QByteArray& body);
    void onScheduleDocsReceived(const QByteArray& body);

    // 添加/删除医生响应
    void onAddResponse(const QByteArray& body);
    void onDelResponse(const QByteArray& body);

private:
    Ui::EditScheduleDialog *ui;

    QDate m_date;// 当前周日期
    ShiftType m_shift;// 当前班次
    int m_departmentId;// 当前科室ID
    bool m_fixShift;// 周视图 true  固定班次信息   月视图 false  可选择班次


    NetConn *m_netconn;
    static const QStringList m_timeSlots;

    QList<DoctorInfo> m_doctors;// 科室医生列表
    int m_pendingStaffId; // 待添加医生的员工 ID
    bool m_dataChanged = false; //数据发生变化  即刷新
    QSet<int> m_addDocId; // 保存已排班医生的id  用于从添加下拉框中删除

    void init_qss();
    void loadCurrentSchedules();  //加载本班次数据（发送请求）

    void tryFillDoctorCombo(); //添加本班次信息
    int shiftToRow();   // 转换班次为行索引

    // 添加排班和删除已排班医生
    void requestScheduledDoctors();
    void requestDoctors();
    void sendAddSchedule(bool force);
    void sendDelSchedule(int doctorId);

};

#endif // EDITSCHEDULEDIALOG_H
