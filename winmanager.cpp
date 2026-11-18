#include "winmanager.h"

WinManager::WinManager(QObject *parent)
    : QObject{parent}
{
    qDebug() << "WinManager 构造";
}

WinManager::~WinManager()
{
    delete login_dialog;
    login_dialog = nullptr;
    delete doc_mainWindow;
    doc_mainWindow = nullptr;
    delete staff_mainWIndow;
    staff_mainWIndow = nullptr;
}

WinManager *WinManager::getInstance()
{
    static WinManager instance;
    return &instance;
}

void WinManager::setStaff_Info(const Staff_Info &info)
{
    m_staffinfo = info;
}

const Staff_Info &WinManager::getStaff_Info()
{
    return m_staffinfo;
}

void WinManager::setAppConfig(const AppConfig &cfg)
{
    m_appConfig = cfg;
}

const AppConfig &WinManager::getAppConfig()
{
    return m_appConfig;
}


NetConn* WinManager::getNetConn()
{
    if (!m_netConn) {
        m_netConn = new NetConn();
    }
    return m_netConn;
}

Login_Dialog* WinManager::getLoginDialog()
{
    if (!login_dialog) {
        login_dialog = new Login_Dialog();
    }
    return login_dialog;
}

Doc_MainWindow *WinManager::getDocMainWindow()
{
    if (!doc_mainWindow) {
        doc_mainWindow = new Doc_MainWindow();
    }
    return doc_mainWindow;
}

Staff_MainWindow *WinManager::getStaffMainWIndow()
{
    if (!staff_mainWIndow) {
        staff_mainWIndow = new Staff_MainWindow();
    }
    return staff_mainWIndow;
}




