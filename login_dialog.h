#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QRegularExpression>
#include "netconn.h"
#include "commonutil.h"
#include "staff_info.h"

namespace Ui {
class Login_Dialog;
}

class Login_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Login_Dialog(QWidget *parent = nullptr);
    ~Login_Dialog();

protected:
    void showEvent(QShowEvent *event);

private slots:
    void on_btn_login_clicked();
    void onLoginResp(const QByteArray& body);

private:
    Ui::Login_Dialog *ui;

    void init_qss();
    void init_login();

    NetConn *m_netConn;
    bool m_connected = false;
};

#endif // LOGIN_DIALOG_H
