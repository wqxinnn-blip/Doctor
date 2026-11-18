#ifndef WINMANAGER_H
#define WINMANAGER_H

#include <QObject>
#include "netconn.h"
#include "login_dialog.h"
#include "staff_info.h"
#include "Doctor/doc_mainwindow.h"
#include "Staff/staff_mainwindow.h"
#include "appconfig.h"

class WinManager : public QObject
{
    Q_OBJECT
public:
    static WinManager *getInstance();

    WinManager(const WinManager&) = delete;
    WinManager& operator=(const WinManager&) = delete;


    void setStaff_Info(const Staff_Info &info);
    const Staff_Info& getStaff_Info();
    void setAppConfig(const AppConfig& cfg);
    const AppConfig& getAppConfig();

    NetConn*              getNetConn();
    Login_Dialog*         getLoginDialog();
    Doc_MainWindow*       getDocMainWindow();
    Staff_MainWindow*     getStaffMainWIndow();
private:
    explicit WinManager(QObject *parent = nullptr);
    ~WinManager();

    NetConn *m_netConn;
    Staff_Info m_staffinfo;
    AppConfig m_appConfig;

    Login_Dialog *login_dialog=nullptr;
    Doc_MainWindow *doc_mainWindow=nullptr;
    Staff_MainWindow *staff_mainWIndow=nullptr;
signals:
};

#endif // WINMANAGER_H
