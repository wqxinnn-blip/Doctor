#ifndef COMMONUTIL_H
#define COMMONUTIL_H

#include <QString>
#include <QByteArray>

class CommonUtil
{
public:
    CommonUtil();

    // MD5加密
    static QString getMd5(const QString &plainText);
};

#endif // COMMONUTIL_H
