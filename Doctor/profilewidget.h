#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <QWidget>
#include <QFile>
#include <QCryptographicHash>
#include "../netconn.h"

namespace Ui {
class ProfileWidget;
}

class ProfileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileWidget(QWidget *parent = nullptr);
    ~ProfileWidget();

private slots:
    void on_btn_submit_clicked();

private:
    Ui::ProfileWidget *ui;
    NetConn *m_netconn = nullptr;


    void init_qss();
    void setProfile();

    void requestModifyPwd();
    void onModifyPwdReceived(const QByteArray& body);
};

#endif // PROFILEWIDGET_H
