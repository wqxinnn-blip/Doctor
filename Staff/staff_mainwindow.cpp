#include "staff_mainwindow.h"
#include "ui_staff_mainwindow.h"

Staff_MainWindow::Staff_MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Staff_MainWindow)
{
    ui->setupUi(this);

    init_qss();
    initRightContent();

}

Staff_MainWindow::~Staff_MainWindow()
{
    delete ui;
}

// ===============================================================================================
// ====================================== 初始化 ========================
// ===============================================================================================

void Staff_MainWindow::init_qss()
{
    //1、打开文件
    QFile file(":/qss/mainWindow.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void Staff_MainWindow::initRightContent()
{
    // 1. 给右侧 widget设置布局
    QVBoxLayout *rightLayout = new QVBoxLayout(ui->right_widget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // 2. 创建堆叠窗口管理多页面
    m_contentStack = new QStackedWidget(this);
    rightLayout->addWidget(m_contentStack);


    m_deptWidget = new DeptManagerWidget(this);
    m_contentStack->addWidget(m_deptWidget); // 0: 科室档案管理

    m_docManagerWidget = new DoctorManagementWidget(this);
    m_contentStack->addWidget(m_docManagerWidget); // 1: 医师档案管理

    m_schedulePage = new ScheduleManagePage(this);
    m_contentStack->addWidget(m_schedulePage);    // 2: 排班调度管理

    m_workloadWidget = new WorkloadStatsWidget(this);
    m_contentStack->addWidget(m_workloadWidget); // 3: 诊疗工作量统计

    m_syslogWidget = new SysLogWidget(this);
    m_contentStack->addWidget(m_syslogWidget); // 4: 系统日志统计

    m_profileWidget = new ProfileWidget(this);
    m_contentStack->addWidget(m_profileWidget); // 5: 个人资料管理
}


// ===============================================================================================
// ====================================== 列表槽函数 （切换到不同功能界面）========================
// ===============================================================================================

void Staff_MainWindow::on_listWidget_currentRowChanged(int currentRow)
{
    // 根据左侧的行号，映射到右侧的 Widget
    switch (currentRow) {
    case 0: //科室
        m_contentStack->setCurrentWidget(m_deptWidget);
        m_deptWidget->onEnter();
        break;
    case 1: //医师
        m_contentStack->setCurrentWidget(m_docManagerWidget);
        break;
    case 2: //排班
        m_contentStack->setCurrentWidget(m_schedulePage);
        m_schedulePage->onEnter();
        break;
    case 3: //工作量
        m_contentStack->setCurrentWidget(m_workloadWidget);
        m_workloadWidget->onEnter();
        break;
    case 4: //日志
        m_contentStack->setCurrentWidget(m_syslogWidget);
        m_syslogWidget->onEnter();
        break;
    case 5: //个人资料
        m_contentStack->setCurrentWidget(m_profileWidget);
        break;
    default:
        break;
    }
}

