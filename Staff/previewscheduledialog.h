#ifndef PREVIEWSCHEDULEDIALOG_H
#define PREVIEWSCHEDULEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include "../dataTypes.h"

namespace Ui {
class PreviewScheduleDialog;
}

class PreviewScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreviewScheduleDialog(const QList<ScheduleEntry>& entries,
                                   const QList<DoctorInfo>& doctors,
                                   const QStringList& warnings,
                                   QWidget *parent = nullptr);
    ~PreviewScheduleDialog();

    QList<ScheduleEntry> getFinalEntries() const;
private:
    void initUi(const QStringList& warnings);
    void refreshTable();

    QList<ScheduleEntry> m_entries;      // 待上传的排班数据
    QList<DoctorInfo> m_doctors;         // 用于通过ID查医生名字
    QTableWidget* m_table = nullptr;
};

#endif // PREVIEWSCHEDULEDIALOG_H
