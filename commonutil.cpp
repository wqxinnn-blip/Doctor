#include "commonutil.h"
#include <QCryptographicHash>

CommonUtil::CommonUtil() {}

QString CommonUtil::getMd5(const QString &plainText)
{
    QByteArray data = plainText.toUtf8();
    QByteArray md5Bin = QCryptographicHash::hash(data,QCryptographicHash::Md5);
    return QString::fromLatin1(md5Bin.toHex());
}
