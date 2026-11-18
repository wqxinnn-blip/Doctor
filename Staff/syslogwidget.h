#ifndef SYSLOGWIDGET_H
#define SYSLOGWIDGET_H

#include <QWidget>
#include <QFile>
#include <QCalendarWidget>
#include "../netconn.h"

namespace Ui {
class SysLogWidget;
}

class SysLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SysLogWidget(QWidget *parent = nullptr);
    ~SysLogWidget();

    void onEnter();

private slots:
    void on_btn_search_clicked();
    void onLogDataReceived(const QByteArray& body);
    void on_btn_prev_clicked();
    void on_btn_next_clicked();

    void on_btn_remove_clicked();

private:
    Ui::SysLogWidget *ui;

    void init_qss();
    void initTable();
    void onPrevPage();
    void onNextPage();

    void requestLogList();

    NetConn* m_netconn = nullptr;

    // 分页数据
    int m_currentPage = 1;
    int m_totalPage = 1;
    int m_pageSize = 15; // 每页显示 15 条

    bool m_isAllDate = true;
};

#endif // SYSLOGWIDGET_H
