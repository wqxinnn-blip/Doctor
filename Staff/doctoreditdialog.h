#ifndef DOCTOREDITDIALOG_H
#define DOCTOREDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QFile>
#include <QStringList>
#include <QMessageBox>
#include "dataTypes.h"

enum class DialogMode {
    AddMode,  // 新增
    EditMode  // 修改
};

namespace Ui {
class DoctorEditDialog;
}

class DoctorEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DoctorEditDialog(const QList<DepartmentInfo>& deptList, QWidget *parent = nullptr);
    ~DoctorEditDialog();

    // 专用于新增：只传工号，其他留空
    void setupForAdd(const QString& newWorkNo);
    // 专用于修改：传入完整旧数据回显
    void setupForEdit(const LocalDoctorData &data);
    LocalDoctorData getDoctorData() const;

private slots:
    void on_btn_enter_clicked();  // 确认按钮
    void on_btn_cancel_clicked(); // 取消按钮

private:
    Ui::DoctorEditDialog *ui;
    QList<DepartmentInfo> m_deptList;

    void init_qss();

};

#endif // DOCTOREDITDIALOG_H
