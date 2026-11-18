#include "schedulemanagepage.h"
#include "ui_schedulemanagepage.h"
#include "winmanager.h"

ScheduleManagePage::ScheduleManagePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScheduleManagePage)
{
    ui->setupUi(this);

    // 初始化
    int_qss();
    setUI();
    m_currentDate = QDate::currentDate();
    m_netconn = WinManager::getInstance()->getNetConn();

    // 绑定信号
    // 切换周/月视图
    connect(ui->scheduleTabWidget, &QTabWidget::currentChanged, this, &ScheduleManagePage::onTabChanged);
    // 获取科室信息
    connect(m_netconn, &NetConn::sigDataToDept, this, &ScheduleManagePage::onDepartmentRecieved);
    // 显示弹窗
    connect(m_weekView, &WeekScheduleView::editScheduleRequested, this, &ScheduleManagePage::onEditScheduleByData);
    connect(m_monthView, &MonthScheduleView::editRequested,this, &ScheduleManagePage::onMonthEditRequested);
    // 网络数据响应
    connect(m_netconn,&NetConn::sigDataToAutoSchedule,this,&ScheduleManagePage::onAutoScheduleReceived);
    connect(m_netconn, &NetConn::sigDataToDoc,this, &ScheduleManagePage::onDoctorListReceived);
    // 按钮槽
    connect(ui->btn_lastweek,  &QToolButton::clicked, this, &ScheduleManagePage::onPrevWeek);
    connect(ui->btn_nextweek,  &QToolButton::clicked, this, &ScheduleManagePage::onNextWeek);
    connect(ui->btn_lastmonth, &QToolButton::clicked, this, &ScheduleManagePage::onPrevMonth);
    connect(ui->btn_nextmonth, &QToolButton::clicked, this, &ScheduleManagePage::onNextMonth);
    connect(ui->btn_today,     &QToolButton::clicked, this, &ScheduleManagePage::onToday);
    connect(ui->btn_auto,      &QToolButton::clicked, this, &ScheduleManagePage::onAutoSchedule);
    //connect(ui->btn_copy,      &QToolButton::clicked, this, &ScheduleManagePage::onCopyLastWeek);
}

ScheduleManagePage::~ScheduleManagePage()
{
    delete ui;
}

void ScheduleManagePage::onEnter()
{
    // 请求科室列表
    requestDepartments();
}


// ================================================================================================
// ====================================== 视图槽/科室下拉框选择 ===================================
// ================================================================================================

// Tab 切换时刷新对应视图数据
void ScheduleManagePage::onTabChanged(int index)
{
    if (m_currentDeptId <= 0) return;

    // 0 -- 周视图  1 -- 月视图
    if (index == 0) {
        m_weekView->onEnter();
    } else {
        m_monthView->setScheduleData(m_currentDept, m_currentDate);
    }
}

// 切换科室时  刷新当前视图数据
void ScheduleManagePage::on_combo_depart_currentIndexChanged(int index)
{
    if(index < 0) return;

    int deptId = ui->combo_depart->currentData().toInt();
    if(deptId <= 0){
        m_currentDeptId = -1;
        return;
    }
    //查询本科室医生列表
    m_currentDeptId = deptId;
    m_currentDept = ui->combo_depart->currentText();

    // 把 deptId 传给周/月视图
    m_weekView->setDepartment(deptId);
    m_monthView->setDepartment(deptId);

    // 当前 Tab 刷新
    int curIndex = ui->scheduleTabWidget->currentIndex();
    if(curIndex == 0){
        m_weekView->onEnter();
    }
    else{
        m_monthView->setScheduleData(m_currentDept,m_currentDate);
    }
}


// ================================================================================================
// ====================================== 初始化 ===================================
// ================================================================================================
void ScheduleManagePage::setUI()
{
    // 顶部工具栏固定高度
    ui->widget->setFixedHeight(56);

    // 设置垂直布局拉伸因子：工具栏不拉伸，Tab自动填满剩余高度
    ui->verticalLayout->setStretch(0, 0);
    ui->verticalLayout->setStretch(1, 1);

    // 下拉框尺寸
    ui->combo_depart->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 初始化Tab页面
    ui->scheduleTabWidget->clear();
    m_weekView = new WeekScheduleView(this);
    m_monthView = new MonthScheduleView(this);
    ui->scheduleTabWidget->addTab(m_weekView, "周排班视图");
    ui->scheduleTabWidget->addTab(m_monthView, "月排班视图");
    ui->scheduleTabWidget->setTabIcon(0, QIcon(":/image/schedule_img/schedule.png"));
    ui->scheduleTabWidget->setTabIcon(1, QIcon(":/image/schedule_img/schedule.png"));

}

void ScheduleManagePage::int_qss()
{
    //1、打开文件
    QFile file(":/qss/schedule.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}


// ================================================================================================
// ====================================== 弹窗显示  周视图/月视图 ===================================
// ================================================================================================
void ScheduleManagePage::onEditScheduleByData(const QDate &date, ShiftType shift)
{

    if (m_currentDeptId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择科室");
        return;
    }

    EditScheduleDialog dlg(date,shift,m_currentDeptId,this);
    dlg.exec();
    //刷新表格
    if (dlg.isDataChanged()) {
        m_weekView->refreshData();
    }
}

void ScheduleManagePage::onMonthEditRequested(const QDate &date)
{
    if (m_currentDeptId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择科室");
        return;
    }

    EditScheduleDialog dlg(date, m_currentDeptId, this);
    dlg.exec();
    if (dlg.isDataChanged()) {
        m_monthView->refreshData();
    }
}


// ================================================================================================
// ====================================== 数据请求 ===================================
// ================================================================================================
void ScheduleManagePage::requestDoctors(int dept_id)
{
    DOCTOR_LIST_REQ req = {};
    req.dept_id = dept_id;

    HEAD head = { SERVICE_TYPE::DOCTOR_LIST, sizeof(DOCTOR_LIST_REQ) };

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req),  sizeof(DOCTOR_LIST_REQ));
    m_netconn->sendPacket(packet);
}

void ScheduleManagePage::requestDepartments()
{
    DEPARTMENT_LIST_REQ req = {};

    HEAD head = {SERVICE_TYPE::DEPARTMENT_LIST,sizeof(DEPARTMENT_LIST_REQ)};

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head),sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req),sizeof(DEPARTMENT_LIST_REQ));

    m_netconn->sendPacket(packet);
}


// ================================================================================================
// ====================================== 网络包响应 ==============================================
// ================================================================================================

void ScheduleManagePage::onDepartmentRecieved(const QByteArray& body)
{
    // 解析数据包
    int count = body.size() / sizeof(DEPARTMENT_LIST_RESP);
    //qDebug()<<"科室的数量："<<count;

    QList<DepartmentInfo> list;
    const char* p = body.constData();
    for(int i=0;i<count;i++){
        const auto* item = reinterpret_cast<const DEPARTMENT_LIST_RESP*>(p);
        DepartmentInfo d;
        d.dept_id = item->dept_id;
        d.deptName = QString::fromUtf8(item->dept_name);
        list.append(d);
        p+=sizeof(DEPARTMENT_LIST_RESP);
    }

    ui->combo_depart->clear();
    ui->combo_depart->addItem("请选择",-1);

    // 逐个添加
    for(const DepartmentInfo& d : list){
        ui->combo_depart->addItem(d.deptName,d.dept_id);
    }

    // 默认选中“请选择”
    ui->combo_depart->setCurrentIndex(0);
}

void ScheduleManagePage::onDoctorListReceived(const QByteArray &body)
{
    int count = body.size() / sizeof(DOCTOR_LIST_RESP);
    m_currentDoctors.clear();

    const char* p = body.constData();
    for (int i = 0; i < count; ++i) {
        const auto* item = reinterpret_cast<const DOCTOR_LIST_RESP*>(p);
        DoctorInfo d;
        d.staffId        = item->staff_id;
        d.realName          = QString::fromUtf8(item->real_name);
        d.maxWeeklyShifts = item->max_weekly_shifts;
        m_currentDoctors.append(d);
        p += sizeof(DOCTOR_LIST_RESP);
    }

    if (m_waitingDoctorsForAuto) {
        m_waitingDoctorsForAuto = false;
        runAutoSchedule();
    }
}

void ScheduleManagePage::onAutoScheduleReceived(const QByteArray &body)
{

    if (body.size() < (int)sizeof(AUTO_SCHEDULE_RESP)) {
        QMessageBox::warning(this, "自动排班", "响应数据不完整");
        return;
    }

    AUTO_SCHEDULE_RESP resp = {};
    memcpy(&resp, body.constData(), sizeof(AUTO_SCHEDULE_RESP));

    int len = strnlen(resp.msg, sizeof(resp.msg));
    QString msg = QString::fromUtf8(resp.msg, len);
    if (msg.isEmpty()) msg = "无消息";

    if (resp.code == 0) {
        if (m_weekView)  m_weekView->refreshData();
        if (m_monthView) m_monthView->refreshData();

        QMessageBox::information(this, "自动排班",
                                 QString("排班完成\n%1").arg(msg));
    }
    else {
        QMessageBox::warning(this, "自动排班",
                             QString("排班失败\n%1").arg(msg));
    }
    // 刷新周视图
    m_weekView->refreshData();
}



// ================================================================================================
// ====================================== 按钮槽函数 ================================================
// ================================================================================================

void ScheduleManagePage::onPrevWeek()
{
    m_currentDate = m_currentDate.addDays(-7);
    m_weekView->setWeek(m_currentDate);
}

void ScheduleManagePage::onNextWeek()
{
    m_currentDate = m_currentDate.addDays(7);
    m_weekView->setWeek(m_currentDate);
}

void ScheduleManagePage::onPrevMonth()
{
    m_currentDate = m_currentDate.addMonths(-1);
    refreshCurrentView();
}

void ScheduleManagePage::onNextMonth()
{
    m_currentDate = m_currentDate.addMonths(1);
    refreshCurrentView();
}

void ScheduleManagePage::onToday()
{
    m_currentDate = QDate::currentDate();
    refreshCurrentView();
}

void ScheduleManagePage::onAutoSchedule()
{
    if (m_currentDeptId <= 0) {
        QMessageBox::information(this, "提示", "请先选择科室");
        return;
    }

    m_waitingDoctorsForAuto = true;
    requestDoctors(m_currentDeptId);
}


// ================================================================================================
// ====================================== 刷新视图/自动排班 ================================================
// ================================================================================================

void ScheduleManagePage::refreshCurrentView()
{
    if (m_currentDeptId <= 0) {
        QMessageBox::information(this, "提示", "请先选择科室");
        return;
    }

    int curIndex = ui->scheduleTabWidget->currentIndex();
    if (curIndex == 0) {
        m_weekView->setWeek(m_currentDate);
    } else {
        m_monthView->setScheduleData(m_currentDept, m_currentDate);
    }
}

void ScheduleManagePage::runAutoSchedule()
{
    if (m_currentDoctors.isEmpty()) {
        QMessageBox::warning(this, "提示", "本科室没有医生，无法自动排班");
        return;
    }

    // 1. 计算当前视图 周一 周日
    QDate weekStart = m_currentDate.addDays(-(m_currentDate.dayOfWeek() - 1));
    QDate weekEnd   = weekStart.addDays(6);
    QDate today     = QDate::currentDate(); // 今日日期
    if (weekEnd < today) {
        QMessageBox::warning(this, "提示",
                             "无法对过去的日期进行自动排班！\n请先点击【今日】或切换到本周/下周视图。");
        return;
    }
    QList<ScheduleEntry> existing = m_weekView->allSchedules();
    //qDebug() << "当前视图周一：" << weekStart << "，今天：" << today;
    AutoSchedule as;
    AutoScheduleResult result = as.autoScheduleWeek(m_currentDeptId, m_currentDoctors, weekStart,existing);

    // 1、全部失败
    if (result.entries.isEmpty()) {
        QString msg = "未能生成任何排班\n";
        if (result.warnings.isEmpty()) {
            msg += "可能本周剩余时间已过（如今天已是周日），或所有时段均已排满。";
        } else {
            for (const QString& w : result.warnings) msg += w + "\n";
        }
        QMessageBox::information(this, "自动排班", msg);
        return;
    }
    PreviewScheduleDialog dlg(result.entries, m_currentDoctors, result.warnings, this);
    if (dlg.exec() == QDialog::Accepted) {
        // 用户在预览窗口点击了“确认上传”
        QList<ScheduleEntry> finalEntries = dlg.getFinalEntries();

        if (finalEntries.isEmpty()) {
            QMessageBox::warning(this, "提示", "您删除了所有的排班记录，取消上传。");
            return;
        }
        // 批量上传被保留下来的记录
        as.sendAutoSchedule(m_currentDeptId, finalEntries);
    }
}

