#ifndef STAFF_MAINWINDOW_H
#define STAFF_MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QListWidgetItem>
#include <QStackedWidget>
#include "schedulemanagepage.h"
#include "../Doctor/profilewidget.h"
#include "syslogwidget.h"
#include "deptmanagerwidget.h"
#include "workloadstatswidget.h"
#include "doctormanagementwidget.h"


namespace Ui {
class Staff_MainWindow;
}

class Staff_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit Staff_MainWindow(QWidget *parent = nullptr);
    ~Staff_MainWindow();

private slots:
    void on_listWidget_currentRowChanged(int currentRow);

private:
    Ui::Staff_MainWindow *ui;

    void init_qss();
    void initRightContent();

    QStackedWidget    *m_contentStack;   // 右侧页面堆叠管理器
    ScheduleManagePage *m_schedulePage;  // 排班管理整页（内部包含周/月视图）
    ProfileWidget *m_profileWidget; //个人资料
    SysLogWidget *m_syslogWidget; //系统日志
    DeptManagerWidget *m_deptWidget; //科室档案
    WorkloadStatsWidget *m_workloadWidget; //诊疗工作量统计
    DoctorManagementWidget *m_docManagerWidget; //医师档案
};

#endif // STAFF_MAINWINDOW_H
