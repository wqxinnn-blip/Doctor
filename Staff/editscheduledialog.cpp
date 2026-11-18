#include "editscheduledialog.h"
#include "ui_editscheduledialog.h"
#include "winmanager.h"

const QStringList EditScheduleDialog::m_timeSlots = { "上午", "下午", "夜班" };

// 周视图构造
EditScheduleDialog::EditScheduleDialog(const QDate& date,ShiftType shift,int departmentId,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditScheduleDialog)
    , m_date(date)
    , m_shift(shift)
    , m_departmentId(departmentId)
    , m_fixShift(true)
{
    ui->setupUi(this);
    setWindowTitle("编辑排班");

    init_qss();
    m_netconn = WinManager::getInstance()->getNetConn();
    //标题
    ui->lab_date_2->setText(QString("📅 %1").arg(m_date.toString("yyyy-MM-dd dddd")));
    // 加载数据
    loadCurrentSchedules();
    // 按钮（添加、删除、关闭窗口）
    connect(ui->btn_add,    &QToolButton::clicked, this, &EditScheduleDialog::onAddDoctor);
    connect(ui->btn_remove, &QToolButton::clicked, this, &EditScheduleDialog::onRemoveDoctor);
    connect(ui->btn_close,  &QToolButton::clicked, this, &QDialog::reject); //关闭弹窗

    connect(m_netconn, &NetConn::sigDataToDoc,this, &EditScheduleDialog::onDoctorsReceived);
    connect(m_netconn, &NetConn::sigDataToScheduleDoc,this, &EditScheduleDialog::onScheduleDocsReceived);
    connect(m_netconn,&NetConn::sigDataToAddScheduleDlg,this,&EditScheduleDialog::onAddResponse);
    connect(m_netconn,&NetConn::sigDataToDelScheduleDlg,this,&EditScheduleDialog::onDelResponse);
}


// 月视图构造
EditScheduleDialog::EditScheduleDialog(const QDate &date, int departmentId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditScheduleDialog)
    , m_date(date)
    , m_shift(ShiftType::Morning)
    , m_departmentId(departmentId)
    , m_fixShift(false)
{
    ui->setupUi(this);
    setWindowTitle("编辑排班");

    init_qss();
    m_netconn = WinManager::getInstance()->getNetConn();

    // 标题
    ui->lab_date_2->setText(QString("📅 %1").arg(m_date.toString("yyyy-MM-dd dddd")));

    // 班次下拉框：填入三个班次，让用户选
    ui->combo_shift->clear();
    ui->combo_shift->addItem("上午", static_cast<int>(ShiftType::Morning));
    ui->combo_shift->addItem("下午", static_cast<int>(ShiftType::Afternoon));
    ui->combo_shift->addItem("夜班", static_cast<int>(ShiftType::Night));
    ui->combo_shift->setCurrentIndex(0);
    ui->combo_shift->setEnabled(true);

    // 用户切班次 → 重新查该班次的已排医生
    connect(ui->combo_shift, QOverload<int>::of(&QComboBox::currentIndexChanged),this, [this](int idx){
        if (idx < 0) return;
        m_shift = static_cast<ShiftType>(
            ui->combo_shift->currentData().toInt());
        qDebug() << "月视图切换班次:" << static_cast<int>(m_shift);

        // 重新查该班次已排医生
        requestScheduledDoctors();
    });

    // 按钮
    connect(ui->btn_add,    &QToolButton::clicked, this, &EditScheduleDialog::onAddDoctor);
    connect(ui->btn_remove, &QToolButton::clicked, this, &EditScheduleDialog::onRemoveDoctor);
    connect(ui->btn_close,  &QToolButton::clicked, this, &QDialog::reject);

    // 网络信号
    connect(m_netconn, &NetConn::sigDataToDoc,          this, &EditScheduleDialog::onDoctorsReceived);
    connect(m_netconn, &NetConn::sigDataToScheduleDoc,  this, &EditScheduleDialog::onScheduleDocsReceived);
    connect(m_netconn, &NetConn::sigDataToAddScheduleDlg, this, &EditScheduleDialog::onAddResponse);
    connect(m_netconn, &NetConn::sigDataToDelScheduleDlg, this, &EditScheduleDialog::onDelResponse);

    // 加载数据
    loadCurrentSchedules();
}

EditScheduleDialog::~EditScheduleDialog()
{
    delete ui;
}


// ================================================================================================
// ====================================== 初始化 ==================================================
// ================================================================================================

void EditScheduleDialog::init_qss()
{
    QFile file(":/qss/EditScheduleDlg.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

// 加载当前班次数据
void EditScheduleDialog::loadCurrentSchedules()
{
    if (!m_netconn) {
        qWarning() << "网络未连接";
        return;
    }
    // 周视图下禁止修改班次显示
    if (m_fixShift) {
        int row = shiftToRow();
        if (row >= 0 && row < m_timeSlots.size()) {
            ui->combo_shift->clear();
            ui->combo_shift->addItem(m_timeSlots[row]);
            ui->combo_shift->setEnabled(false);
        } else {
            qWarning() << "非法班次行:" << row;
        }
    }
// 本科室医生数据请求（用于添加排班）
    requestDoctors();
// 已排班医生数据请求
    requestScheduledDoctors();
}


// ================================================================================================
// ====================================== 工具函数 ================================================
// ================================================================================================

void EditScheduleDialog::tryFillDoctorCombo()
{
    if(m_doctors.isEmpty()) return;

    int currentDocId = ui->combo_doctor->currentData().toInt();

    ui->combo_doctor->clear();
    ui->combo_doctor->addItem("请选择", -1);
    for (const auto& d : std::as_const(m_doctors)) {
        if(m_addDocId.contains(d.staffId)) continue;
        ui->combo_doctor->addItem(
            QString("%1(%2)").arg(d.realName).arg(d.title),d.staffId);
    }
    int idx = ui->combo_doctor->findData(currentDocId);
    if (idx >= 0) ui->combo_doctor->setCurrentIndex(idx);
    else ui->combo_doctor->setCurrentIndex(0);
}

// 将班次信息转为行索引
int EditScheduleDialog::shiftToRow()
{
    return static_cast<int>(m_shift) - 1;
}

bool EditScheduleDialog::isDataChanged() const
{
    return m_dataChanged;
}


// ================================================================================================
// ====================================== 网络包响应 ================================================
// ================================================================================================

void EditScheduleDialog::onDoctorsReceived(const QByteArray &body)
{
    if (body.size() == 0 || body.size() % sizeof(DOCTOR_LIST_RESP) != 0) {
        qWarning() << "医生列表包长度异常:" << body.size();
        return;
    }

    int count = body.size() / sizeof(DOCTOR_LIST_RESP);
    QList<DoctorInfo> list;
    list.reserve(count);

    const char* p = body.constData();

    auto safeStr = [](const char* s, int maxLen) {
        int len = 0;
        while (len < maxLen && s[len] != '\0') ++len;
        return QString::fromUtf8(s, len);
    };
    for (int i = 0; i < count; ++i) {
        const auto* item = reinterpret_cast<const DOCTOR_LIST_RESP*>(p);
        DoctorInfo d;
        d.staffId         = item->staff_id;
        d.realName        = safeStr(item->real_name, sizeof(item->real_name));
        d.title           = safeStr(item->title,     sizeof(item->title));
        d.maxWeeklyShifts = item->max_weekly_shifts;
        list.append(d);
        p += sizeof(DOCTOR_LIST_RESP);
    }
    m_doctors = list;
    //qDebug()<<"医生数据接收："<<list.size()<<"条";

    tryFillDoctorCombo();
}

void EditScheduleDialog::onScheduleDocsReceived(const QByteArray &body)
{
    // 长度校验
    if (body.size() % sizeof(Schedule_Doc_RESP) != 0) {
        qWarning() << "已排班医生包长度异常:" << body.size();
        return;
    }
    int count = body.size() / sizeof(Schedule_Doc_RESP);
    const char* p = body.constData();

    m_addDocId.clear();
    ui->list_scheduled->clear();
    for(int i=0;i<count;i++){
        const auto* item = reinterpret_cast<const Schedule_Doc_RESP*>(p);

        QString name  = QString::fromUtf8(item->name,  qstrnlen(item->name,  sizeof(item->name)));
        QString title = QString::fromUtf8(item->title, qstrnlen(item->title, sizeof(item->title)));

        QString doc = QString("%1(%2)").arg(name).arg(title);

        // 存 userData = staff_id，方便后续移除
        auto* listItem = new QListWidgetItem(doc);
        listItem->setData(Qt::UserRole, item->staff_id);
        m_addDocId.insert(item->staff_id);

        ui->list_scheduled->addItem(listItem);

        p += sizeof(Schedule_Doc_RESP);
    }
    //qDebug() << "已排班医生:" << count << "条";
    tryFillDoctorCombo();
}


void EditScheduleDialog::onAddResponse(const QByteArray &body)
{
    if (body.size() != sizeof(ADD_SCHEDULE_RESP)) {
        qWarning() << "ADD_SCHEDULE_RESP 长度异常:" << body.size();
        return;
    }

    const auto* resp = reinterpret_cast<const ADD_SCHEDULE_RESP*>(body.constData());
    QString msg = QString::fromUtf8(resp->message, qstrnlen(resp->message, sizeof(resp->message)));

    switch (resp->code) {
    case 0:
        // 成功
        QMessageBox::information(this, "成功", msg);
        m_dataChanged = true;
        requestScheduledDoctors();
        break;

    case 1: {
        // 软冲突，询问用户
        auto btn = QMessageBox::question(this, "提示", msg,
                                         QMessageBox::Yes | QMessageBox::No);
        if (btn == QMessageBox::Yes) {
            // 用户确认 → 发强制添加请求
            sendAddSchedule(true);
        }
        break;
    }

    case 2:
        // 硬冲突，直接拦截
        QMessageBox::warning(this, "无法添加", msg);
        break;

    default:
        QMessageBox::warning(this, "未知错误", msg);
        break;
    }
}

void EditScheduleDialog::onDelResponse(const QByteArray &body)
{
    const auto* resp = reinterpret_cast<const DEL_SCHEDULE_RESP*>(body.constData());
    if (resp->code == 0) {
        //QMessageBox::information(this, "成功", "已移除");
        m_dataChanged = true;
        requestScheduledDoctors();   // 重新查
    } else {
        QMessageBox::warning(this, "失败",
                             QString::fromUtf8(resp->message));
    }
}



// ================================================================================================
// ====================================== 网络包请求 ================================================
// ================================================================================================

void EditScheduleDialog::requestScheduledDoctors()
{
    Schedule_Doc_REQ req = {};
    req.deptId = m_departmentId;
    req.shift  = static_cast<uint8_t>(m_shift);

    QByteArray dateBytes = m_date.toString("yyyy-MM-dd").toUtf8();
    strncpy(req.workdate, dateBytes.constData(), sizeof(req.workdate) - 1);
    req.workdate[sizeof(req.workdate) - 1] = '\0';

    HEAD head = { SERVICE_TYPE::Schedule_Doc, sizeof(Schedule_Doc_REQ) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(Schedule_Doc_REQ));
    m_netconn->sendPacket(packet);
}

void EditScheduleDialog::requestDoctors()
{
    DOCTOR_LIST_REQ req = {};
    req.dept_id = m_departmentId;

    HEAD head = { SERVICE_TYPE::DOCTOR_LIST, sizeof(DOCTOR_LIST_REQ)};

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req),  sizeof(DOCTOR_LIST_REQ));
    m_netconn->sendPacket(packet);
}

void EditScheduleDialog::onAddDoctor()
{
    // ===== 1. 取选中的医生 =====
    int staffId = ui->combo_doctor->currentData().toInt();
    if (staffId == -1) {
        QMessageBox::warning(this, "提示", "请先选择医生");
        return;
    }

    // ===== 2. C4：一个班次只排一人（本地） =====
    if (ui->list_scheduled->count() > 0) {
        QMessageBox::warning(this, "提示",
                             "该班次已排班\n"
                             "如需更换，请先移除已有医生。");
        return;
    }
    // 4、发送请求
    sendAddSchedule(false);
}

void EditScheduleDialog::onRemoveDoctor()
{
    auto* item = ui->list_scheduled->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先选中要移除的医生");
        return;
    }

    int doctorId = item->data(Qt::UserRole).toInt();

    // 二次确认
    auto ret = QMessageBox::question(this, "确认",
                                     QString("确定要移除该医生吗？"),
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    sendDelSchedule(doctorId);
}


void EditScheduleDialog::sendAddSchedule(bool force)
{
    ADD_SCHEDULE_REQ req = {};
    req.dept_id  = m_departmentId;
    req.staff_id = ui->combo_doctor->currentData().toInt();
    req.shift = static_cast<uint8_t>(m_shift);
    req.force = force ? 1:0;   // 0 = 先检查

    QByteArray dateBytes = m_date.toString("yyyy-MM-dd").toUtf8();
    strncpy(req.workdate, dateBytes.constData(), sizeof(req.workdate) - 1);
    req.workdate[sizeof(req.workdate) - 1] = '\0';

    HEAD head = { SERVICE_TYPE::CHECK_ADD_SCHEDULE, sizeof(ADD_SCHEDULE_REQ) };

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req),  sizeof(ADD_SCHEDULE_REQ));
    m_netconn->sendPacket(packet);
}

void EditScheduleDialog::sendDelSchedule(int doctorId)
{
    DEL_SCHEDULE_REQ req = {};
    req.dept_id     = m_departmentId;
    req.doctor_id   = doctorId;
    req.shift       = static_cast<uint8_t>(m_shift);

    QByteArray dateBytes = m_date.toString("yyyy-MM-dd").toUtf8();
    strncpy(req.workdate, dateBytes.constData(), sizeof(req.workdate) - 1);

    HEAD head = { SERVICE_TYPE::CHECK_DEL_SCHEDULE, sizeof(DEL_SCHEDULE_REQ) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(DEL_SCHEDULE_REQ));
    m_netconn->sendPacket(packet);
}