#ifndef DOC_MAINWINDOW_H
#define DOC_MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "appointmentlistpage.h"
#include "diagnosisarchivepage.h"
#include "doctorschedulewidget.h"
#include "profilewidget.h"
#include <QFile>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace Ui {
class Doc_MainWindow;
}

class Doc_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit Doc_MainWindow(QWidget *parent = nullptr);
    ~Doc_MainWindow();
protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_listWidget_currentRowChanged(int currentRow);
    void onStartConsultation(int appointmentId, int patientId,int tongue_record_id);

private:
    Ui::Doc_MainWindow *ui;

    void init_qss();
    void initRightContent();

    bool m_firstShow = true;

    QStackedWidget* m_contentStack = nullptr;

    AppointmentListPage* m_appointmentPage = nullptr;   // [0]
    DiagnosisArchivePage* m_diagnosisPage = nullptr;    // [1]
    DoctorScheduleWidget* m_doctorSchedule = nullptr;   // [2]
    ProfileWidget* m_profileWidget = nullptr;           // [3]


};

#endif // DOC_MAINWINDOW_H
