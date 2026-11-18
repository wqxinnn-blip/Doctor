#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QDate>

// 班次类型
enum class ShiftType {
    Morning = 1,
    Afternoon = 2,
    Night = 3
};

struct AppointmentInfo {
    int     appointmentId = -1;   // 预约ID
    int     patientId = -1;       // 患者ID
    QString patientName;          // 患者姓名
    //QString idCard;               // 身份证号
    QString workDate;             // 预约日期 "2026-10-11"
    quint8  shiftType = 1;        // 1=上午 2=下午 3=夜班
    int     tongueRecordId = -1;  // 关联舌苔报告ID（-1 无）
    int     status = 1;           // 1=已预约 2=已问诊 3=已失约 4=已取消
};

// 排班记录
struct ScheduleEntry {
    int scheduleId = -1;
    int doctorId = -1;
    QString doc_name;
    QString doc_title;
    QDate workdate;
    ShiftType shift = ShiftType::Morning;
};

// 医生信息
struct DoctorInfo {
    int staffId = -1;
    QString realName;
    QString title;
    int maxWeeklyShifts = 5;
};

// 科室信息
struct DepartmentInfo {
    int dept_id;
    QString deptName;
};


// ---- 患者基本信息（从预约列表带过来）----
struct PatientBriefInfo {
    int     patientId = -1;
    int     appointmentId = -1;
    int     tongue_record_id = -1;
    QString name;
    int     age = 0;
    QString phone;
    int     gender = 0;
};

struct PrescriptionItemInfo {
    QString medicineName;   // 药名
    QString dosage;         // 剂量
    QString usageMethod;    // 用法
};

// ---- 历史报告 ----
struct HistoryReport {
    int     recordId = -1;
    int     doctorId = -1;
    QString doctorName;
    QString visitDate;
    QString diagnosis;
    // 处方明细
    QList<PrescriptionItemInfo> items;

};

struct LocalDoctorData {
    QString workNo;
    QString name;
    QString dept;
    QString title;
    int age;
};

#endif // DATATYPES_H
