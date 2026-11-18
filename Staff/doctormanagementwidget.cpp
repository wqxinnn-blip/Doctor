#include "doctormanagementwidget.h"
#include "ui_doctormanagementwidget.h"
#include "WinManager.h"
#include "doctoreditdialog.h"
#include "protocol.h"

DoctorManagementWidget::DoctorManagementWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DoctorManagementWidget)
{
    ui->setupUi(this);
    init_qss();
    // 初始化表格列宽
    ui->tableWidget->setColumnCount(6);
    ui->tableWidget->setHorizontalHeaderLabels({"工号", "医师姓名", "所属科室", "医师职称", "医师年龄", "操作"});
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(50);
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(5, 160);

    connect(WinManager::getInstance()->getNetConn(), &NetConn::sigDataToDoctorList,
            this, &DoctorManagementWidget::onDocDataReceived);
    connect(WinManager::getInstance()->getNetConn(),&NetConn::sigDataToDept,
            this,&DoctorManagementWidget::onDeptDataReceived);
    connect(WinManager::getInstance()->getNetConn(), &NetConn::sigDataToDelDocArchive,
            this, &DoctorManagementWidget::onDelDocReceived);
    connect(WinManager::getInstance()->getNetConn(), &NetConn::sigDataToNewDoctorId,
            this, &DoctorManagementWidget::onNewDoctorIdReceived);
    connect(WinManager::getInstance()->getNetConn(), &NetConn::sigDataToUpdateDoctor,
            this, &DoctorManagementWidget::onModifyDoctorResp);
}

DoctorManagementWidget::~DoctorManagementWidget()
{
    delete ui;
}

void DoctorManagementWidget::init_qss()
{
    //1、打开文件
    QFile file(":/qss/DocArchive.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void DoctorManagementWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (m_allDoctors.isEmpty()) {
        requestAllDoctors();
    }
    if (m_allDept.isEmpty()) {
        requestALLDepts();
    }
}


// ================================================================================================
// ====================================== 数据请求 ===================================
// ================================================================================================

void DoctorManagementWidget::requestAllDoctors() {

    GET_DOCTORS_REQ req = {};
    req.code = 1;

    HEAD head = {SERVICE_TYPE::GET_ALL_DOCTORS, sizeof(GET_DOCTORS_REQ)};

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(GET_DOCTORS_REQ));
    WinManager::getInstance()->getNetConn()->sendPacket(packet);
}

void DoctorManagementWidget::requestALLDepts()
{
    DEPARTMENT_LIST_REQ req = {};
    req.reserved = 1;

    HEAD head = {SERVICE_TYPE::DEPARTMENT_LIST, sizeof(DEPARTMENT_LIST_REQ)};

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(DEPARTMENT_LIST_REQ));
    WinManager::getInstance()->getNetConn()->sendPacket(packet);
}

void DoctorManagementWidget::sendDoctorData(int action, const LocalDoctorData &doc)
{
    MODIFY_DOCTOR_REQ req = {};
    req.action = action;
    strncpy(req.doctor.work_no, doc.workNo.toUtf8().constData(),sizeof(req.doctor.work_no) - 1);
    strncpy(req.doctor.real_name,doc.name.toUtf8().constData(),sizeof(req.doctor.real_name) - 1);
    strncpy(req.doctor.dept_name,doc.dept.toUtf8().constData(),sizeof(req.doctor.dept_name) - 1);
    strncpy(req.doctor.title,doc.title.toUtf8().constData(),sizeof(req.doctor.title) - 1);

    req.doctor.age = doc.age;

    HEAD head = { SERVICE_TYPE::UPDATE_DOCTOR, sizeof(MODIFY_DOCTOR_REQ)};

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(MODIFY_DOCTOR_REQ));

    WinManager::getInstance()->getNetConn()->sendPacket(packet);
    qDebug() << "发送医生数据请求，Action:" << action << ", 工号:" << doc.workNo;
}


// ================================================================================================
// ====================================== 数据响应 ===================================
// ================================================================================================

// 接收到的医师列表
void DoctorManagementWidget::onDocDataReceived(const QByteArray& body) {
    if (body.size() < (int)sizeof(GET_DOCTORS_RESP)) return;
    GET_DOCTORS_RESP* resp = (GET_DOCTORS_RESP*)body.constData();
    if (resp->code != 0) return;

    m_allDoctors.clear();
    const char* p = body.constData() + sizeof(GET_DOCTORS_RESP);
    for (int i = 0; i < resp->count; ++i) {
        DOCTOR_ITEM* item = (DOCTOR_ITEM*)p;
        LocalDoctorData doc;
        doc.workNo = QString::fromUtf8(item->work_no);
        doc.name = QString::fromUtf8(item->real_name);
        doc.dept = QString::fromUtf8(item->dept_name);
        doc.title = QString::fromUtf8(item->title);
        doc.age = item->age;
        m_allDoctors.append(doc);
        p += sizeof(DOCTOR_ITEM);
    }

    // 初始显示所有
    m_filteredDoctors = m_allDoctors;
    m_currentPage = 1;
    m_totalPage = (m_filteredDoctors.size() + m_pageSize - 1) / m_pageSize;
    if (m_totalPage == 0) m_totalPage = 1;
    loadTableData();
}


void DoctorManagementWidget::onDeptDataReceived(const QByteArray &body)
{
    // 解析数据包
    int count = body.size() / sizeof(DEPARTMENT_LIST_RESP);
    qDebug()<<"科室的数量："<<count;

    m_allDept.clear();
    const char* p = body.constData();
    for(int i=0;i<count;i++){
        const auto* item = reinterpret_cast<const DEPARTMENT_LIST_RESP*>(p);
        DepartmentInfo d;
        d.dept_id = item->dept_id;
        d.deptName = QString::fromUtf8(item->dept_name);
        m_allDept.append(d);
        p+=sizeof(DEPARTMENT_LIST_RESP);
    }
}

void DoctorManagementWidget::onDelDocReceived(const QByteArray &body)
{
    if (body.size() < (int)sizeof(SIMPLE_RESP)) return;
    SIMPLE_RESP* resp = (SIMPLE_RESP*)body.constData();
    if(resp->code == 0){
        QMessageBox::information(this,"提示",resp->msg);
        requestAllDoctors();
    }
    else{
        QMessageBox::information(this,"提示",resp->msg);
    }
}



void DoctorManagementWidget::loadTableData() {
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    ui->lab_pageInfo->setText(QString("%1 / %2").arg(m_currentPage).arg(m_totalPage));

    int startIndex = (m_currentPage - 1) * m_pageSize;
    int endIndex = qMin(startIndex + m_pageSize, m_filteredDoctors.size());

    for (int i = startIndex; i < endIndex; ++i) {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        const LocalDoctorData& doc = m_filteredDoctors[i];

        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(doc.workNo));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(doc.name));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(doc.dept));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(doc.title));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(doc.age)));

        // 操作按钮
        QWidget* widget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(10);
        layout->setAlignment(Qt::AlignCenter);

        QPushButton* btnEdit = new QPushButton("修改");
        QPushButton* btnDel = new QPushButton("删除");

        btnEdit->setFixedWidth(60);
        btnDel->setFixedWidth(60);

        layout->addWidget(btnEdit);
        layout->addWidget(btnDel);
        ui->tableWidget->setCellWidget(row, 5, widget);

        connect(btnEdit, &QPushButton::clicked, this, [this, doc](){ onEditClicked(doc); });
        connect(btnDel, &QPushButton::clicked, this, [this, workNo = doc.workNo](){ onDeleteClicked(workNo); });
    }
}

void DoctorManagementWidget::onNewDoctorIdReceived(const QByteArray& body)
{
    if (body.size() < (int)sizeof(NEW_DOCTOR_ID_RESP)) return;
    NEW_DOCTOR_ID_RESP* resp = (NEW_DOCTOR_ID_RESP*)body.constData();

    if (resp->code != 0) {
        QMessageBox::warning(this, "提示", QString::fromUtf8(resp->msg));
        return;
    }

    QString newWorkNo = QString::fromUtf8(resp->work_no);

    DoctorEditDialog dlg(m_allDept, this);
    dlg.setupForAdd(newWorkNo);

    if (dlg.exec() == QDialog::Accepted) {
        LocalDoctorData newDoc = dlg.getDoctorData();
        sendDoctorData(1, newDoc);
    }
}

void DoctorManagementWidget::onModifyDoctorResp(const QByteArray &body)
{
    if (body.size() < (int)sizeof(SIMPLE_RESP)) return;

    SIMPLE_RESP* resp = (SIMPLE_RESP*)body.constData();

    if (resp->code == 0) {
        QMessageBox::information(this, "成功", QString::fromUtf8(resp->msg));
        requestAllDoctors();
    } else {
        QMessageBox::warning(this, "失败", QString::fromUtf8(resp->msg));
    }
}


// ================================================================================================
// ====================================== 按钮槽函数 ===================================
// ================================================================================================
void DoctorManagementWidget::on_btn_search_clicked()
{
    QString workNo = ui->edit_workno->text().trimmed();
    QString name = ui->edit_name->text().trimmed();

    m_filteredDoctors.clear();

    for (const auto& doc : m_allDoctors) {
        bool matchWork = workNo.isEmpty() || doc.workNo.contains(workNo);
        bool matchName = name.isEmpty() || doc.name.contains(name);
        if (matchWork && matchName) {
            m_filteredDoctors.append(doc);
        }
    }

    m_currentPage = 1;
    m_totalPage = (m_filteredDoctors.size() + m_pageSize - 1) / m_pageSize;
    if (m_totalPage == 0) m_totalPage = 1;

    loadTableData();

    if (m_filteredDoctors.isEmpty()) {
        QMessageBox::information(this, "提示", "未搜索到相关数据");
    }
}

void DoctorManagementWidget::on_btn_addDoc_clicked()
{
    if (m_allDept.isEmpty()) {
        QMessageBox::warning(this, "提示", "科室数据尚未加载，请稍后再试！");
        return;
    }
    HEAD head = { SERVICE_TYPE::GET_NEW_DOCTOR_ID, 0 };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));

    WinManager::getInstance()->getNetConn()->sendPacket(packet);
}


void DoctorManagementWidget::on_btn_prev_clicked()
{
    if (m_currentPage <= 1) {
        QMessageBox::warning(this, "警告", "已经是第一页了！");
        return;
    }
    m_currentPage--;
    loadTableData();
}


void DoctorManagementWidget::on_btn_next_clicked()
{
    if (m_currentPage >= m_totalPage) {
        QMessageBox::warning(this, "警告", "已经是最后一页了！");
        return;
    }
    m_currentPage++;
    loadTableData();
}

void DoctorManagementWidget::onEditClicked(const LocalDoctorData &doc)
{
    DoctorEditDialog dlg(m_allDept, this);
    dlg.setupForEdit(doc);
    if (dlg.exec() == QDialog::Accepted) {
        LocalDoctorData updatedDoc = dlg.getDoctorData();
        sendDoctorData(2, updatedDoc);
        qDebug() << "正在提交修改请求...";
    }

}

void DoctorManagementWidget::onDeleteClicked(QString workNo)
{
    if (QMessageBox::question(this, "确认", "确定要删除该医师吗？") == QMessageBox::Yes) {
        DELETE_DOCTOR_REQ req;
        strncpy(req.work_no, workNo.toUtf8().constData(), 15);

        HEAD head = {SERVICE_TYPE::DELETE_DOCTOR, sizeof(req)};
        QByteArray packet;
        packet.append((const char*)&head, sizeof(head));
        packet.append((const char*)&req, sizeof(req));

        WinManager::getInstance()->getNetConn()->sendPacket(packet);
    }
}

