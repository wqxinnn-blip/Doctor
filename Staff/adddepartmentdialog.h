#ifndef ADDDEPARTMENTDIALOG_H
#define ADDDEPARTMENTDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QFile>
#include "../protocol.h"
#include "../netconn.h"

namespace Ui {
class AddDepartmentDialog;
}

class AddDepartmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDepartmentDialog(int deptId = -1, QWidget *parent = nullptr);
    ~AddDepartmentDialog();

private slots:

    void onChiefDoctorsReceived(const QByteArray& body);
    void onUnassignedDoctorsReceived(const QByteArray& body);
    void onAddDepartmentReceived(const QByteArray& body);

    void on_btn_add_clicked();
    void on_btn_enter_clicked();

    void onDeptDetailReceived(const QByteArray& body);
    void onUpdateDepartmentReceived(const QByteArray& body);   // 接收修改响应

private:

    void init_qss();
    void requestDoctors();           // 请求医生列表
    void requestDeptDetail(int deptId);
    void relayoutMembers();           // 重新排列网格的函数
    void tryRenderEditData();
    void addMemberTag(int docId, const QString& docDisplay); // 抽离画标签逻辑

    Ui::AddDepartmentDialog *ui;
    NetConn *m_netconn;
    int m_deptId;                    // 当前科室ID

    QList<int> m_addedMemberIds;      // 记录已添加的医生 ID，防止重复
    QMap<int, QString> m_allDoctors; // 所有医生信息（编辑模式下即 该科室所有医生和未分配的医生
    QMap<int, QWidget*> m_memberTags; // 记录 ID 对应的标签控件


    // 编辑回显控制标志位
    bool m_detailLoaded = false;
    bool m_chiefLoaded = false;
    bool m_memberLoaded = false;
    int m_tempLeaderId = -1;
    QList<int> m_tempMemberIds;

    bool m_chiefEmpty = false;
    bool m_memberEmpty = false;
    bool m_warningShown = false;
};

#endif // ADDDEPARTMENTDIALOG_H
