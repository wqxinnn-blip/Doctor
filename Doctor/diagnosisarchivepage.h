#ifndef DIAGNOSISARCHIVEPAGE_H
#define DIAGNOSISARCHIVEPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QDesktopServices>
#include "dataTypes.h"
#include "../netconn.h"
#include "qlistwidget.h"
#include "uploadreporttask.h"

namespace Ui {
class DiagnosisArchivePage;
}

class DiagnosisArchivePage : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosisArchivePage(QWidget *parent = nullptr);
    ~DiagnosisArchivePage();

    // 外部调用：加载患者
    void loadPatient(int appointmentId, int patientId,int tongue_record_id);

    // 页面进入/离开（如果导航框架需要）
    void onEnter();
    void onLeave();

signals:
    // 生成报告成功后通知主窗口刷新预约列表
    void reportGenerated(int appointmentId);

private slots:
    // 网络响应
    void onPatientInfoReceived(const QByteArray& body);
    void onHistoryReportReceived(const QByteArray& body);
    void onSaveDiagnosisResp(const QByteArray& body);
    void onGenerateReportResp(const QByteArray& body);

    void onReportUploadFinished(bool flag,QString msg,QString fullPath);

    // 按钮 查看报告、保存、生成报告、添加药品
    void onViewReportClicked();
    void onSaveClicked();
    void onGenerateReportClicked();
    void onAddMedicineClicked();
    // 附加功能
    void onMoreHistoryClicked();
    void onCameraDetectClicked();

private:
    void setupUi();
    void setupLeftLayout(QVBoxLayout* root);
    void setupRightLayout(QVBoxLayout* root);
    void applyQss();
    void clearInputs();
    bool validateInputs(QString& errMsg);
    void updateMedicineLabel();
    QString generateLocalReport();
    void updateHistoryUI();

    // 网络请求
    void requestPatientInfo(int patientId);
    void requestHistoryReport(int patientId);
    void requestSaveDiagnosis();
    void uploadReportFile(const QString& localPath);

    // UI 组件
    QLabel*      m_labPatientName = nullptr;
    QLabel*      m_labPatientAge = nullptr;
    QLabel*      m_labPatientPhone = nullptr;

    QLabel*      m_labHistoryDate = nullptr;
    QLabel*      m_labHistoryResult = nullptr;
    QLabel*      m_labHistoryTips = nullptr;
    QLabel*      m_labHistorySuggestion = nullptr;
    QPushButton* m_btnMoreHistory = nullptr;

    QTextEdit*   m_editResult = nullptr;
    QTextEdit*   m_editSuggestion = nullptr;
    QListWidget *m_medicineList = nullptr;
    QPushButton* m_btnAddMedicine = nullptr;


    QPushButton* m_btnViewReport = nullptr;
    QPushButton* m_btnCameraDetect = nullptr;
    QPushButton* m_btnSave = nullptr;
    QPushButton* m_btnReport = nullptr;

    QLabel* m_labStatus = nullptr;

    NetConn*     m_netConn = nullptr;
    PatientBriefInfo m_patient;
    HistoryReport    m_lastHistory;

    // 保存成功后记录，给"生成报告"用
    int m_savedRecordId = -1;
    // 药方明细表
    QList<PrescriptionItemInfo> m_prescriptionItems;
    // 上传文件
    UploadReportTask *m_reportUploadTask = nullptr;
};

#endif // DIAGNOSISARCHIVEPAGE_H
