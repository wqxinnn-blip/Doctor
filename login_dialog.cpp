#include "login_dialog.h"
#include "ui_login_dialog.h"
#include "winmanager.h"

Login_Dialog::Login_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login_Dialog)
{
    ui->setupUi(this);

    init_qss();
    init_login();  
}

Login_Dialog::~Login_Dialog()
{
    delete ui;
}

void Login_Dialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (m_connected) return;

    m_netConn = WinManager::getInstance()->getNetConn();
    if (!m_netConn) {
        qWarning() << "NetConn 为空";
        return;
    }
    connect(m_netConn,&NetConn::sigDataToLogin,this,&Login_Dialog::onLoginResp);
    m_connected = true;
}

void Login_Dialog::init_qss()
{
    //1、打开文件
    QFile file(":/qss/style.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void Login_Dialog::init_login()
{
    // 账户输入框左侧图标
    QAction *actUser = new QAction(this);
    actUser->setIcon(QIcon(":/image/login_img/yonghu.png").pixmap(32, 32));
    ui->edit_user->addAction(actUser, QLineEdit::LeadingPosition);

    // 密码输入框左侧图标
    QAction *actPwd = new QAction(this);
    actPwd->setIcon(QIcon(":/image/login_img/mima.png").pixmap(32, 32));
    ui->edit_pwd->addAction(actPwd, QLineEdit::LeadingPosition);
}

void Login_Dialog::on_btn_login_clicked()
{
    // 1、获取账号和密码
    QString work_Id = ui->edit_user->text().trimmed();
    QString pwd = ui->edit_pwd->text().trimmed();
    // 2、长度校验
    if(work_Id.length() ==0 ){
        ui->lab_tip_time->setText(QString("工号不能为空！"));
        return;
    }
    // 3、密码校验
    if(pwd.isEmpty()){
        ui->lab_tip_time->setText(QString("密码不能为空！"));
        return;
    }
    if(pwd.length() <6 || pwd.length()>10)
    {
        ui->lab_tip_time->setText(QString("密码长度必须为6~10位！"));
        return;
    }
    // 5、校验通过，打包传输
    HEAD head = {SERVICE_TYPE::STAFF_LOGIN,sizeof(STAFF_LOGIN_REQ)};
    STAFF_LOGIN_REQ login_req = {};
    strcpy(login_req.work_id,work_Id.toStdString().c_str());
    // 密码加密
    QString pwdMd5 = CommonUtil::getMd5(pwd);
    strcpy(login_req.password,pwdMd5.toStdString().c_str());

    QByteArray byteArr;
    byteArr.resize(sizeof(HEAD)+head.len);
    char* data = byteArr.data();
    memcpy(data, &head, sizeof(HEAD));
    memcpy(data + sizeof(HEAD), &login_req, sizeof(STAFF_LOGIN_REQ));

    // 6、发送
    qint64 len = m_netConn->sendPacket(byteArr);
    qDebug()<<"发送 "<<len<<" 字节！";
}

void Login_Dialog::onLoginResp(const QByteArray &body)
{
    // 1、解析包体数据
    STAFF_LOGIN_RESP login_resp = {};
    char *recv_buf = (char*)body.data();
    memcpy(&login_resp,recv_buf,sizeof(STAFF_LOGIN_RESP));

    ui->lab_tip_time->setText(login_resp.msg);
    qDebug()<<"验证结果:"<<login_resp.msg;
    // 2、验证
    if(login_resp.result == 1){
        qDebug()<<"提取个人信息";
        Staff_Info info;
        info.setName(login_resp.name);
        info.setPhone(login_resp.phone);

        info.setStaff_id(login_resp.staff_id);

        WinManager::getInstance()->setStaff_Info(info);

        //判断权限
        WinManager *wm = WinManager::getInstance();
        if(login_resp.role == 1){
            qDebug()<<"进入医生主界面";
            wm->getDocMainWindow()->show();
        }
        else{
            qDebug()<<"进入管理端主界面";
            wm->getStaffMainWIndow()->show();
        }
        this->hide();
    }
}



