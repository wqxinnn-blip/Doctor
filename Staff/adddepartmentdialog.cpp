#include "adddepartmentdialog.h"
#include "ui_adddepartmentdialog.h"
#include "winmanager.h"

AddDepartmentDialog::AddDepartmentDialog(int deptId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddDepartmentDialog)
    , m_deptId(deptId)
{
    ui->setupUi(this);
    setWindowTitle("新增科室");
    init_qss();
    m_netconn = WinManager::getInstance()->getNetConn();
    // 设置网格布局左对齐、顶对齐
    ui->gridLayout_members->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->gridLayout_members->setSpacing(20);

    // 标题
    if (m_deptId > 0) {
        setWindowTitle("编辑档案");
        ui->label->setText("编辑档案");
    } else {
        setWindowTitle("新增科室");
        ui->label->setText("新增科室");
    }

    connect(ui->btn_cancel, &QPushButton::clicked, this, &QDialog::reject);
    // 新增模式
    connect(m_netconn,&NetConn::sigDataToChiefDoctors,this,&AddDepartmentDialog::onChiefDoctorsReceived);
    connect(m_netconn,&NetConn::sigDataToUnassignedDoctors,this,&AddDepartmentDialog::onUnassignedDoctorsReceived);
    connect(m_netconn,&NetConn::sigDataToAddDepartment,this,&AddDepartmentDialog::onAddDepartmentReceived);
    // 编辑模式
    connect(m_netconn, &NetConn::sigDataToDeptDetail, this, &AddDepartmentDialog::onDeptDetailReceived);
    connect(m_netconn, &NetConn::sigDataToUpdateDept, this, &AddDepartmentDialog::onUpdateDepartmentReceived);

    // 负责人下拉框的信号
    connect(ui->mainDoc_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) return;

        static int lastLeaderId = -1; // 记录上一次的负责人
        if (lastLeaderId > 0) {
            QString lastDisplay = m_allDoctors.value(lastLeaderId);
            if (!lastDisplay.isEmpty() && ui->doc_combo->findData(lastLeaderId) == -1) {
                ui->doc_combo->addItem(lastDisplay, lastLeaderId);
            }
        }

        // 2. 获取选中的负责人
        int currentLeaderId = ui->mainDoc_combo->currentData().toInt();
        if (currentLeaderId > 0) {
            if (m_addedMemberIds.contains(currentLeaderId)) {
                m_addedMemberIds.removeOne(currentLeaderId);
                QWidget* tagWidget = m_memberTags.value(currentLeaderId);
                if (tagWidget) {
                    m_memberTags.remove(currentLeaderId);
                    tagWidget->deleteLater();
                    relayoutMembers(); // 重新排版
                }
            }

            // 2.2 把新负责人从团队成员下拉框中删掉
            int idx = ui->doc_combo->findData(currentLeaderId);
            if (idx >= 0) {
                ui->doc_combo->removeItem(idx);
            }
        }
        // 更新记录
        lastLeaderId = currentLeaderId;
    });

    requestDoctors(); //获取未分配的医生和主任和副主任医师
    if (m_deptId > 0) {
        requestDeptDetail(m_deptId);
    }
}

AddDepartmentDialog::~AddDepartmentDialog()
{
    delete ui;
}

// ===============================================================================================
// ====================================== 初始化 =========================================
// ===============================================================================================

void AddDepartmentDialog::init_qss()
{
    //1、打开文件
    QFile file(":/qss/addDept.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}


// ===============================================================================================
// ====================================== 网络数据响应 =========================================
// ===============================================================================================

void AddDepartmentDialog::onChiefDoctorsReceived(const QByteArray &body)
{
    int count = body.size() / sizeof(DOCTOR_LIST_RESP);
    const char* p = body.constData();

    ui->mainDoc_combo->clear();
    for (int i = 0; i < count; ++i) {
        const auto* item = reinterpret_cast<const DOCTOR_LIST_RESP*>(p);
        QString display = QString("%1 (%2)")
                              .arg(QString::fromUtf8(item->real_name, qstrnlen(item->real_name, sizeof(item->real_name))))
                              .arg(QString::fromUtf8(item->title, qstrnlen(item->title, sizeof(item->title))));

        ui->mainDoc_combo->addItem(display, item->staff_id);
        m_allDoctors.insert(item->staff_id, display); // 记录到全局医生Map
        p += sizeof(DOCTOR_LIST_RESP);
    }
    if (count == 0) m_chiefEmpty = true;
    m_chiefLoaded = true;
    tryRenderEditData();
}

void AddDepartmentDialog::onUnassignedDoctorsReceived(const QByteArray &body)
{
    int count = body.size() / sizeof(DOCTOR_LIST_RESP);
    const char* p = body.constData();

    ui->doc_combo->clear();
    for (int i = 0; i < count; ++i) {
        const auto* item = reinterpret_cast<const DOCTOR_LIST_RESP*>(p);
        QString display = QString("%1 (%2)")
                              .arg(QString::fromUtf8(item->real_name, qstrnlen(item->real_name, sizeof(item->real_name))))
                              .arg(QString::fromUtf8(item->title, qstrnlen(item->title, sizeof(item->title))));

        ui->doc_combo->addItem(display, item->staff_id);
        m_allDoctors.insert(item->staff_id, display);
        p += sizeof(DOCTOR_LIST_RESP);
    }
    if (count == 0) m_memberEmpty = true;
    m_memberLoaded = true;
    tryRenderEditData();
}

void AddDepartmentDialog::onAddDepartmentReceived(const QByteArray &body)
{
    ui->btn_enter->setEnabled(true);
    if(body.size() < (int)(sizeof(ADD_DEPT_RESP))) return;
    const auto* resp = reinterpret_cast<const ADD_DEPT_RESP*>(body.constData());

    if(resp->code == 0){
        QMessageBox::information(this,"提示","添加成功");
        accept();
    }
    else{
        QMessageBox::information(this,"提示",QString::fromUtf8(resp->message));
    }
}

void AddDepartmentDialog::addMemberTag(int docId, const QString &docDisplay)
{
    auto* tagWidget = new QWidget(this);
    tagWidget->setStyleSheet("background-color: #C8F0C8; border-radius: 12px;");

    auto* tagLayout = new QHBoxLayout(tagWidget);
    tagLayout->setContentsMargins(8, 2, 8, 2);
    tagLayout->setSpacing(4);

    auto* nameLab = new QLabel(docDisplay, tagWidget);
    nameLab->setStyleSheet("color: #333; background: transparent;");
    auto* btnDel = new QPushButton("x", tagWidget);
    btnDel->setFixedSize(16, 16);
    btnDel->setStyleSheet("background: transparent; border: none; color: #555; font-weight: bold;");

    tagLayout->addWidget(nameLab);
    tagLayout->addWidget(btnDel);

    m_memberTags.insert(docId, tagWidget);

    connect(btnDel, &QPushButton::clicked, this, [this, docId, docDisplay, tagWidget]() {
        m_addedMemberIds.removeOne(docId);
        m_memberTags.remove(docId);

        //加回到医生下拉框
        ui->doc_combo->addItem(docDisplay, docId);
        tagWidget->deleteLater();
        relayoutMembers();
    });
}

void AddDepartmentDialog::onDeptDetailReceived(const QByteArray &body)
{
    if (body.size() < (int)sizeof(GET_DEPT_DETAIL_RESP)) return;
    const auto* resp = reinterpret_cast<const GET_DEPT_DETAIL_RESP*>(body.constData());

    // 回显文字信息
    ui->edit_dept->setText(QString::fromUtf8(resp->dept_name));
    ui->edit_detail->setText(QString::fromUtf8(resp->profile));
    m_tempLeaderId = resp->leader_id;

    // 解析老成员 ID
    m_tempMemberIds.clear();
    const char* p = body.constData() + sizeof(GET_DEPT_DETAIL_RESP);
    for (int i = 0; i < resp->member_count; ++i) {
        int id;
        memcpy(&id, p, sizeof(int));
        m_tempMemberIds.append(id); // 添加到临时成员列表
        p += sizeof(int);
    }

    m_detailLoaded = true;
    tryRenderEditData(); // 尝试渲染
}

void AddDepartmentDialog::onUpdateDepartmentReceived(const QByteArray &body)
{
    ui->btn_enter->setEnabled(true);
    if (body.size() < (int)sizeof(UPDATE_DEPT_RESP)) return;
    const auto* resp = reinterpret_cast<const UPDATE_DEPT_RESP*>(body.constData());

    if (resp->code == 0) {
        QMessageBox::information(this, "提示", "修改成功");
        accept();
    } else {
        QMessageBox::warning(this, "提示", QString::fromUtf8(resp->message));
    }
}


// ===============================================================================================
// ====================================== 网络请求 =========================================
// ===============================================================================================

void AddDepartmentDialog::requestDoctors()
{
    // 编辑模式下  需要额外带回该科室的主任和副主任医师
    CHIEF_DOC_REQ req = {};
    req.current_dept_id = (m_deptId > 0) ? m_deptId : 0;
    HEAD head1 = { SERVICE_TYPE::GET_CHIEF_DOCTORS, sizeof(req)};
    QByteArray pkt1;
    pkt1.append((char*)&head1, sizeof(HEAD));
    pkt1.append((char*)&req, sizeof(req));
    m_netconn->sendPacket(pkt1);

    // 编辑模式下  获取未分配的所有医生信息 + 该科室的成员信息
    EMPTY_DOCTOR_REQ req1 = {};
    req1.current_dept_id = (m_deptId>0) ? m_deptId : 0;
    HEAD head2 = { SERVICE_TYPE::GET_UNASSIGNED_DOCTORS, sizeof(req1)};
    QByteArray pkt2;
    pkt2.append((char*)&head2, sizeof(HEAD));
    pkt2.append((char*)&req1, sizeof(req1));
    m_netconn->sendPacket(pkt2);
}

void AddDepartmentDialog::requestDeptDetail(int deptId)
{
    GET_DEPT_DETAIL_REQ req = {};
    req.dept_id = deptId;

    HEAD head = { SERVICE_TYPE::GET_DEPT_DETAIL, sizeof(req) };
    QByteArray pkt;
    pkt.append((char*)&head, sizeof(HEAD));
    pkt.append((char*)&req, sizeof(req));
    m_netconn->sendPacket(pkt);
}



// ===============================================================================================
// ====================================== 按钮槽函数 =========================================
// ===============================================================================================

// 添加成员
void AddDepartmentDialog::on_btn_add_clicked()
{
    int docId = ui->doc_combo->currentData().toInt();
    if (docId <= 0 || m_addedMemberIds.contains(docId)) return;

    QString docDisplay = ui->doc_combo->currentText();

    // 记录并移除下拉项
    m_addedMemberIds.append(docId);
    ui->doc_combo->removeItem(ui->doc_combo->currentIndex());

    addMemberTag(docId, docDisplay);

    // 重新排列
    relayoutMembers();
}

void AddDepartmentDialog::on_btn_enter_clicked()
{
    // 1. 数据校验
    QString deptName = ui->edit_dept->text().trimmed();
    QString desc = ui->edit_detail->text().trimmed();
    int leaderId = ui->mainDoc_combo->currentData().toInt();

    if (deptName.isEmpty() || leaderId <= 0) {
        QMessageBox::warning(this, "提示", "科室名称和负责人不能为空");
        return;
    }

    if(m_deptId > 0){
        // ===== 编辑模式提交 =====
        UPDATE_DEPT_REQ req = {};
        req.dept_id = m_deptId;
        req.leader_id = leaderId;
        strncpy(req.dept_name, deptName.toUtf8().constData(), sizeof(req.dept_name) - 1);
        strncpy(req.profile, desc.toUtf8().constData(), sizeof(req.profile) - 1);
        req.member_count = m_addedMemberIds.size();

        int bodyLen = sizeof(UPDATE_DEPT_REQ) + req.member_count * sizeof(int);
        HEAD head = { SERVICE_TYPE::UPDATE_DEPT, bodyLen };
        QByteArray packet;
        packet.resize(sizeof(HEAD) + bodyLen);
        char* p = packet.data();
        memcpy(p, &head, sizeof(HEAD)); p += sizeof(HEAD);
        memcpy(p, &req, sizeof(req));   p += sizeof(req);
        for (int id : m_addedMemberIds) {
            memcpy(p, &id, sizeof(int));
            p += sizeof(int);
        }
        m_netconn->sendPacket(packet);
    }
    else{
        ADD_DEPT_REQ req = {};
        req.leader_id = leaderId;
        strncpy(req.deptName, deptName.toUtf8().constData(), sizeof(req.deptName) - 1);
        strncpy(req.Profile, desc.toUtf8().constData(), sizeof(req.Profile) - 1);
        req.member_count = m_addedMemberIds.size();

        int bodyLen = sizeof(ADD_DEPT_REQ) + req.member_count * sizeof(int);
        HEAD head = { SERVICE_TYPE::ADD_DEPARTMENT, bodyLen };

        QByteArray packet;
        packet.resize(sizeof(HEAD) + bodyLen);
        char* p = packet.data();

        memcpy(p, &head, sizeof(HEAD));  p += sizeof(HEAD);
        memcpy(p, &req, sizeof(req));    p += sizeof(req);
        for (int id : m_addedMemberIds) {
            memcpy(p, &id, sizeof(int));
            p += sizeof(int);
        }
        m_netconn->sendPacket(packet);
    }
    ui->btn_enter->setEnabled(false);
}


// ===============================================================================================
// ====================================== 工具函数 =========================================
// ===============================================================================================

void AddDepartmentDialog::relayoutMembers()
{
    // 1. 把所有标签从布局中移除  根据更新的 m_addedMemberIds、m_memberTags重新添加
    for (QWidget* tag : m_memberTags) {
        ui->gridLayout_members->removeWidget(tag);
    }

    // 2. 重新计算行列并添加
    int row = 0, col = 0;
    const int MAX_COLUMNS = 3;

    for (int id : m_addedMemberIds) {
        QWidget* tag = m_memberTags.value(id);
        if (!tag) continue;

        // 重新塞进网格
        ui->gridLayout_members->addWidget(tag, row, col);

        col++;
        if (col >= MAX_COLUMNS) {
            col = 0;
            row++;
        }
    }
}

void AddDepartmentDialog::tryRenderEditData()
{
    if (!m_chiefLoaded || !m_memberLoaded) return;
    // 2. 统一
    if (!m_warningShown) {
        m_warningShown = true; // 加上这个锁，保证绝对不弹第二次

        if (m_chiefEmpty && m_memberEmpty) {
            QMessageBox::warning(this, "提示", "当前系统内暂无任何可分配的医生！\n请先去【医生档案管理】添加医生。");
        } else if (m_chiefEmpty) {
            QMessageBox::warning(this, "提示", "暂无可用的主任/副主任医师作为负责人！");
        } else if (m_memberEmpty) {
            QMessageBox::information(this, "提示", "当前没有未分配科室的医生，您可以先选择负责人。");
        }
    }


    if (m_deptId <= 0) return;
    if (!m_detailLoaded || !m_chiefLoaded || !m_memberLoaded) return;
    // 1. 渲染负责人
    int leaderIdx = ui->mainDoc_combo->findData(m_tempLeaderId);
    if (leaderIdx >= 0) ui->mainDoc_combo->setCurrentIndex(leaderIdx);

    for (int id : m_tempMemberIds) { 
        if (id == m_tempLeaderId) continue;
        // 移除医生列表中的该科室成员
        int idx = ui->doc_combo->findData(id);
        if (idx >= 0) ui->doc_combo->removeItem(idx);

        // 记录并画标签
        m_addedMemberIds.append(id);
        QString docDisplay = m_allDoctors.value(id);
        addMemberTag(id, docDisplay);
    }
    int curLeaderId = ui->mainDoc_combo->currentData().toInt();
    if (curLeaderId > 0) {
        int idx = ui->doc_combo->findData(curLeaderId);
        if (idx >= 0) ui->doc_combo->removeItem(idx);
    }
    relayoutMembers();
}





