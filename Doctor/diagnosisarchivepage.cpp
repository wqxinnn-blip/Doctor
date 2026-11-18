#include "diagnosisarchivepage.h"
//#include "ui_diagnosisarchivepage.h"
#include "winmanager.h"
#include "addmedicinedialog.h"
#include "tonguereportdialog.h"

DiagnosisArchivePage::DiagnosisArchivePage(QWidget *parent)
    : QWidget(parent)
{

    m_netConn = WinManager::getInstance()->getNetConn();

    setupUi();
    applyQss();

    // 网络信号
    connect(m_netConn, &NetConn::sigDataToPatientInfo,
            this, &DiagnosisArchivePage::onPatientInfoReceived);
    connect(m_netConn, &NetConn::sigDataToHistoryReport,
            this, &DiagnosisArchivePage::onHistoryReportReceived);
    connect(m_netConn, &NetConn::sigDataToSaveDiagnosis,
            this, &DiagnosisArchivePage::onSaveDiagnosisResp);
    connect(m_netConn, &NetConn::sigDataToGenerateReport,
            this, &DiagnosisArchivePage::onGenerateReportResp);
}

DiagnosisArchivePage::~DiagnosisArchivePage()
{
}

// 加载患者的基本信息和最近一次诊疗
void DiagnosisArchivePage::loadPatient(int appointmentId, int patientId,int tongue_record_id)
{
    m_patient.patientId = patientId;
    m_patient.appointmentId = appointmentId;
    m_patient.tongue_record_id = tongue_record_id;

    m_savedRecordId = -1;
    clearInputs();

    // 请求患者信息 + 历史报告
    requestPatientInfo(patientId);
    requestHistoryReport(patientId);
}

void DiagnosisArchivePage::onEnter()
{
    // 如果外部通过导航进来时已经 loadPatient 过，可以在这里刷新
}

void DiagnosisArchivePage::onLeave()
{
    clearInputs();
}

// ============================================================
//                        UI 搭建
// ============================================================
void DiagnosisArchivePage::setupUi()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(24);

    // 左侧
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(16);
    setupLeftLayout(leftLayout);
    mainLayout->addWidget(leftWidget, 2);

    // 右侧
    auto* rightWidget = new QWidget(this);
    rightWidget->setFixedWidth(260);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(16);
    setupRightLayout(rightLayout);
    mainLayout->addWidget(rightWidget, 0);
}

// ============================================================
//                        左侧布局
// ============================================================
void DiagnosisArchivePage::setupLeftLayout(QVBoxLayout* root)
{
    // ==================== 历史报告 ====================
    auto* historyTitle = new QLabel("历史报告", this);
    historyTitle->setObjectName("sectionTitle");
    root->addWidget(historyTitle);

    // 历史报告卡片
    auto* historyCard = new QFrame(this);
    historyCard->setObjectName("historyCard");
    auto* hLayout = new QVBoxLayout(historyCard);
    hLayout->setContentsMargins(20, 16, 20, 16);
    hLayout->setSpacing(8);

    // 日期
    m_labHistoryDate = new QLabel("暂无历史记录", historyCard);
    m_labHistoryDate->setObjectName("historyDate");
    hLayout->addWidget(m_labHistoryDate);

    // 诊疗结果
    m_labHistoryResult = new QLabel("", historyCard);
    m_labHistoryResult->setWordWrap(true);
    m_labHistoryResult->setObjectName("historyText");
    hLayout->addWidget(m_labHistoryResult);

    // 健康提示
    m_labHistoryTips = new QLabel("", historyCard);
    m_labHistoryTips->setWordWrap(true);
    m_labHistoryTips->setObjectName("historyText");
    hLayout->addWidget(m_labHistoryTips);

    // 建议用药
    m_labHistorySuggestion = new QLabel("", historyCard);
    m_labHistorySuggestion->setWordWrap(true);
    m_labHistorySuggestion->setObjectName("historyText");
    hLayout->addWidget(m_labHistorySuggestion);

    // "更多"按钮
    auto* moreRow = new QHBoxLayout();
    moreRow->addStretch();
    m_btnMoreHistory = new QPushButton("更多 >", historyCard);
    m_btnMoreHistory->setObjectName("btnMore");
    m_btnMoreHistory->setCursor(Qt::PointingHandCursor);
    moreRow->addWidget(m_btnMoreHistory);
    hLayout->addLayout(moreRow);

    root->addWidget(historyCard);

    connect(m_btnMoreHistory, &QPushButton::clicked,
            this, &DiagnosisArchivePage::onMoreHistoryClicked);

    // ==================== 此次诊疗 ====================
    auto* diagTitle = new QLabel("此次诊疗", this);
    diagTitle->setObjectName("sectionTitle");
    root->addWidget(diagTitle);

    auto* diagCard = new QFrame(this);
    diagCard->setObjectName("diagCard");
    auto* dLayout = new QVBoxLayout(diagCard);
    dLayout->setContentsMargins(20, 16, 20, 16);
    dLayout->setSpacing(14);

    auto makeDiagRow = [this, dLayout](const QString& label,
                                       QTextEdit*& edit,
                                       int minH) -> QHBoxLayout* {
        auto* row = new QHBoxLayout();
        auto* lab = new QLabel(label, this);
        lab->setObjectName("diagLabel");
        lab->setFixedWidth(80);
        lab->setAlignment(Qt::AlignRight | Qt::AlignTop);

        edit = new QTextEdit(this);
        edit->setObjectName("diagEdit");
        edit->setMinimumHeight(minH);
        edit->setPlaceholderText("请输入...");

        row->addWidget(lab);
        row->addWidget(edit, 1);
        dLayout->addLayout(row);

        return row;   // ★ 返回布局
    };

    makeDiagRow("诊疗结果：", m_editResult, 70);
    makeDiagRow("诊疗建议：", m_editSuggestion, 70);
    // 建议用药 + 添加按钮
    auto* medicineRow = new QHBoxLayout();

    auto* lab = new QLabel("建议用药：", this);
    lab->setObjectName("diagLabel");
    lab->setFixedWidth(80);
    lab->setAlignment(Qt::AlignRight | Qt::AlignTop);

    m_medicineList = new QListWidget(this);
    m_medicineList->setObjectName("diagList");
    m_medicineList->setMinimumHeight(80);

    m_btnAddMedicine = new QPushButton("添加用药", this);
    m_btnAddMedicine->setObjectName("btnAddMedicine");
    m_btnAddMedicine->setCursor(Qt::PointingHandCursor);
    m_btnAddMedicine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    medicineRow->addWidget(lab);
    medicineRow->addWidget(m_medicineList, 1);
    medicineRow->addWidget(m_btnAddMedicine, 0, Qt::AlignTop);
    dLayout->addLayout(medicineRow);


    medicineRow->addWidget(m_btnAddMedicine, 0, Qt::AlignTop);
    connect(m_btnAddMedicine, &QPushButton::clicked,
            this, &DiagnosisArchivePage::onAddMedicineClicked);

    root->addWidget(diagCard, 1);
}

// ============================================================
//                        右侧布局
// ============================================================
void DiagnosisArchivePage::setupRightLayout(QVBoxLayout* root)
{
    // ==================== 患者信息卡 ====================
    auto* infoCard = new QFrame(this);
    infoCard->setObjectName("patientCard");
    auto* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(14);

    // 头像
    auto* avatar = new QLabel(infoCard);
    avatar->setObjectName("avatar");
    avatar->setFixedSize(80, 80);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setText("👤");
    auto* avatarRow = new QHBoxLayout();
    avatarRow->addStretch();
    avatarRow->addWidget(avatar);
    avatarRow->addStretch();
    infoLayout->addLayout(avatarRow);

    // 姓名
    m_labPatientName = new QLabel("姓名：--", infoCard);
    m_labPatientName->setObjectName("patientInfo");
    infoLayout->addWidget(m_labPatientName);

    // 年龄
    m_labPatientAge = new QLabel("年龄：--", infoCard);
    m_labPatientAge->setObjectName("patientInfo");
    infoLayout->addWidget(m_labPatientAge);

    // 手机号
    m_labPatientPhone = new QLabel("手机号：--", infoCard);
    m_labPatientPhone->setObjectName("patientInfo");
    infoLayout->addWidget(m_labPatientPhone);

    root->addWidget(infoCard);

    // ==================== 查看检测报告 ====================
    auto* reportCard = new QFrame(this);
    reportCard->setObjectName("actionCard");
    auto* reportLayout = new QVBoxLayout(reportCard);
    reportLayout->setContentsMargins(16, 16, 16, 16);

    m_btnViewReport = new QPushButton("查看检测报告", reportCard);
    m_btnViewReport->setObjectName("btnViewReport");
    m_btnViewReport->setCursor(Qt::PointingHandCursor);
    m_btnViewReport->setMinimumHeight(60);
    reportLayout->addWidget(m_btnViewReport);

    root->addWidget(reportCard);

    // ==================== 操作区 ====================
    auto* actionCard = new QFrame(this);
    actionCard->setObjectName("actionCard");
    auto* actionLayout = new QVBoxLayout(actionCard);
    actionLayout->setContentsMargins(16, 16, 16, 16);
    actionLayout->setSpacing(12);

    m_btnCameraDetect = new QPushButton("摄像头检测", actionCard);
    m_btnCameraDetect->setObjectName("btnCamera");
    m_btnCameraDetect->setCursor(Qt::PointingHandCursor);
    m_btnCameraDetect->setMinimumHeight(50);

    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(10);

    m_btnSave = new QPushButton("保存", actionCard);
    m_btnSave->setObjectName("btnSave");
    m_btnSave->setCursor(Qt::PointingHandCursor);
    m_btnSave->setMinimumHeight(44);

    m_btnReport = new QPushButton("生成报告", actionCard);
    m_btnReport->setObjectName("btnReport");
    m_btnReport->setCursor(Qt::PointingHandCursor);
    m_btnReport->setMinimumHeight(44);

    bottomRow->addWidget(m_btnSave);
    bottomRow->addWidget(m_btnReport);

    actionLayout->addWidget(m_btnCameraDetect);
    actionLayout->addLayout(bottomRow);

    root->addWidget(actionCard);

    // ==================== 提示区 ====================
    m_labStatus = new QLabel(this);
    m_labStatus->setObjectName("operationStatus");
    m_labStatus->setWordWrap(true);            // 允许换行
    m_labStatus->setAlignment(Qt::AlignCenter);// 居中
    m_labStatus->setMinimumHeight(36);
    m_labStatus->setText("");                  // 初始为空
    root->addWidget(m_labStatus);
    root->addStretch();

    // 信号
    connect(m_btnViewReport,    &QPushButton::clicked, this, &DiagnosisArchivePage::onViewReportClicked);
    connect(m_btnCameraDetect,  &QPushButton::clicked, this, &DiagnosisArchivePage::onCameraDetectClicked);
    connect(m_btnSave,          &QPushButton::clicked, this, &DiagnosisArchivePage::onSaveClicked);
    connect(m_btnReport,        &QPushButton::clicked, this, &DiagnosisArchivePage::onGenerateReportClicked);
}

void DiagnosisArchivePage::applyQss()
{
    //1、打开文件
    QFile file(":/qss/dignosis.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}


// ============================================================
//                         网络请求
// ============================================================
void DiagnosisArchivePage::requestPatientInfo(int patientId)
{
    PATIENT_INFO_REQ req = {};
    req.patient_id = patientId;

    HEAD head = { SERVICE_TYPE::PATIENT_INFO, sizeof(req) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));
    m_netConn->sendPacket(packet);
}

void DiagnosisArchivePage::requestHistoryReport(int patientId)
{
    HISTORY_REPORT_REQ req = {};
    req.patient_id = patientId;

    HEAD head = { SERVICE_TYPE::HISTORY_REPORT, sizeof(req)};
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));
    m_netConn->sendPacket(packet);
}

void DiagnosisArchivePage::requestSaveDiagnosis()
{
    SAVE_DIAGNOSIS_REQ req = {};
    // 关联外键  预约ID、患者ID、医生ID,舌苔检测记录ID
    req.appointment_id = m_patient.appointmentId;
    req.patient_id     = m_patient.patientId;
    req.doctor_id      = WinManager::getInstance()->getStaff_Info().getStaff_id();
    req.tongue_record_id = m_patient.tongue_record_id;

    // qDebug()<<"预约ID = "<<req.appointment_id<<",患者ID = "<<req.patient_id<<
    //     ",医生ID="<<req.doctor_id<<",检测ID="<<req.tongue_record_id;
    // 填入 诊疗结果、诊疗建议、建议用药
    QByteArray r = m_editResult->toPlainText().toUtf8();
    QByteArray s = m_editSuggestion->toPlainText().toUtf8();
    strncpy(req.diagnosis, r.constData(), sizeof(req.diagnosis) - 1);
    strncpy(req.advice, s.constData(), sizeof(req.advice) - 1);

    //拼接发送包
    req.item_count = m_prescriptionItems.size();
    int bodyLen = sizeof(SAVE_DIAGNOSIS_REQ) + m_prescriptionItems.size() * sizeof(PRESCRIPTION_ITEM);
    HEAD head = { SERVICE_TYPE::SAVE_DIAGNOSIS, bodyLen };

    QByteArray packet;
    packet.reserve(sizeof(HEAD) + bodyLen);
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(SAVE_DIAGNOSIS_REQ));
    // 添加药方
    for (int i = 0; i < m_prescriptionItems.size(); ++i) {
        PRESCRIPTION_ITEM item = {};
        const auto& info = m_prescriptionItems[i];

        QByteArray name = info.medicineName.toUtf8();
        strncpy(item.medicine_name, name.constData(), sizeof(item.medicine_name) - 1);
        QByteArray dosage = info.dosage.toUtf8();
        strncpy(item.dosage, dosage.constData(), sizeof(item.dosage) - 1);
        QByteArray usage = info.usageMethod.toUtf8();
        strncpy(item.usage_method, usage.constData(), sizeof(item.usage_method) - 1);

        item.sort_order = static_cast<uint8_t>(i + 1);
        packet.append(reinterpret_cast<const char*>(&item), sizeof(item));
    }
    m_netConn->sendPacket(packet);
}

void DiagnosisArchivePage::uploadReportFile(const QString &localPath)
{
    // 1. 创建上传任务（首次）
    if (!m_reportUploadTask) {
        m_reportUploadTask = new UploadReportTask(m_netConn, this);
        // 连接信号
        connect(m_reportUploadTask, &UploadReportTask::uploadFinished,
                this, &DiagnosisArchivePage::onReportUploadFinished);
    }

    // 2. 启动上传
    m_reportUploadTask->startUpload(m_savedRecordId, localPath);

}


// ============================================================
//                         网络响应
// ============================================================
void DiagnosisArchivePage::onPatientInfoReceived(const QByteArray& body)
{
    if (body.size() < (int)sizeof(PATIENT_INFO_RESP)) return;
    const auto* resp = reinterpret_cast<const PATIENT_INFO_RESP*>(body.constData());
    if (resp->code != 0) return;

    // 添加本次就诊患者的基本信息
    m_patient.name  = QString::fromUtf8(resp->name,qstrnlen(resp->name,sizeof(resp->name)));
    m_patient.age   = resp->age;
    m_patient.phone = QString::fromUtf8(resp->phone,qstrnlen(resp->phone,sizeof(resp->phone)));
    m_patient.gender = resp->gender;
    // 添加信息
    m_labPatientName->setText(QString("姓名：%1").arg(m_patient.name));
    m_labPatientAge->setText(QString("年龄：%1 岁").arg(m_patient.age));
    m_labPatientPhone->setText(QString("手机号：%1").arg(m_patient.phone));
}

void DiagnosisArchivePage::onHistoryReportReceived(const QByteArray& body)
{
    if (body.size() < (int)sizeof(HISTORY_REPORT_RESP)) return;
    const auto* resp = reinterpret_cast<const HISTORY_REPORT_RESP*>(body.constData());

    if (resp->code != 0) {
        m_lastHistory = HistoryReport();   // 重置
        updateHistoryUI();
        return;
    }
    // 基本信息
    HistoryReport h;
    h.recordId = resp->record_id;
    h.doctorId = resp->doctor_id;
    h.doctorName = QString::fromUtf8(resp->doctor_name,
                                     qstrnlen(resp->doctor_name, sizeof(resp->doctor_name)));
    h.visitDate  = QString::fromUtf8(resp->visit_date,
                                    qstrnlen(resp->visit_date, sizeof(resp->visit_date)));
    h.diagnosis = QString::fromUtf8(resp->diagnosis,
                                    qstrnlen(resp->diagnosis, sizeof(resp->diagnosis)));

    // 解析处方明细
    const char* p = body.constData() + sizeof(HISTORY_REPORT_RESP);
    for (int i = 0; i < resp->item_count; ++i) {
        const auto* item = reinterpret_cast<const PRESCRIPTION_ITEM*>(p);
        PrescriptionItemInfo info;
        info.medicineName = QString::fromUtf8(item->medicine_name,qstrnlen(item->medicine_name,
                                                 sizeof(item->medicine_name))).trimmed();
        info.dosage  = QString::fromUtf8(item->dosage,qstrnlen(item->dosage,
                                                 sizeof(item->dosage))).trimmed();
        info.usageMethod = QString::fromUtf8(item->usage_method,qstrnlen(item->usage_method,
                                                 sizeof(item->usage_method))).trimmed();
        h.items.append(info);

        p += sizeof(PRESCRIPTION_ITEM);
    }
    m_lastHistory = h;
    // 刷新历史报告区
    updateHistoryUI();
}

void DiagnosisArchivePage::onSaveDiagnosisResp(const QByteArray& body)
{
    if (body.size() < (int)sizeof(SAVE_DIAGNOSIS_RESP)) return;
    const auto* resp = reinterpret_cast<const SAVE_DIAGNOSIS_RESP*>(body.constData());

    if (resp->code == 0) {
        // 保存诊疗记录 ID
        m_savedRecordId = resp->record_id;
        QMessageBox::information(this, "成功", "诊疗记录已保存");
    } else {
        QMessageBox::warning(this, "失败",QString::fromUtf8(resp->message, qstrnlen(resp->message, sizeof(resp->message))));
    }
}

void DiagnosisArchivePage::onGenerateReportResp(const QByteArray& body)
{
    if (body.size() < (int)sizeof(GENERATE_REPORT_RESP)) return;
    const auto* resp = reinterpret_cast<const GENERATE_REPORT_RESP*>(body.constData());

    if (resp->code == 0) {
        QMessageBox::information(this, "成功", "诊疗报告已生成并保存！");
        // 通知主窗口刷新列表
        emit reportGenerated(m_patient.appointmentId);
    } else {
        m_labStatus->setText(QString::fromUtf8(resp->message, qstrnlen(resp->message, sizeof(resp->message))));
    }
}

void DiagnosisArchivePage::onReportUploadFinished(bool flag, QString msg,QString fullPath)
{
    if (flag) {
        m_labStatus->setText("报告已生成并上传");
        QFile::remove(fullPath);
    } else {
        m_labStatus->setText("上传失败：" + msg);
    }
}

// ============================================================
//                         按钮槽
// ============================================================
void DiagnosisArchivePage::onMoreHistoryClicked()
{
    QMessageBox::information(this, "提示", "查看全部历史记录");
}

void DiagnosisArchivePage::onCameraDetectClicked()
{
    QMessageBox::information(this, "提示", "打开摄像头检测");
}
// 主要按钮槽
void DiagnosisArchivePage::onViewReportClicked()
{
    if (m_patient.tongue_record_id <= 0) {
        QMessageBox::information(this, "提示", "该患者暂无检测报告");
        return;
    }
    TongueReportDialog dlg(m_patient.tongue_record_id, this);
    dlg.exec();
}

void DiagnosisArchivePage::onSaveClicked()
{
    QString errMsg;
    if (!validateInputs(errMsg)) {
        QMessageBox::warning(this, "提示", errMsg);
        return;
    }
    requestSaveDiagnosis();
}

void DiagnosisArchivePage::onGenerateReportClicked()
{
    if (m_savedRecordId <= 0) {
        QMessageBox::warning(this, "提示", "请先保存诊疗记录");
        return;
    }

    HEAD head = {SERVICE_TYPE::GENERATE_REPORT,sizeof(GENERATE_REPORT_REQ)};
    GENERATE_REPORT_REQ req = {};
    req.medical_record_id = m_savedRecordId;

    QByteArray packet;
    packet.resize(sizeof(HEAD) + sizeof(GENERATE_REPORT_REQ)); // ★ 改成 resize
    memcpy(packet.data(), &head, sizeof(HEAD));
    memcpy(packet.data() + sizeof(HEAD), &req, sizeof(GENERATE_REPORT_REQ));
    m_netConn->sendPacket(packet);
}

void DiagnosisArchivePage::onAddMedicineClicked()
{
    AddMedicineDialog dlg(m_prescriptionItems,this);

    if (dlg.exec() == QDialog::Accepted) {
        m_prescriptionItems = dlg.getItems();
        //刷新"建议用药"显示
        updateMedicineLabel();
    }
}


// ============================================================
//                         工具
// ============================================================
void DiagnosisArchivePage::clearInputs()
{
    if (m_editResult)     m_editResult->clear();
    if (m_editSuggestion) m_editSuggestion->clear();

    m_prescriptionItems.clear();
    updateMedicineLabel();
}

bool DiagnosisArchivePage::validateInputs(QString& errMsg)
{
    // 1. 校验
    if (m_editResult->toPlainText().trimmed().isEmpty()) {
        errMsg = "诊疗结果不能为空";
        return false;
    }
    if (m_editSuggestion->toPlainText().trimmed().isEmpty()) {
        errMsg = "诊疗建议不能为空";
        return false;
    }
    if (m_prescriptionItems.isEmpty()) {
        errMsg = "请至少添加一味药";
        return false;
    }
    return true;
}

void DiagnosisArchivePage::updateMedicineLabel()
{
    // 1. 清空列表
    m_medicineList->clear();
    // 2. 空处理
    if (m_prescriptionItems.isEmpty()) {
        auto* placeholder = new QListWidgetItem("暂无药品");
        placeholder->setTextAlignment(Qt::AlignCenter);
        placeholder->setForeground(QColor("#909399"));
        placeholder->setFlags(Qt::NoItemFlags);   // 不可选
        m_medicineList->addItem(placeholder);
        return;
    }
    // 3. 添加
    for (int i = 0; i < m_prescriptionItems.size(); ++i) {
        const auto& item = m_prescriptionItems[i];

        QString text = QString("%1. %2  %3  %4")
                           .arg(i + 1)
                           .arg(item.medicineName)
                           .arg(item.dosage)
                           .arg(item.usageMethod);

        m_medicineList->addItem(text);
    }
}

QString DiagnosisArchivePage::generateLocalReport()
{
    // 1. 本地临时目录
    QString tmpDir = QDir::tempPath() + "/doctor_reports";
    QDir().mkpath(tmpDir);

    // 2. 文件名
    QString fileName = QString("report_%1_%2.doc")
                           .arg(m_savedRecordId)
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString fullPath = tmpDir + "/" + fileName;

    // 3. 拼 HTML（从成员变量拿数据）
    QString html;
    QTextStream out(&html);

    out << "<html><head><meta charset='utf-8'>"
        << "<style>"
        << "body { font-family: 'Microsoft YaHei', SimSun; font-size: 14px; padding: 40px; }"
        << "h1 { text-align: center; color: #2c3e50; }"
        << "h2 { color: #2980b9; border-bottom: 1px solid #ccc; padding-bottom: 5px; }"
        << "table { border-collapse: collapse; width: 100%; margin: 10px 0; }"
        << "th, td { border: 1px solid #ccc; padding: 8px; }"
        << "th { background-color: #f0f0f0; }"
        << ".label { font-weight: bold; display: inline-block; width: 100px; }"
        << "</style></head><body>";

    out << "<h1>中医诊疗报告</h1>";

    // 患者信息
    out << "<h2>一、患者信息</h2>";
    out << "<div><span class='label'>姓名：</span>" << m_patient.name << "</div>";
    out << "<div><span class='label'>年龄：</span>" << m_patient.age << " 岁</div>";
    out << "<div><span class='label'>就诊日期：</span>"
        << QDate::currentDate().toString("yyyy-MM-dd") << "</div>";
    out << "<div><span class='label'>就诊医生：</span>"
        << WinManager::getInstance()->getStaff_Info().getName() << "</div>";

    // 诊断
    out << "<h2>二、诊断结果</h2>";
    out << "<p>" << m_editResult->toPlainText().toHtmlEscaped().replace("\n", "<br/>") << "</p>";

    // 诊疗建议
    out << "<h2>三、诊疗建议</h2>";
    out << "<p>" << m_editSuggestion->toPlainText().toHtmlEscaped().replace("\n", "<br/>") << "</p>";

    // 处方明细
    out << "<h2>四、处方明细</h2>";
    out << "<table>"
        << "<tr><th>序号</th><th>药名</th><th>剂量</th><th>用法</th></tr>";

    for (int i = 0; i < m_prescriptionItems.size(); ++i) {
        const auto& item = m_prescriptionItems[i];
        out << "<tr>"
            << "<td>" << (i + 1) << "</td>"
            << "<td>" << item.medicineName << "</td>"
            << "<td>" << item.dosage << "</td>"
            << "<td>" << item.usageMethod << "</td>"
            << "</tr>";
    }
    out << "</table>";

    // 底部
    out << "<br/><br/>";
    out << "<div style='text-align:right; margin-top:50px;'>"
        << "医生签名：____________<br/>"
        << "日期：" << QDate::currentDate().toString("yyyy 年 MM 月 dd 日")
        << "</div>";

    out << "</body></html>";

    // 4. 写文件
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法创建报告:" << fullPath;
        return "";
    }

    QTextStream fileOut(&file);
    fileOut.setEncoding(QStringConverter::Utf8);
    fileOut << html;
    file.close();

    return fullPath;
}

void DiagnosisArchivePage::updateHistoryUI()
{
    if (m_lastHistory.recordId <= 0) {
        m_labHistoryDate->setText("暂无历史诊疗记录");
        m_labHistoryResult->clear();
        m_labHistoryTips->clear();
        m_labHistorySuggestion->clear();
        return;
    }
    // 标题：就诊时间 + 医生
    m_labHistoryDate->setText(QString("最近一次就诊：%1  医生：%2")
            .arg(m_lastHistory.visitDate)
            .arg(m_lastHistory.doctorName));

    // 诊断
    QString raw = m_lastHistory.diagnosis;
    QString diagPart, advicePart;
    // 找分隔符
    int idx = raw.indexOf("【诊疗建议】");
    if (idx >= 0) {
        diagPart = raw.left(idx).trimmed();      // 诊断
        advicePart = raw.mid(idx + QString("【诊疗建议】").length()).trimmed();  // 建议
    } else {
        // 整个作为诊疗结果
        diagPart = raw.trimmed();
        advicePart = "";
    }
    // 诊断
    QString diagHtml = diagPart.toHtmlEscaped().replace("\n", "<br/>");
    m_labHistoryResult->setText(
        QString("<b>诊断：</b>%1").arg(diagHtml.isEmpty() ? "无" : diagHtml));

    // 诊疗建议
    QString adviceHtml = advicePart.toHtmlEscaped().replace("\n", "<br/>");
    m_labHistoryTips->setText(
        QString("<b>诊疗建议：</b>%1").arg(adviceHtml.isEmpty() ? "无" : adviceHtml));
    // 处方
    if (m_lastHistory.items.isEmpty()) {
        m_labHistorySuggestion->setText("<b>处方：</b>无");
    } else {
        QStringList lines;
        for (int i = 0; i < m_lastHistory.items.size(); ++i) {
            const auto& item = m_lastHistory.items[i];
            lines << QString("%1. %2  %3  %4")
                         .arg(i + 1)
                         .arg(item.medicineName)
                         .arg(item.dosage)
                         .arg(item.usageMethod);
        }
        m_labHistorySuggestion->setText(
            "<b>处方：</b><br/>" + lines.join("<br/>"));
    }
}
