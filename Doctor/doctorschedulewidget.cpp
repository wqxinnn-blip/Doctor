#include "doctorschedulewidget.h"
#include "winmanager.h"
#include "protocol.h"

DoctorScheduleWidget::DoctorScheduleWidget(QWidget *parent)
    : QWidget{parent}
{
    m_netconn = WinManager::getInstance()->getNetConn();

    init_qss();
    setupUi();
    initTable();

    // 初始化当前周 周一
    m_currentMonday = QDate::currentDate().addDays(1 - QDate::currentDate().dayOfWeek());

    connect(m_netconn, &NetConn::sigDataToDoctorWeekSchedule, this, &DoctorScheduleWidget::onDataReceived);
}

void DoctorScheduleWidget::onEnter()
{
    refreshData();
}

void DoctorScheduleWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(20);

    // --- 顶部导航栏 ---
    auto* navLayout = new QHBoxLayout();

    auto* btnPrev = new QPushButton("上一周", this);
    btnPrev->setObjectName("navBtn");
    btnPrev->setFixedSize(100, 34);

    m_labDateRange = new QLabel(this);
    m_labDateRange->setAlignment(Qt::AlignCenter);
    m_labDateRange->setObjectName("dateLabel");

    auto* btnNext = new QPushButton("下一周", this);
    btnNext->setObjectName("navBtn");
    btnNext->setFixedSize(100, 34);

    navLayout->addStretch();
    navLayout->addWidget(btnPrev);
    navLayout->addWidget(m_labDateRange);
    navLayout->addWidget(btnNext);
    navLayout->addStretch();

    root->addLayout(navLayout);
    root->addWidget(m_table = new QTableWidget(this));

    connect(btnPrev, &QPushButton::clicked, this, &DoctorScheduleWidget::onPrevWeek);
    connect(btnNext, &QPushButton::clicked, this, &DoctorScheduleWidget::onNextWeek);
}

void DoctorScheduleWidget::init_qss()
{
    //1、打开文件
    QFile file(":/qss/doctorSchedule.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void DoctorScheduleWidget::initTable()
{
    // 行：上午、下午、夜班（3行）
    // 列：班次+周一到周日（8列）
    m_table->setRowCount(3);
    m_table->setColumnCount(8);

    QStringList headers = {"班次", "周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    m_table->setHorizontalHeaderLabels(headers);

    // 表头样式
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setFixedHeight(45);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    // 表格属性
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(60);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // 只读
    m_table->setSelectionMode(QAbstractItemView::NoSelection);   // 不能选中
    m_table->setFrameShape(QFrame::NoFrame);

    // 初始化第一列（班次名称）
    QStringList shiftNames = {"上午班", "下午班", "夜班"};
    for (int i = 0; i < 3; ++i) {
        auto* item = new QTableWidgetItem(shiftNames[i]);
        item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, item);
    }
}

void DoctorScheduleWidget::updateDateLabel()
{
    QDate sunday = m_currentMonday.addDays(6);
    QString text = QString("%1年%2月%3日 - %4年%5月%6日")
                       .arg(m_currentMonday.year()).arg(m_currentMonday.month()).arg(m_currentMonday.day())
                       .arg(sunday.year()).arg(sunday.month()).arg(sunday.day());
    m_labDateRange->setText(text);
}

void DoctorScheduleWidget::onPrevWeek()
{
    m_currentMonday = m_currentMonday.addDays(-7);
    refreshData();
}

void DoctorScheduleWidget::onNextWeek()
{
    m_currentMonday = m_currentMonday.addDays(7);
    refreshData();
}

void DoctorScheduleWidget::refreshData()
{
    updateDateLabel();

    // 先把 1~7 列全部置空（清空旧数据）
    for (int r = 0; r < 3; ++r) {
        for (int c = 1; c < 8; ++c) {
            if (!m_table->item(r, c)) {
                m_table->setItem(r, c, new QTableWidgetItem(""));
            }
            m_table->item(r, c)->setText("");
            m_table->item(r, c)->setBackground(QColor("#f5f7fa")); // 没排班显示浅灰
        }
    }

    // 发送请求
    DOCTOR_WEEK_SCHEDULE_REQ req = {};
    req.doctor_id = WinManager::getInstance()->getStaff_Info().getStaff_id();

    QByteArray startStr = m_currentMonday.toString("yyyy-MM-dd").toUtf8();
    QByteArray endStr = m_currentMonday.addDays(6).toString("yyyy-MM-dd").toUtf8();

    strncpy(req.start_date, startStr.constData(), sizeof(req.start_date) - 1);
    strncpy(req.end_date, endStr.constData(), sizeof(req.end_date) - 1);

    HEAD head = { SERVICE_TYPE::DOCTOR_WEEK_SCHEDULE, sizeof(req) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));
    m_netconn->sendPacket(packet);
    //qDebug()<<"发送个人排班请求";
}

void DoctorScheduleWidget::onDataReceived(const QByteArray &body)
{
    if (body.size() < (int)sizeof(DOCTOR_WEEK_SCHEDULE_RESP_HEAD)) return;
    const auto* head = reinterpret_cast<const DOCTOR_WEEK_SCHEDULE_RESP_HEAD*>(body.constData());
    if (head->code != 0) return;

    const char* p = body.constData() + sizeof(DOCTOR_WEEK_SCHEDULE_RESP_HEAD);

    for (int i = 0; i < head->count; ++i) {
        const auto* item = reinterpret_cast<const DOCTOR_WEEK_SCHEDULE_ITEM*>(p);
        p += sizeof(DOCTOR_WEEK_SCHEDULE_ITEM);

        QString dateStr = QString::fromUtf8(item->work_date, 10);
        QDate workDate = QDate::fromString(dateStr, "yyyy-MM-dd");

        // 计算列：0是周一，1是周二...6是周日，加1对应表格的1~7列
        int col = m_currentMonday.daysTo(workDate) + 1;
        if (col < 1 || col > 7) continue; // 越界跳过

        // 计算行：1=上午(行0)，2=下午(行1)，3=夜班(行2)
        int row = item->shift_type - 1;
        if (row < 0 || row > 2) continue;

        // 填时间
        QString timeText;
        if (item->shift_type == 1) timeText = "上午 8:00-12:00";
        else if (item->shift_type == 2) timeText = "下午 14:00-17:00";
        else if (item->shift_type == 3) timeText = "晚上 19:00-21:00";

        auto* cellItem = m_table->item(row, col);
        cellItem->setText(timeText);
        cellItem->setTextAlignment(Qt::AlignCenter);
        cellItem->setBackground(QColor("#ffffff")); // 有排班显示纯白
    }
}
