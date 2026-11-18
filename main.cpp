#include <QApplication>
#include "winmanager.h"
#include "appconfig.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString cfgPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
    AppConfig cfg = AppConfig::load(cfgPath);

    WinManager::getInstance()->setAppConfig(cfg);
    NetConn* netConn = WinManager::getInstance()->getNetConn();
    netConn->connectToServer(cfg.businessIp, cfg.businessPort, cfg.reconnectInterval);

    WinManager::getInstance()->getLoginDialog()->show();
    return QApplication::exec();
}
