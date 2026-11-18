#ifndef DEPTMANAGERWIDGET_H
#define DEPTMANAGERWIDGET_H

#include <QWidget>
#include <QFile>
#include "../netconn.h"

namespace Ui {
class DeptManagerWidget;
}

class DeptManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeptManagerWidget(QWidget *parent = nullptr);
    ~DeptManagerWidget();

    void onEnter();
private:
    Ui::DeptManagerWidget *ui;
    NetConn *m_netconn = nullptr;

    // 分页变量
    int m_currentPage = 1;
    int m_totalPage;
    int m_pageSize = 9;
    // 初始化
    void init_list();
    void init_qss();
    // 刷新科室网格显示
    void refreshDeptData();

private slots:
    void onDeptArchiveDataReceived(const QByteArray& body);
    void on_btn_prev_clicked();
    void on_btn_next_clicked();
    void on_btn_add_clicked();
    void on_btn_edit_clicked();
};

#endif // DEPTMANAGERWIDGET_H
