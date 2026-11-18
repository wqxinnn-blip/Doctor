#include "previewscheduledialog.h"

PreviewScheduleDialog::PreviewScheduleDialog(const QList<ScheduleEntry>& entries,
                                             const QList<DoctorInfo>& doctors,
                                             const QStringList& warnings,
                                             QWidget *parent)
    : QDialog(parent)
    , m_entries(entries)
    , m_doctors(doctors)
{
    setWindowTitle("自动排班预览");
    resize(600, 500);

    setStyleSheet("QDialog { background-color: #ffffff; } QLabel { color: #303133; } "
                  "QTableWidget { border: 1px solid #EBEEF5; }");

    initUi(warnings);
    refreshTable();
}

PreviewScheduleDialog::~PreviewScheduleDialog()
{
}

void PreviewScheduleDialog::initUi(const QStringList& warnings)
{
    auto* mainLayout = new QVBoxLayout(this);

    // 1. 顶部提示文字
    auto* topLayout = new QVBoxLayout();
    auto* titleLab = new QLabel(QString("已为您自动生成 %1 条排班记录。").arg(m_entries.size()), this);
    titleLab->setStyleSheet("font-size: 15px; font-weight: bold; color: #409EFF;");
    topLayout->addWidget(titleLab);

    if (!warnings.isEmpty()) {
        auto* warnLab = new QLabel(this);
        warnLab->setText("⚠️ 冲突警告：" + warnings.join("；"));
        warnLab->setStyleSheet("color: #F56C6C; font-size: 12px;");
        warnLab->setWordWrap(true);
        topLayout->addWidget(warnLab);
    }

    auto* tipLab = new QLabel("您可以在下方列表中删除不需要的排班。", this);
    tipLab->setStyleSheet("color: #909399; font-size: 12px; margin-top: 5px;");
    topLayout->addWidget(tipLab);

    mainLayout->addLayout(topLayout);

    // 2. 表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"日期", "星期", "班次", "医生", "操作"});

    // 列宽设置
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table);

    // 3. 底部按钮
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* btnCancel = new QPushButton("取消上传", this);
    auto* btnConfirm = new QPushButton("确认上传", this);

    btnCancel->setStyleSheet("padding: 6px 20px; border-radius: 4px; border: none; background: #409EFF; color: #FFF; font-weight: bold;");
    btnConfirm->setStyleSheet("padding: 6px 20px; border-radius: 4px; border: none; background: #409EFF; color: #FFF; font-weight: bold;");

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnConfirm, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnConfirm);
    mainLayout->addLayout(btnLayout);
}

void PreviewScheduleDialog::refreshTable()
{
    m_table->setRowCount(m_entries.size());

    // 按日期、班次排序
    std::sort(m_entries.begin(), m_entries.end(), [](const ScheduleEntry& a, const ScheduleEntry& b) {
        if (a.workdate != b.workdate) return a.workdate < b.workdate;
        return a.shift < b.shift;
    });

    for (int i = 0; i < m_entries.size(); ++i) {
        const auto& entry = m_entries[i];

        // 1. 日期
        auto* dateItem = new QTableWidgetItem(entry.workdate.toString("yyyy-MM-dd"));
        dateItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, dateItem);

        // 2. 星期
        QStringList weekDays = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
        auto* weekItem = new QTableWidgetItem(weekDays[entry.workdate.dayOfWeek() - 1]);
        weekItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, weekItem);

        // 3. 班次
        QString shiftStr;
        if (entry.shift == ShiftType::Morning) shiftStr = "早班";
        else if (entry.shift == ShiftType::Afternoon) shiftStr = "下午班";
        else shiftStr = "夜班";
        auto* shiftItem = new QTableWidgetItem(shiftStr);
        shiftItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, shiftItem);

        // 4. 医生姓名
        QString docName = "未知医生";
        for (const auto& d : m_doctors) {
            if (d.staffId == entry.doctorId) {
                docName = d.realName;
                break;
            }
        }
        auto* docItem = new QTableWidgetItem(docName);
        docItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 3, docItem);

        // 5. 删除按钮
        auto* btnDel = new QPushButton("删除", this);
        btnDel->setStyleSheet("color: #F56C6C; border: none; background: transparent;");
        btnDel->setCursor(Qt::PointingHandCursor);

        connect(btnDel, &QPushButton::clicked, this, [this, entry]() {
            // 从列表中移除这一项
            for (int j = 0; j < m_entries.size(); ++j) {
                if (m_entries[j].workdate == entry.workdate &&
                    m_entries[j].shift == entry.shift &&
                    m_entries[j].doctorId == entry.doctorId) {
                    m_entries.removeAt(j);
                    break;
                }
            }
            refreshTable(); // 重新渲染
        });

        // 把按钮居中放在格子里
        auto* widget = new QWidget();
        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(btnDel);
        m_table->setCellWidget(i, 4, widget);
    }
}

QList<ScheduleEntry> PreviewScheduleDialog::getFinalEntries() const
{
    return m_entries;
}
