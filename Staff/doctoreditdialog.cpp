#include "doctoreditdialog.h"
#include "ui_doctoreditdialog.h"

DoctorEditDialog::DoctorEditDialog(const QList<DepartmentInfo>& deptList, QWidget *parent)
    : QDialog(parent)
    ,ui(new Ui::DoctorEditDialog)
    ,m_deptList(deptList)
{
    ui->setupUi(this);
    init_qss();

    ui->combo_dept->clear();
    ui->combo_dept->addItem("请选择", -1);
        for (const auto& dept : m_deptList) {
        ui->combo_dept->addItem(dept.deptName,dept.dept_id);
    }

}

DoctorEditDialog::~DoctorEditDialog()
{
    delete ui;
}

void DoctorEditDialog::setupForAdd(const QString &newWorkNo)
{
    ui->lab_workNo1->setText(QString(newWorkNo));
    // 清空输入框
    ui->edit_name->clear();
    ui->edit_age->clear();

    ui->combo_dept->setCurrentIndex(0);
    ui->combo_title->setCurrentIndex(0);
}

void DoctorEditDialog::setupForEdit(const LocalDoctorData &data)
{
    ui->lab_workNo1->setText(data.workNo);
    ui->edit_name->setText(data.name);
    ui->edit_age->setText(QString::number(data.age));

    if (!data.dept.isEmpty()) {
        int deptIndex = ui->combo_dept->findText(data.dept);
        if (deptIndex != -1) {
            ui->combo_dept->setCurrentIndex(deptIndex);
        } else {
            ui->combo_dept->addItem(data.dept);
            ui->combo_dept->setCurrentIndex(ui->combo_dept->count() - 1);
        }
    }
    if (!data.title.isEmpty()) {
        int titleIndex = ui->combo_title->findText(data.title);
        if (titleIndex != -1) {
            ui->combo_title->setCurrentIndex(titleIndex);
        }
    }
}

LocalDoctorData DoctorEditDialog::getDoctorData() const
{
    LocalDoctorData data;
    data.workNo = ui->lab_workNo1->text().trimmed();
    data.name = ui->edit_name->text().trimmed();
    data.dept = ui->combo_dept->currentText();
    data.title = ui->combo_title->currentText();
    data.age = ui->edit_age->text().toInt();
    return data;
}


// 点击确认
void DoctorEditDialog::on_btn_enter_clicked()
{
    if (ui->edit_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "姓名不能为空！");
        return;
    }
    if (ui->edit_age->text().toInt() <= 0 || ui->edit_age->text().toInt() > 100) {
        QMessageBox::warning(this, "提示", "请输入正确的年龄(1-100)！");
        return;
    }
    if (ui->combo_dept->currentIndex() == 0) {
        QMessageBox::warning(this, "提示", "请选择科室！");
        return;
    }
    if (ui->combo_title->currentIndex() == 0) {
        QMessageBox::warning(this, "提示", "请选择医师职称！");
        return;
    }
    this->accept();
}

// 点击取消
void DoctorEditDialog::on_btn_cancel_clicked()
{
    this->reject();
}

void DoctorEditDialog::init_qss()
{
    //1、打开文件
    QFile file(":/qss/DocEdit.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}


