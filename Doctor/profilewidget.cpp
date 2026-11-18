#include "profilewidget.h"
#include "ui_profilewidget.h"
#include "winmanager.h"

ProfileWidget::ProfileWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProfileWidget)
{
    ui->setupUi(this);
    init_qss();
    setProfile();

    this->setAttribute(Qt::WA_StyledBackground, true);
    m_netconn = WinManager::getInstance()->getNetConn();

    connect(m_netconn,&NetConn::sigDataToModifyPwd,this,&ProfileWidget::onModifyPwdReceived);
}

ProfileWidget::~ProfileWidget()
{
    delete ui;
}

void ProfileWidget::init_qss()
{
    QFile file(":/qss/profile.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void ProfileWidget::setProfile()
{
    WinManager *wm = WinManager::getInstance();
    ui->edit_name->setText(wm->getStaff_Info().getName());
    ui->edit_phone->setText(wm->getStaff_Info().getPhone());
}

void ProfileWidget::requestModifyPwd()
{
    QString oldpwd = ui->edit_oldpwd->text().trimmed();
    QString newpwd = ui->edit_newpwd->text().trimmed();
    QString verifyPwd = ui->edit_verify->text().trimmed();

    if(oldpwd.isEmpty() || newpwd.isEmpty() || verifyPwd.isEmpty()){
        QMessageBox::information(this,"提示","密码不能为空");
        return;
    }
    if(newpwd != verifyPwd){
        QMessageBox::information(this,"提示","两次密码输入不一致");
        return;
    }
    if(newpwd.size() < 6 || newpwd.size() > 10){
        QMessageBox::information(this,"提示","密码长度不满足要求");
        return;
    }
    if(oldpwd == newpwd){
        QMessageBox::information(this,"提示","不能与原密码相同");
        return;
    }
    // 校验成功 发送
    QByteArray hashOld = QCryptographicHash::hash(oldpwd.toUtf8(), QCryptographicHash::Md5).toHex();
    QByteArray hashNew = QCryptographicHash::hash(newpwd.toUtf8(), QCryptographicHash::Md5).toHex();

    // 3. 组装协议数据
    MODIFY_PWD_REQ req = {};
    req.staff_id = WinManager::getInstance()->getStaff_Info().getStaff_id();

    snprintf(req.oldPwd, sizeof(req.oldPwd), "%s", hashOld.constData());
    snprintf(req.newPwd, sizeof(req.newPwd), "%s", hashNew.constData());

    // 4. 打包并发送
    HEAD head = { SERVICE_TYPE::MODIFYPWD, sizeof(MODIFY_PWD_REQ) };

    QByteArray packet;
    packet.resize(sizeof(HEAD) + sizeof(MODIFY_PWD_REQ));
    char* p = packet.data();

    memcpy(p, &head, sizeof(HEAD));
    memcpy(p + sizeof(HEAD), &req, sizeof(MODIFY_PWD_REQ));

    WinManager::getInstance()->getNetConn()->sendPacket(packet);

    qDebug() << "[ProfileWidget] 发送修改密码请求, staff_id =" << req.staff_id;
}

void ProfileWidget::onModifyPwdReceived(const QByteArray& body)
{
    if (body.size() < (int)sizeof(MODIFY_PWD_RESP)) return;

    const auto* resp = reinterpret_cast<const MODIFY_PWD_RESP*>(body.constData());
    if (resp->code == 0) {
        QMessageBox::information(this, "成功", "密码修改成功!");

        // 清空输入框
        ui->edit_oldpwd->clear();
        ui->edit_newpwd->clear();
        ui->edit_verify->clear();

        //WinManager::getInstance()->getNetConn()->setLoggedIn(false);

    } else {
        QMessageBox::warning(this, "失败", QString::fromUtf8(resp->message));
    }
}

void ProfileWidget::on_btn_submit_clicked()
{
    requestModifyPwd();
}

