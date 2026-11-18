#include "syslogwidget.h"
#include "ui_syslogwidget.h"
#include "winmanager.h"

SysLogWidget::SysLogWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SysLogWidget)
{
    ui->setupUi(this);
    m_netconn = WinManager::getInstance()->getNetConn();
    init_qss();
    initTable();

    ui->edit_date->setCalendarPopup(true);
    ui->edit_date->setDisplayFormat("yyyy-MM-dd");
    ui->edit_date->setDate(QDate::currentDate());
    m_isAllDate = true;
    // 用户手动选日期时，取消“查全部”标志
    connect(ui->edit_date, &QDateEdit::dateChanged, this, [this](const QDate&){
        m_isAllDate = false;
    });

    // 绑定网络信号
    connect(m_netconn, &NetConn::sigDataToSysLogList, this, &SysLogWidget::onLogDataReceived);
}

SysLogWidget::~SysLogWidget()
{
    delete ui;
}

void SysLogWidget::onEnter()
{    
    ui->edit_date->blockSignals(true);
    ui->edit_date->setDate(QDate::currentDate());
    ui->edit_date->calendarWidget()->setCurrentPage(
        QDate::currentDate().year(), QDate::currentDate().month()
        );
    ui->edit_date->blockSignals(false);

    m_isAllDate = true;
    ui->combo_type->setCurrentIndex(0);
    m_currentPage = 1;
    requestLogList();
}


// ================================================================================================
// ====================================== 初始化 ================================================
// ================================================================================================
void SysLogWidget::init_qss()
{

    //1、打开文件
    QFile file(":/qss/sysLog.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void SysLogWidget::initTable()
{
    // 前 3 列给足够的固定宽度
    ui->log_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->log_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->log_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->log_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    ui->log_table->setColumnWidth(0, 130); // 日期
    ui->log_table->setColumnWidth(1, 130); // 操作人
    ui->log_table->setColumnWidth(2, 160); // 操作类型

    ui->log_table->horizontalHeader()->setFixedHeight(40);
    ui->log_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->log_table->verticalHeader()->setVisible(false);
    ui->log_table->verticalHeader()->setDefaultSectionSize(40);
    ui->log_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->log_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->log_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->log_table->setFrameShape(QFrame::NoFrame);
    ui->log_table->setShowGrid(true);
}


// ================================================================================================
// ====================================== 按钮操作 ================================================
// ================================================================================================

void SysLogWidget::onPrevPage()
{
    if (m_currentPage <= 1) {
        QMessageBox::warning(this, "警告", "已经是第一页了！");
        return;
    }
    m_currentPage--;
    requestLogList();
}

void SysLogWidget::onNextPage()
{
    if (m_currentPage >= m_totalPage) {
        QMessageBox::warning(this, "警告", "已经是最后一页了！");
        return;
    }
    m_currentPage++;
    requestLogList();
}

void SysLogWidget::on_btn_remove_clicked()
{
    ui->edit_date->blockSignals(true);
    ui->edit_date->setDate(QDate::currentDate());
    ui->edit_date->blockSignals(false);

    m_isAllDate = true;
    on_btn_search_clicked();
}

void SysLogWidget::on_btn_search_clicked()
{
    m_currentPage = 1;
    requestLogList();
}

void SysLogWidget::on_btn_prev_clicked()
{
    onPrevPage();
}


void SysLogWidget::on_btn_next_clicked()
{
    onNextPage();
}


// ================================================================================================
// ====================================== 网络数据响应 ===========================================
// ================================================================================================
void SysLogWidget::onLogDataReceived(const QByteArray &body)
{
    if (body.size() < (int)sizeof(GET_LOG_LIST_RESP_HEAD)) return;

    const auto* head = reinterpret_cast<const GET_LOG_LIST_RESP_HEAD*>(body.constData());
    if (head->code != 0) {
        QMessageBox::warning(this, "错误", "获取日志列表失败");
        return;
    }
    // 更新总页数
    m_totalPage = qMax(1, (head->total + m_pageSize - 1) / m_pageSize);
    ui->lab_pageinfo->setText(QString("%1 / %2").arg(m_currentPage).arg(m_totalPage));

    // 清空表格
    ui->log_table->clearSpans(); // 清除单元格合并
    ui->log_table->clearContents();
    ui->log_table->setRowCount(head->count);

    // 未搜索到相关数据
    if (head->count == 0) {
        ui->log_table->setRowCount(1);
        auto* emptyItem = new QTableWidgetItem("未搜索到相关数据");
        emptyItem->setTextAlignment(Qt::AlignCenter);
        emptyItem->setForeground(QColor("#909399"));
        ui->log_table->setItem(0, 0, emptyItem);
        ui->log_table->setSpan(0, 0, 1, 4);
        return;
    }

    // 解析数据
    const char* p = body.constData() + sizeof(GET_LOG_LIST_RESP_HEAD);
    for (int i = 0; i < head->count; ++i) {
        const auto* item = reinterpret_cast<const GET_LOG_ITEM*>(p);
        p += sizeof(GET_LOG_ITEM);

        auto makeItem = [](const QString& text, bool isCenter = true) {
            auto* it = new QTableWidgetItem(text);
            it->setTextAlignment(isCenter ? Qt::AlignCenter : (Qt::AlignLeft | Qt::AlignVCenter));
            return it;
        };
        ui->log_table->setItem(i, 0, makeItem(QString::fromUtf8(item->operate_date,
                                                                qstrnlen(item->operate_date, sizeof(item->operate_date)))));
        ui->log_table->setItem(i, 1, makeItem(QString::fromUtf8(item->operator_name,
                                                                qstrnlen(item->operator_name, sizeof(item->operator_name)))));
        ui->log_table->setItem(i, 2, makeItem(QString::fromUtf8(item->operate_type,
                                                                qstrnlen(item->operate_type, sizeof(item->operate_type)))));
        ui->log_table->setItem(i, 3, makeItem(QString::fromUtf8(item->detail,
                                                                qstrnlen(item->detail, sizeof(item->detail))), false));
    }
}


// ================================================================================================
// ====================================== 网络数据请求 ===========================================
// ================================================================================================
void SysLogWidget::requestLogList()
{
    GET_LOG_LIST_REQ req = {};

    // 输入框里有内容，才带上日期
    if (!m_isAllDate) {
        QString dateStr = ui->edit_date->date().toString("yyyy-MM-dd");
        strncpy(req.search_date, dateStr.toUtf8().constData(), sizeof(req.search_date) - 1);
    }

    // 类型处理
    QString typeStr = ui->combo_type->currentText().trimmed();
    if (typeStr != "全部类型") {
        strncpy(req.search_type, typeStr.toUtf8().constData(), sizeof(req.search_type) - 1);
    }

    req.page = m_currentPage;
    req.page_size = m_pageSize;

    HEAD head = { SERVICE_TYPE::GET_LOG_LIST, sizeof(req) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));

    m_netconn->sendPacket(packet);
}


