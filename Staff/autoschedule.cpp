#include "autoschedule.h"
#include "winmanager.h"

AutoSchedule::AutoSchedule(QObject *parent)
    : QObject{parent}
{
    m_netconn = WinManager::getInstance()->getNetConn();
}

AutoScheduleResult AutoSchedule::autoScheduleWeek(int dept_id, const QList<DoctorInfo> &doctors,
                                                  const QDate &weekStartDate,const QList<ScheduleEntry>& existing)
{
    qDebug()<<"进入自动排班算法";
    // 创建结果对象，存储排班信息，警告和统计等
    AutoScheduleResult result = {};
    // 判断当前科室是否有医生可排班
    if(doctors.isEmpty()){
        result.warnings.append(QStringLiteral("该科室没有医生，无法排班！"));
        return result;
    }
    // 获取本周已有排班记录 存入list
    QList<ScheduleEntry> allExisting = existing;
    //统计每位医生本周已排班总次数
    QMap<int,int> weeklyShiftCount; // 键：医生ID 值:本周已排班数
    // 查询数据库
    for(const DoctorInfo& doc : doctors){
        // 初始化 排班数map
        weeklyShiftCount[doc.staffId] = 0;
    }
    // 循环遍历排班记录 通过doc_id 关联  累计排班数
    for(const ScheduleEntry& e : allExisting){
        if(weeklyShiftCount.contains(e.doctorId)){
            weeklyShiftCount[e.doctorId]++;
        }
    }
    // 定义每天需要分配的班次类型 List
    QList<ShiftType> shifts = {ShiftType::Morning,ShiftType::Afternoon,ShiftType::Night};
    // 轮转起始索引
    // 早：0 下午：1/3 晚:2/3
    int morningIdx = 0;
    int afternoonIdx = doctors.size()/3;
    int nightIdx = doctors.size() *2 /3;
    //三层遍历
    //最外层： 循环遍历一周七天
    // 获取当前日期 --- 只对未来进行自动排班（不包括今日）
    QDate today = QDate::currentDate();

    for(int dayOffset = 0;dayOffset < 7; ++dayOffset){
        // 获取当前日期
        QDate currentdate = weekStartDate.addDays(dayOffset); //返回偏移的新日期 不修改原日期
        if (currentdate <= today) {
            continue;
        }
        //中间层：对当天每种排班尝试分配
        // 检查该班次是否已经被已有排班占用（即手动排班添加过的）
        // 通过循环处理 排班记录   用标记位判断
        for(ShiftType shift:shifts){
            bool shiftAlreadyFilled = false;
            for(const ScheduleEntry& e : allExisting){
                if(e.shift == shift && e.workdate == currentdate){
                    shiftAlreadyFilled = true;
                    break;
                }
            }
            //true：已存在 跳过该班次   false：继续处理
            if(shiftAlreadyFilled) continue;
            // 根据班次类型选择轮转起始索引
            int startIdx;
            switch(shift){
            case ShiftType::Morning: startIdx = morningIdx; break;
            case ShiftType::Afternoon: startIdx = afternoonIdx; break;
            case ShiftType::Night: startIdx = nightIdx; break;
            }
            //轮转查找可分配医生
            int selectedDoctorId = -1;
            for(int i=0;i<doctors.size();i++){
                int idx = (startIdx+i) % doctors.size();
                int docId = doctors[idx].staffId;

                //调用约束函数
                if(canAssign(docId,weeklyShiftCount,currentdate,shift,allExisting,doctors)){
                    selectedDoctorId = docId;
                    // 更新轮转指针，下一次搜索从下一个医生开始
                    switch(shift){
                    case ShiftType::Morning: morningIdx = (idx+1) % doctors.size(); break;
                    case ShiftType::Afternoon: afternoonIdx = (idx+1) % doctors.size(); break;
                    case ShiftType::Night: nightIdx = (idx+1) % doctors.size(); break;
                    }
                    break;
                }
            }
            // 未找到可分配医生，记录冲突警告
            if(selectedDoctorId == -1){
                result.totalConficts++;
                result.warnings.append(
                    QStringLiteral("%1 %2:无可分配医生（所有医生已达上限或冲突)")
                        .arg(currentdate.toString("MM-dd"))
                        .arg(shiftToChinese(shift)));
                continue;
            }
            //将选中的医生暂存到本地
            ScheduleEntry newEntry = {};
            newEntry.scheduleId = -1;
            newEntry.doctorId = selectedDoctorId;
            newEntry.workdate = currentdate;
            newEntry.shift = shift;

            result.entries.append(newEntry);
            result.totalAssigned++;

            //更新本地状态
            //assignedToday.insert(selectedDoctorId);
            weeklyShiftCount[selectedDoctorId]++;
            //更新existing
            allExisting.append(newEntry);
        }
    }
    return result;
}

// 医生id、本周排班数、待排班日期、班次、排班记录、医生记录和周起始日期
bool AutoSchedule::canAssign(int doc_id,const QMap<int,int>& weeklyShiftCount,QDate currentDate,ShiftType shift,
                             const QList<ScheduleEntry>& existing,const QList<DoctorInfo>& doctors)
{
    //  1、同一天不可以两班
    for(const ScheduleEntry& e : existing){
        //存在该医生在当日有排班
        if(e.doctorId == doc_id && e.workdate == currentDate){
            return false;
        }
    }
    //  2、单周排班不超过上限
    int maxShifts = 5;
    for(const auto& d : doctors){
        if(d.staffId == doc_id){
            maxShifts = d.maxWeeklyShifts;
            break;
        }
    }
    if(weeklyShiftCount.value(doc_id,0) >= maxShifts) return false;
    //  3、不能安排连续夜班
    if(shift == ShiftType::Night){
        QDate prev = currentDate.addDays(-1);

        for(const ScheduleEntry& e : existing){
            if(e.doctorId == doc_id && e.workdate == prev && e.shift == shift){
                return false;
            }
        }
    }
    return true;
}

QString AutoSchedule::shiftToChinese(ShiftType shift)
{
    QString name;
    switch(shift){
    case ShiftType::Morning: name = "早班"; break;
    case ShiftType::Afternoon: name = "下午班"; break;
    case ShiftType::Night: name = "晚班"; break;
    }
    return name;
}

void AutoSchedule::sendAutoSchedule(int dept_id, QList<ScheduleEntry> entries)
{
    QByteArray body;
    int count = entries.size();

    body.append(reinterpret_cast<const char*>(&dept_id),sizeof(dept_id));
    body.append(reinterpret_cast<const char*>(&count),sizeof(count));

    for(const auto& e : entries){
        AUTO_SCHEDULE_ITEM item = {};
        item.staff_id = e.doctorId;
        item.shift = static_cast<uint8_t>(e.shift);

        QByteArray dateBytes = e.workdate.toString("yyyy-MM-dd").toUtf8();
        int copyLen = qMin(dateBytes.size(), (int)sizeof(item.workdate) - 1);
        memcpy(item.workdate, dateBytes.constData(), copyLen);
        item.workdate[copyLen] = '\0';

        body.append(reinterpret_cast<const char*>(&item),sizeof(item));
    }
    HEAD head = {SERVICE_TYPE::AUTO_SCHEUDLE, static_cast<int>(body.size())};

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head),sizeof(HEAD));
    packet.append(body);

    m_netconn->sendPacket(packet);
}




