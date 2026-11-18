#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QSettings>
#include <QFile>
#include <QDebug>

class AppConfig
{
public:
    AppConfig();

    // 业务服务器
    QString businessIp;
    int     businessPort = 0;
    int     reconnectInterval = 2000;

    static AppConfig load(const QString& path);
};

#endif // APPCONFIG_H
