#include "appointmentlistpage.h"
#include "tonguereportdialog.h"
#include "../protocol.h"
#include "../staff_info.h"
#include "winmanager.h"

AppointmentListPage::AppointmentListPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    m_netconn = WinManager::getInstance()->getNetConn();

    //接收本班次预约信息
    connect(m_netconn, &NetConn::sigDataToAppointmentList,
            this, &AppointmentListPage::onAppointmentListReceived);
    // 失约
    connect(m_netconn, &NetConn::sigDataToAppointmentMiss,
            this, &AppointmentListPage::onAppointmentMissResp);
    // 刷新定时器
    m_shiftTimer = new QTimer(this);
    connect(m_shiftTimer, &QTimer::timeout, this,&AppointmentListPage::onRefreshTimerTimeout);
}

AppointmentListPage::~AppointmentListPage()
{
}

void AppointmentListPage::onEnter()
{
    // 重置页码
    m_currentPage = 1;
    // 判断当前班次
    m_currentShift = getShiftByTime(QTime::currentTime());
    // 刷新列表
    requestTodayAppointments();
    // 启动定时器
    m_shiftTimer->stop();
    m_shiftTimer->start(30 * 1000);
}

void AppointmentListPage::onLeave()
{
    // 关闭定时器
    m_shiftTimer->stop();
    // 清空表格
    m_table->clearContents();
    m_table->setRowCount(0);
    // 清理患者预约信息
    m_appointments.clear();
    m_requesting = false;
}

void AppointmentListPage::onPrevPage()
{
    if (m_currentPage <= 1){
        QMessageBox::information(this,"提示","当前已是第一页");
        return;
    }
    if(m_requesting) return;
    m_currentPage--;
    requestTodayAppointments();
}

void AppointmentListPage::onNextPage()
{
    if (m_currentPage >= m_totalPage){
        QMessageBox::information(this,"提示","当前已是最后一页");
        return;
    }
    if(m_requesting) return;
    m_currentPage++;
    requestTodayAppointments();
}

void AppointmentListPage::onRefreshTimerTimeout()
{
    int cur = getShiftByTime(QTime::currentTime());
    if (cur != m_currentShift) {
        m_currentShift = cur;
        m_currentPage  = 1;
    }
    qDebug()<<"执行定时数据刷新";
    requestTodayAppointments();
}



void AppointmentListPage::onAppointmentListReceived(const QByteArray &body)
{
    if (!this->isVisible()) return;
    // 收到响应数据包  解锁
    m_requesting = false;
    if (m_currentShift == -1) {
        // 当前为非工作时段，丢弃该响应数据包;
        return;
    }

    if (body.size() < (int)sizeof(APPOINTMENT_LIST_RESP_HEAD)) {
        qWarning() << "body 太小";
        return;
    }

    const auto* head = reinterpret_cast<const APPOINTMENT_LIST_RESP_HEAD*>(body.constData());
    if (head->code != 0) {
        qWarning() << "查预约失败:" << head->code;
        return;
    }

    if (head->count < 0 || head->count > 1000) {
        qWarning() << "非法 count:" << head->count;
        return;
    }

    int expectedSize = sizeof(APPOINTMENT_LIST_RESP_HEAD)
                       + head->count * sizeof(APPOINTMENT_LIST_RESP);
    if (body.size() < expectedSize) {
        qWarning() << "body 长度不足:" << body.size() << "/" << expectedSize;
        return;
    }

    // 解析
    QList<AppointmentInfo> list;
    const char* p = body.constData() + sizeof(APPOINTMENT_LIST_RESP_HEAD);
    for (int i = 0; i < head->count; ++i) {
        const auto* item = reinterpret_cast<const APPOINTMENT_LIST_RESP*>(p);

        AppointmentInfo a;
        a.appointmentId = item->appointment_id;
        a.patientId     = item->patient_id;
        a.patientName   = QString::fromUtf8(item->patient_name).trimmed();
        a.workDate      = QString::fromUtf8(item->work_date).trimmed();
        a.shiftType     = item->shift_type;
        a.tongueRecordId = item->tongue_record_id;
        a.status        = item->status;

        list.append(a);
        p += sizeof(APPOINTMENT_LIST_RESP);
    }

    // 3. 比较新旧数据
    bool changed = (list.size() != m_appointments.size());
    if (!changed) {
        for (int i = 0; i < list.size(); ++i) {
            const auto& a = list[i];
            const auto& b = m_appointments[i];
            if (a.appointmentId  != b.appointmentId  ||
                a.status         != b.status         ||
                a.tongueRecordId != b.tongueRecordId ||
                a.patientName    != b.patientName    ||
                a.shiftType      != b.shiftType) {
                changed = true;
                break;
            }
        }
    }

    m_appointments = list;
    m_totalPage = qMax(1, (head->total + m_pageSize - 1) / m_pageSize);

    if (changed) {
        fillTable(list);
    } else {
        qDebug() << "[AppointmentListPage] 数据无变化，跳过表格重建";
    }
    updatePagination();
}

void AppointmentListPage::onAppointmentMissResp(const QByteArray &body)
{
    if (body.size() != sizeof(APPOINTMENT_MISS_RESP)) return;

    const auto* resp = reinterpret_cast<const APPOINTMENT_MISS_RESP*>(body.constData());

    if (resp->code == 0) {
        QMessageBox::information(this, "提示", "已标记失约");
        m_requesting = false;
        requestTodayAppointments();
    } else {
        QMessageBox::warning(this, "失败",QString::fromUtf8(resp->message,qstrnlen(resp->message, sizeof(resp->message))));
    }
}


void AppointmentListPage::updatePagination()
{
    m_labPageInfo->setText(QString("%1 / %2").arg(m_currentPage).arg(m_totalPage));
    m_btnPrev->setEnabled(m_currentPage > 1);
    m_btnNext->setEnabled(m_currentPage < m_totalPage);
}



// 请求本医生 今日患者预约记录
void AppointmentListPage::requestTodayAppointments()
{
    // 避免定时器、翻页、失约刷新同时触发
    if (m_requesting) {
        qDebug() << "[AppointmentListPage] 上一次请求未完成，跳过本次";
        return;
    }
    m_requesting = true;

    // 非工作时段
    if (m_currentShift == -1) {
        // 非工作时段
        m_table->setRowCount(0);
        m_currentPage = 1;
        m_totalPage = 1;
        updatePagination();

        m_requesting = false;
        return;
    }

    APPOINTMENT_LIST_REQ req = {};
    req.doctor_id = WinManager::getInstance()->getStaff_Info().getStaff_id();
    req.page = m_currentPage;
    req.page_size = m_pageSize;
    req.shift_type = m_currentShift;
    // 日期：今天
    QByteArray dateBytes = QDate::currentDate().toString("yyyy-MM-dd").toUtf8();
    strncpy(req.work_date, dateBytes.constData(), sizeof(req.work_date) - 1);
    req.work_date[sizeof(req.work_date) - 1] = '\0';

    HEAD head = { SERVICE_TYPE::APPOINTMENT_LIST, sizeof(req) };

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));
    m_netconn->sendPacket(packet);
    qDebug() << "[AppointmentListPage] 请求当日预约, doctor_id =" << req.doctor_id
             << ", shift =" << req.shift_type
             << ", page =" << req.page;
}

int AppointmentListPage::getShiftByTime(const QTime &now)
{
    if (now >= QTime(8, 0)  && now < QTime(12, 0)) return 1;
    if (now >= QTime(14, 0) && now < QTime(17, 0)) return 2;
    if (now >= QTime(19, 0) && now < QTime(21, 0)) return 3;
    return -1;
}


// 填充表格
void AppointmentListPage::fillTable(const QList<AppointmentInfo> &list)
{
    m_table->clearContents();
    m_table->setRowCount(list.size());

    for (int row = 0; row < list.size(); ++row) {
        const auto& a = list[row];

        auto makeItem = [](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        m_table->setItem(row, 0, makeItem(a.workDate));
        m_table->setItem(row, 1, makeItem(a.patientName));

        QString shiftStr;
        switch (a.shiftType) {
        case 1: shiftStr = "上午 08:00-12:00"; break;
        case 2: shiftStr = "下午 14:00-17:00"; break;
        case 3: shiftStr = "夜班 19:00-21:00"; break;
        }
        m_table->setItem(row, 2, makeItem(shiftStr));


        // 检测报告
        auto* btnReport = new QPushButton("查看报告", m_table);
        btnReport->setObjectName("btn_view_report");
        btnReport->setCursor(Qt::PointingHandCursor);


        // "查看报告"按钮点击
        connect(btnReport, &QPushButton::clicked, this, [this, a]() {
            if (a.tongueRecordId <= 0) {
                QMessageBox::information(this, "提示", "该患者暂无舌苔检测报告");
                return;
            }
            qDebug()<<"点击查看报告";
            qDebug()<<"tongueRecordId = "<<a.tongueRecordId;
            m_pendingAppointmentId = a.appointmentId;
            TongueReportDialog dlg(a.tongueRecordId,this);
            dlg.exec();

        });
        m_table->setCellWidget(row, 3, btnReport);


        // 操作
        auto* opWidget = new QWidget();
        auto* opLayout = new QHBoxLayout(opWidget);
        opLayout->setContentsMargins(8,4,8,4);
        opLayout->setSpacing(12); // 两个按钮之间空隙12px

        QPushButton* btnConsult = new QPushButton("问诊", opWidget);
        btnConsult->setObjectName("btn_consult");
        btnConsult->setCursor(Qt::PointingHandCursor);
        QPushButton* btnMiss = new QPushButton("失约", opWidget);
        btnMiss->setObjectName("btn_miss");
        btnMiss->setCursor(Qt::PointingHandCursor);

        opLayout->addWidget(btnConsult);
        opLayout->addWidget(btnMiss);

        // 就诊
        connect(btnConsult, &QPushButton::clicked, this, [this, a]() {
            emit startConsultation(a.appointmentId, a.patientId, a.tongueRecordId);
        });
        // 失约
        connect(btnMiss, &QPushButton::clicked, this, [this, a]() {
            auto ret = QMessageBox::question(this, "确认",
                                             QString("确定将 %1 标记为已失约？").arg(a.patientName),
                                             QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) return;

            APPOINTMENT_MISS_REQ req = {};
            req.appointment_id = a.appointmentId;
            HEAD head = { SERVICE_TYPE::APPOINTMENT_MISS, sizeof(req) };
            QByteArray packet;
            packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
            packet.append(reinterpret_cast<const char*>(&req), sizeof(req));
            m_netconn->sendPacket(packet);
        });
        m_table->setCellWidget(row, 4, opWidget);
    }

}

void AppointmentListPage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(16);

    // ===== 表格 =====
    m_table = new QTableWidget(this);
    setupTable();
    root->addWidget(m_table, 1);

    // ===== 分页 =====
    auto* pageLayout = new QHBoxLayout();
    pageLayout->setSpacing(20);
    pageLayout->addStretch();

    m_btnPrev = new QPushButton("上一页", this);
    m_btnPrev->setObjectName("btn_prev");

    m_labPageInfo = new QLabel("1 / 1", this);
    m_labPageInfo->setObjectName("lab_page_info");
    m_labPageInfo->setAlignment(Qt::AlignCenter);

    m_btnNext = new QPushButton("下一页", this);
    m_btnNext->setObjectName("btn_next");

    pageLayout->addWidget(m_btnPrev);
    pageLayout->addWidget(m_labPageInfo);
    pageLayout->addWidget(m_btnNext);
    pageLayout->addStretch();

    root->addLayout(pageLayout);

    // ===== 信号 =====
    connect(m_btnPrev, &QPushButton::clicked, this, &AppointmentListPage::onPrevPage);
    connect(m_btnNext, &QPushButton::clicked, this, &AppointmentListPage::onNextPage);
}

void AppointmentListPage::setupTable()
{
    QStringList headers = { "日期", "姓名", "预约时间", "检测报告", "操作" };
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);

    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_table->horizontalHeader()->setFixedHeight(45);

    m_table->horizontalHeader()->setMinimumSectionSize(80);

    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->setAlternatingRowColors(false);
    m_table->setRowCount(0);

    // 行高
    m_table->verticalHeader()->setDefaultSectionSize(50);

    // 居中
    m_table->setStyleSheet(
        "QTableWidget::item {"
        "    text-align: center;"
        "    padding: 8px;"
        "}"
        );
}


