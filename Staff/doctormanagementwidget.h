#ifndef DOCTORMANAGEMENTWIDGET_H
#define DOCTORMANAGEMENTWIDGET_H

#include <QWidget>
#include <QList>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QSpinBox>
#include <QFile>
#include <QString>
#include "dataTypes.h"

namespace Ui {
class DoctorManagementWidget;
}

class DoctorManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorManagementWidget(QWidget *parent = nullptr);
    ~DoctorManagementWidget();
protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onDocDataReceived(const QByteArray& body); // 接收医生数据
    void onDeptDataReceived(const QByteArray& body); // 接收科室数据
    void onDelDocReceived(const QByteArray& body); //删除响应
    void onNewDoctorIdReceived(const QByteArray& body); //新工号响应
    void onModifyDoctorResp(const QByteArray& body);    // 处理新增/修改结果

    void onEditClicked(const LocalDoctorData& doc);
    void onDeleteClicked(QString workNo);

    void on_btn_search_clicked();
    void on_btn_addDoc_clicked();
    void on_btn_prev_clicked();
    void on_btn_next_clicked();

private:
    Ui::DoctorManagementWidget *ui;

    void init_qss();

    QList<LocalDoctorData> m_allDoctors;
    QList<DepartmentInfo> m_allDept;
    QList<LocalDoctorData> m_filteredDoctors;

    int m_pageSize = 10;
    int m_currentPage = 1;
    int m_totalPage = 1;

    void loadTableData();

    void requestAllDoctors();
    void requestALLDepts();
    void sendDoctorData(int action, const LocalDoctorData& doc);
};

#endif // DOCTORMANAGEMENTWIDGET_H
