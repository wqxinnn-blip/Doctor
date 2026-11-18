#include "doc_mainwindow.h"
#include "ui_doc_mainwindow.h"

Doc_MainWindow::Doc_MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Doc_MainWindow)
{
    ui->setupUi(this);
    init_qss();
    initRightContent();
}

Doc_MainWindow::~Doc_MainWindow()
{
    delete ui;
}

void Doc_MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if(m_firstShow)
    {
        m_firstShow = false;
        //qDebug() << "窗口首次显示，选中第0行";
        ui->listWidget->setCurrentRow(0);
    }
}


void Doc_MainWindow::init_qss()
{
    //1、打开文件
    QFile file(":/qss/mainWindow.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}


void Doc_MainWindow::initRightContent()
{
    QVBoxLayout* rightLayout = new QVBoxLayout(ui->right_widgte);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_contentStack = new QStackedWidget(this);
    rightLayout->addWidget(m_contentStack);

    // [0] 预约患者诊疗
    m_appointmentPage = new AppointmentListPage(this);
    m_contentStack->addWidget(m_appointmentPage);

    // [1] 诊疗档案管理
    m_diagnosisPage = new DiagnosisArchivePage(this);
    m_contentStack->addWidget(m_diagnosisPage);

    // [2] 个人排班查看
    m_doctorSchedule = new DoctorScheduleWidget(this);
    m_contentStack->addWidget(m_doctorSchedule);

    // [3] 个人资料管理
    m_profileWidget = new ProfileWidget(this);
    m_contentStack->addWidget(m_profileWidget);


    m_contentStack->setCurrentIndex(0);

    // 连接问诊信号
    connect(m_appointmentPage, &AppointmentListPage::startConsultation,
            this, &Doc_MainWindow::onStartConsultation);
}



void Doc_MainWindow::on_listWidget_currentRowChanged(int currentRow)
{
    // 离开旧页面
    int old = m_contentStack->currentIndex();

    if (old == 0 && m_appointmentPage) {
        m_appointmentPage->onLeave();
    }
    else if (old == 1 && m_diagnosisPage) {
        m_diagnosisPage->onLeave();
    }

    // 切换
    m_contentStack->setCurrentIndex(currentRow);


    // 进入新页面
    if (currentRow == 0 && m_appointmentPage) {
        m_appointmentPage->onEnter();
    }
    else if (currentRow == 1 && m_diagnosisPage) {
        // 诊疗档案页 通过 loadPatient 触发
    }
    else if (currentRow == 2 && m_doctorSchedule) {
        m_doctorSchedule->onEnter();
    }
    else if (currentRow == 3 && m_profileWidget) {

    }

    // currentRow == 2 / 3 占位页，暂无处理

}


void Doc_MainWindow::onStartConsultation(int appointmentId, int patientId,int tongue_record_id)
{
    // 先加载患者数据
    m_diagnosisPage->loadPatient(appointmentId, patientId,tongue_record_id);

    // 切菜单
    ui->listWidget->setCurrentRow(1);
}