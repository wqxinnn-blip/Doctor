#include "appconfig.h"

AppConfig::AppConfig() {}

AppConfig AppConfig::load(const QString &path)
{
    AppConfig cfg;
    if (!QFile::exists(path)) {
        qWarning() << "[Config] 配置文件不存在:" << path << "，使用默认值";
    }

    QSettings s(path, QSettings::IniFormat);

    // 业务服务器
    cfg.businessIp   = s.value("business/ip",   "127.0.0.1").toString();
    cfg.businessPort = s.value("business/port", 8888).toInt();
    cfg.reconnectInterval = s.value("business/reconnectInterval",2000).toInt();
    return cfg;
}

