#ifndef DOCTORSCHEDULEWIDGET_H
#define DOCTORSCHEDULEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QDate>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include "../netconn.h"

class DoctorScheduleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DoctorScheduleWidget(QWidget *parent = nullptr);
    void onEnter();

private slots:
    void onPrevWeek();
    void onNextWeek();
    void onDataReceived(const QByteArray& body);

private:
    void setupUi();
    void init_qss();
    void initTable();
    void refreshData();
    void updateDateLabel();

    NetConn* m_netconn = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_labDateRange = nullptr;

    QDate m_currentMonday; // 记录当前显示的周一日期
};

#endif // DOCTORSCHEDULEWIDGET_H
