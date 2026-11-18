#include "addmedicinedialog.h"
#include "ui_addmedicinedialog.h"

AddMedicineDialog::AddMedicineDialog(const QList<PrescriptionItemInfo>& initItems,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddMedicineDialog)
    , m_items(initItems)
{
    ui->setupUi(this);
    this->setWindowTitle("添加药方");
    init_qss();

    // 输入框提示
    ui->edit_medical->setPlaceholderText("如：麻黄");
    ui->edit_dosage->setPlaceholderText("如：6g");
    ui->edit_use->setPlaceholderText("如：水煎服，一日两次");

    // 默认聚焦药名
    ui->edit_medical->setFocus();
    // 刷新列表
    refreshList();

}

AddMedicineDialog::~AddMedicineDialog()
{
    delete ui;
}

QList<PrescriptionItemInfo> AddMedicineDialog::getItems()
{
    return m_items;
}

void AddMedicineDialog::on_btn_add_clicked()
{
    QString name   = ui->edit_medical->text().trimmed();
    QString dosage = ui->edit_dosage->text().trimmed();
    QString usage  = ui->edit_use->text().trimmed();

    // ---- 校验 ----
    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入药品名称");
        ui->edit_medical->setFocus();
        return;
    }

    if (dosage.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入剂量");
        ui->edit_dosage->setFocus();
        return;
    }
    for (const auto& item : m_items) {
        if (item.medicineName == name && item.dosage == dosage) {
            QMessageBox::information(this, "提示",
                                     QString("药品【%1 %2】已存在").arg(name, dosage));
            return;
        }
    }

    // 添加到列表
    PrescriptionItemInfo info;
    info.medicineName = name;
    info.dosage       = dosage;
    info.usageMethod  = usage;
    m_items.append(info);

    // 重置加刷新
    clearInputs();
    refreshList();
    ui->edit_medical->setFocus();
}

void AddMedicineDialog::on_btn_del_clicked()
{
    int row = ui->list_detail->currentRow();
    if (row < 0 || row >= m_items.size()) {
        QMessageBox::information(this, "提示", "请先选中要删除的药品");
        return;
    }
    // 确认
    const auto& item = m_items[row];
    auto ret = QMessageBox::question(this, "确认",
                                     QString("确定删除【%1 %2】吗？")
                                         .arg(item.medicineName, item.dosage),
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    //删除
    m_items.removeAt(row);
    refreshList();
    //选中下一项
    if (row < m_items.size()) {
        ui->list_detail->setCurrentRow(row);
    } else if (m_items.size() > 0) {
        ui->list_detail->setCurrentRow(m_items.size() - 1);
    }
}

void AddMedicineDialog::on_btn_close_clicked()
{
    accept();
}

void AddMedicineDialog::init_qss()
{
    QFile file(":/qss/addMedicine.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void AddMedicineDialog::clearInputs()
{
    ui->edit_medical->clear();
    ui->edit_dosage->clear();
    ui->edit_use->clear();
}

void AddMedicineDialog::refreshList()
{
    ui->list_detail->clear();
    for (int i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        QString text = QString("%1. %2  %3  %4")
                           .arg(i + 1)
                           .arg(item.medicineName)
                           .arg(item.dosage)
                           .arg(item.usageMethod);
        ui->list_detail->addItem(text);
    }
}



