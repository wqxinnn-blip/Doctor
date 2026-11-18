#include "monthscheduleview.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include "winmanager.h"

MonthScheduleView::MonthScheduleView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_netconn = WinManager::getInstance()->getNetConn();

    m_table = new QTableWidget(this);
    initTable();
    mainLayout->addWidget(m_table);

    connect(m_netconn, &NetConn::sigDataToMonthSchedule,
            this, &MonthScheduleView::onScheduleDataReceived);
}


// ================================================================================================
// ====================================== 初始化 ================================================
// ================================================================================================

void MonthScheduleView::initTable()
{
    m_table->setRowCount(6);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(m_weekHeaders);
    m_table->verticalHeader()->setVisible(false);

    // 表头拉伸
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setMinimumSectionSize(72);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 去掉表格边框
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setGridStyle(Qt::SolidLine);

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
            m_table->setItem(row, col, item);
        }
    }

    connect(m_table, &QTableWidget::cellDoubleClicked,this, [this](int row, int col){
        int idx = row * 7 + col;
        if (idx >= 0 && idx < m_dates.size()) {
            emit editRequested(m_dates[idx]);
        }
    });
}

void MonthScheduleView::setScheduleData(const QString &dept, const QDate &date)
{
    Q_UNUSED(dept);
    m_currentMonth = QDate(date.year(), date.month(), 1);

    // 1. 画日历
    fillMonthCalendar(m_currentMonth);

    // 2. 查本月排班
    QDate firstDay = m_currentMonth;
    QDate lastDay  = m_currentMonth.addMonths(1).addDays(-1);
    requestMonthSchedule(firstDay, lastDay);
}


// ================================================================================================
// ====================================== 工具函数 ================================================
// ================================================================================================

void MonthScheduleView::updateCell(int row, int col, const QDate& date)
{
    QTableWidgetItem* item = m_table->item(row, col);
    if (!item) return;

    // 保留原日期数字（第一行）
    QString text = QString::number(date.day());
    item->setForeground(QColor("#333333"));
    item->setBackground(Qt::white);
    if (col >= 5) {
        item->setBackground(QColor("#FFF5F5"));
    }
    if (date == QDate::currentDate()) {
        item->setBackground(QColor("#E6F2FF"));
    }

    // 收集当天所有排班
    for (const ScheduleEntry& s : std::as_const(m_allSchedules)) {
        if (s.workdate != date) continue;

        QString icon;
        switch (s.shift) {
        case ShiftType::Morning:   icon = "☀";  break;
        case ShiftType::Afternoon: icon = "🌤"; break;
        case ShiftType::Night:     icon = "🌙"; break;
        }

        text += QString("\n%1 %2").arg(icon, s.doc_name);
    }

    item->setText(text);
    item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
}

void MonthScheduleView::refreshData()
{
    if (!m_currentMonth.isValid()) return;
    QDate firstDay = m_currentMonth;
    QDate lastDay  = m_currentMonth.addMonths(1).addDays(-1);
    requestMonthSchedule(firstDay, lastDay);
}

void MonthScheduleView::setDepartment(int deptId)
{
    m_departmentId = deptId;
}

void MonthScheduleView::fillMonthCalendar(const QDate &baseDate)
{
    m_dates.clear();

    int year  = baseDate.year();
    int month = baseDate.month();

    QDate firstDay(year, month, 1);
    int firstWeekDay = firstDay.dayOfWeek();   // 1=周一 ... 7=周日
    int daysInMonth  = firstDay.daysInMonth();

    QDate lastMonth = firstDay.addMonths(-1);
    int daysInLastMonth = lastMonth.daysInMonth();

    QDate today = QDate::currentDate();

    int currentDay = 1;

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);

            QDate cellDate;

            // 上个月
            if (row == 0 && col < firstWeekDay - 1) {
                int lastDay = daysInLastMonth - (firstWeekDay - 2 - col);
                item->setText(QString::number(lastDay));
                item->setForeground(QColor("#C0C4CC"));
                item->setBackground(QColor("#F5F6F8"));
                cellDate = QDate(lastMonth.year(), lastMonth.month(), lastDay);
            }
            // 当月
            else if (currentDay <= daysInMonth) {
                item->setText(QString::number(currentDay));
                item->setForeground(QColor("#333333"));
                cellDate = QDate(year, month, currentDay);

                if (col >= 5) {
                    item->setBackground(QColor("#FFF5F5"));
                }
                if (cellDate == today) {
                    item->setBackground(QColor("#E6F2FF"));
                }
                currentDay++;
            }
            // 下个月
            else {
                int nextDay = currentDay - daysInMonth;
                item->setText(QString::number(nextDay));
                item->setForeground(QColor("#C0C4CC"));
                item->setBackground(QColor("#F5F6F8"));

                QDate nextMonth = firstDay.addMonths(1);
                cellDate = QDate(nextMonth.year(), nextMonth.month(), nextDay);

                currentDay++;
            }

            m_dates.append(cellDate);
            m_table->setItem(row, col, item);
        }
    }
}

// ================================================================================================
// ====================================== 网络包请求 ================================================
// ================================================================================================

void MonthScheduleView::requestMonthSchedule(const QDate& firstDay, const QDate& lastDay)
{
    MONTH_SCHEDULE_REQ req = {};
    strncpy(req.first_day, firstDay.toString("yyyy-MM-dd").toUtf8().constData(), 10);
    strncpy(req.last_day,  lastDay.toString("yyyy-MM-dd").toUtf8().constData(), 10);
    req.deptId = m_departmentId;

    HEAD head = { SERVICE_TYPE::MONTH_SCHEDULE, sizeof(MONTH_SCHEDULE_REQ) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(MONTH_SCHEDULE_REQ));
    m_netconn->sendPacket(packet);
}


// ================================================================================================
// ====================================== 网络包响应 ================================================
// ================================================================================================

void MonthScheduleView::onScheduleDataReceived(const QByteArray& body)
{
    if (body.size() % sizeof(MONTH_SCHEDULE_RESP) != 0) {
        qWarning() << "排班数据包长度异常:" << body.size();
        return;
    }

    int count = body.size() / sizeof(MONTH_SCHEDULE_RESP);
    QList<ScheduleEntry> list;

    const char* p = body.constData();
    for (int i = 0; i < count; ++i) {
        const auto* item = reinterpret_cast<const MONTH_SCHEDULE_RESP*>(p);

        ScheduleEntry e;
        e.scheduleId = item->schedule_id;
        e.doctorId   = item->doc_id;
        e.doc_name   = QString::fromUtf8(item->doc_name);
        e.doc_title  = QString::fromUtf8(item->doc_title);
        e.workdate   = QDate::fromString(QString::fromUtf8(item->workdate), "yyyy-MM-dd");
        e.shift      = static_cast<ShiftType>(item->shift);

        list.append(e);
        p += sizeof(MONTH_SCHEDULE_RESP);
    }

    m_allSchedules = list;

    // 按日期分组，填到对应格子
    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            int idx = r * 7 + c;
            if (idx >= m_dates.size()) continue;
            updateCell(r, c, m_dates[idx]);
        }
    }
}