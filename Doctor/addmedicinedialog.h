#ifndef ADDMEDICINEDIALOG_H
#define ADDMEDICINEDIALOG_H

#include <QDialog>
#include <QFile>
#include <QMessageBox>
#include "dataTypes.h"

namespace Ui {
class AddMedicineDialog;
}

class AddMedicineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMedicineDialog(const QList<PrescriptionItemInfo>& initItems,QWidget *parent = nullptr);
    ~AddMedicineDialog();

    QList<PrescriptionItemInfo> getItems();

private slots:
    void on_btn_add_clicked();

    void on_btn_del_clicked();

    void on_btn_close_clicked();

private:
    Ui::AddMedicineDialog *ui;

    void init_qss();
    void clearInputs();
    void refreshList();

    // 历史药方数据 （医生可以添加/删除） 避免二次进入数据覆盖
    QList<PrescriptionItemInfo> m_items;
};

#endif // ADDMEDICINEDIALOG_H
