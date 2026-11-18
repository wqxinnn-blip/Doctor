#include "weekscheduleview.h"
#include "winmanager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QIcon>

WeekScheduleView::WeekScheduleView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_netconn = WinManager::getInstance()->getNetConn();

    m_table = new QTableWidget(this);
    initTable();
    mainLayout->addWidget(m_table);
}


void WeekScheduleView::onEnter()
{
    // 1. 保存周信息  切回该界面时能保持该周数据
    if (m_dates.size() != 7) {
        m_dates = calcWeekDates(QDate::currentDate());
    }
    // 添加表头
    updateHeader();
    // 2. 初始显示"--"
    showLoading();
    //2、发送请求 本周排班 + 医生列表
    requestWeekSchedule(m_dates[0],m_dates[6],m_departmentId);
}

// ================================================================================================
// ====================================== 初始化 ================================================
// ================================================================================================

void WeekScheduleView::initTable()
{
    // 3行（时间段） × 7列（周一到周日）
    m_table->setRowCount(3);
    m_table->setColumnCount(7);

    // 行表头
    m_table->setVerticalHeaderLabels(m_timeSlots);
    // 水平表头初始占位
    m_table->setHorizontalHeaderLabels({ "周一", "周二", "周三", "周四", "周五", "周六", "周日" });

    // 表头设置
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 列宽均分
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);  //居中
   m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setFixedWidth(60);                            // 行表头宽度

    // 表格交互属性
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);             // 禁止编辑
    m_table->setGridStyle(Qt::SolidLine);

    // 绑定信号
    connect(m_table,&QTableWidget::cellDoubleClicked,this,&WeekScheduleView::onCellDoubledClicked);
    connect(m_netconn,&NetConn::sigDataToSchedule,this,&WeekScheduleView::onScheduleDataRecieved);
    // connect(m_netconn,&NetConn::sigDataToDoc,this,&WeekScheduleView::onDoctorsReceived);
}

// 初始加载表格
void WeekScheduleView::showLoading()
{
    m_table->clearContents();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        for (int c = 0; c < m_table->columnCount(); ++c) {
            QTableWidgetItem* item = new QTableWidgetItem("--");
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QColor("#909399"));
            m_table->setItem(r, c, item);
        }
    }
}


// ================================================================================================
// ====================================== 工具函数 ================================================
// ================================================================================================
// 计算 baseDate 所在那一周的 7 天，从周一到周日
QList<QDate> WeekScheduleView::calcWeekDates(const QDate &baseDate)
{
    QList<QDate> dates;
    // Qt 中周一 = 1，周日 = 7，计算当周周一
    int offset = 1 - baseDate.dayOfWeek();
    QDate monday = baseDate.addDays(offset);

    // 添加本周日期
    for (int i = 0; i < 7; ++i) {
        dates.append(monday.addDays(i));
    }
    return dates;
}

void WeekScheduleView::setWeek(const QDate &baseDate)
{

    m_dates = calcWeekDates(baseDate);
    updateHeader();

    showLoading();
    requestWeekSchedule(m_dates[0], m_dates[6], m_departmentId);
}

void WeekScheduleView::setDepartment(int deptId)
{
    m_departmentId = deptId;
}

void WeekScheduleView::refreshData()
{

    if (m_dates.size() != 7) {
        m_dates = calcWeekDates(QDate::currentDate());
        updateHeader();
    }
    requestWeekSchedule(m_dates[0], m_dates[6], m_departmentId);
}



void WeekScheduleView::updateHeader()
{
    if (m_dates.size() != 7) return;

    QStringList headers;
    const QStringList weekNames = { "周一","周二","周三","周四","周五","周六","周日" };
    for (int i = 0; i < 7; ++i) {
        QString dateStr = m_dates[i].toString("MM-dd");
        QString suffix  = (i >= 5) ? " *" : "";
        headers.append(QString("%1\n%2%3").arg(weekNames[i], dateStr, suffix));
    }
    m_table->setHorizontalHeaderLabels(headers);
}


void WeekScheduleView::refresh()
{
    showLoading();
    // 填充数据
    for(const ScheduleEntry& s : std::as_const(m_allSchedules))
    {
        // 列  通过日期判断
        int col = m_dates.indexOf(s.workdate); // 1~7 即 周一到周日
        //qDebug()<<"col:"<<col;
        if(col<0) continue;

        // 行 通过班次判断
        int row = static_cast<int>(s.shift)-1;
        //qDebug()<<"row:"<<row;
        if (row < 0 || row > 2) continue;

        // 填入单元格
        QTableWidgetItem* item = m_table->item(row,col);
        if(!item){
            item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row,col,item);
        }
        item->setText(QString("%1(%2)").arg(s.doc_name,s.doc_title));
        item->setForeground(QColor("#303133"));
    }
}

QList<ScheduleEntry> WeekScheduleView::allSchedules() const
{
    return m_allSchedules;
}


// ================================================================================================
// ====================================== 网络请求 ================================================
// ================================================================================================

void WeekScheduleView::requestWeekSchedule(const QDate &mondy, const QDate &sunday, int deptId)
{
    //1、填入数据
    WEEK_SCHEDULE_REQ req = {};
    QDate newStart = mondy.addDays(-1);
    QDate newEnd = sunday;
    strncpy(req.monday,newStart.toString("yyyy-MM-dd").toUtf8().constData(),10);
    strncpy(req.sunday,newEnd.toString("yyyy-MM-dd").toUtf8().constData(),10);
    req.deptId = deptId;

    //2、打包数据
    HEAD head = {SERVICE_TYPE::WEEK_SCHEDULE,sizeof(WEEK_SCHEDULE_REQ)};
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head),sizeof(head));
    packet.append(reinterpret_cast<const char*>(&req),sizeof(req));

    //3、发送数据
    m_netconn->sendPacket(packet);
}


// ================================================================================================
// ====================================== 按钮槽函数 ================================================
// ================================================================================================

// 双击单元格时触发
void WeekScheduleView::onCellDoubledClicked(int row, int col)
{
    // 1、检查 行、列
    if(col < 0 || col >= m_dates.size()) return;

    //2、根据行索引确定班次
    ShiftType shift;
    switch (row) {
    case ROW_MORNING: shift = ShiftType::Morning;           break;
    case ROW_AFTERNOON: shift = ShiftType::Afternoon;       break;
    case ROW_NIGHT: shift = ShiftType::Night;               break;
    default: return;
    }

    //3、发出编辑请求信号，由schedulemanagepage 处理 --》打开编辑弹窗
    emit editScheduleRequested(m_dates[col],shift);
}


// ================================================================================================
// ====================================== 网络响应================================================
// ================================================================================================

void WeekScheduleView::onScheduleDataRecieved(const QByteArray& body)
{
    if (body.size() % sizeof(WEEK_SCHEDULE_RESP) != 0) {
        qWarning() << "排班数据包长度异常:" << body.size();
        return;
    }

    int count = body.size() / sizeof(WEEK_SCHEDULE_RESP);

    QList<ScheduleEntry> list;
    const char* p = body.constData();
    for(int i=0;i<count;i++){
        const auto* item = reinterpret_cast<const WEEK_SCHEDULE_RESP*>(p);
        ScheduleEntry d;
        d.scheduleId = item->schedule_id;
        d.doctorId = item->doc_id;
        d.doc_name = QString::fromUtf8(item->doc_name);
        d.doc_title = QString::fromUtf8(item->doc_title);
        d.workdate = QDate::fromString(QString::fromUtf8(item->workdate), "yyyy-MM-dd");
        //qDebug()<<"医生:"<<d.doc_name<<",职称："<<d.doc_title<<"，workdate："<<d.workdate;

        d.shift = static_cast<ShiftType>(item->shift);
        list.append(d);
        p+=sizeof(WEEK_SCHEDULE_RESP);
    }

    m_allSchedules = list;
    m_scheduleReceived = true;

    qDebug()<<"排班数据接收："<<list.size()<<"条";
    refresh();
}




