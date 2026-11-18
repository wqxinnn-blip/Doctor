#include "deptmanagerwidget.h"
#include "ui_deptmanagerwidget.h"
#include "adddepartmentdialog.h"
#include "winmanager.h"

DeptManagerWidget::DeptManagerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DeptManagerWidget)
{
    ui->setupUi(this);
    init_qss();
    init_list();
    m_netconn = WinManager::getInstance()->getNetConn();

    connect(m_netconn, &NetConn::sigDataToDeptArchive, this, &DeptManagerWidget::onDeptArchiveDataReceived);
}

DeptManagerWidget::~DeptManagerWidget()
{
    delete ui;
}

void DeptManagerWidget::onEnter()
{
    m_currentPage = 1;
    refreshDeptData();
}

// ===============================================================================================
// ====================================== 初始化 =========================================
// ===============================================================================================

void DeptManagerWidget::init_list()
{
    // 1. 切换为网格/图标模式
    ui->listWidget->setViewMode(QListView::IconMode);
    // 2. 允许自动换行（窗口拉宽时，一行能显示更多卡片）
    ui->listWidget->setResizeMode(QListView::Adjust);
    // 3. 禁止拖拽
    ui->listWidget->setMovement(QListView::Static);
    // 4. 设置每个网格（卡片）的统一大小
    ui->listWidget->setGridSize(QSize(320, 120));
    // 5. 设置卡片之间的间距
    ui->listWidget->setSpacing(30);
    // 6. 去掉选中时的虚线框
    ui->listWidget->setStyleSheet("QListWidget::item { outline: none; }");
}

void DeptManagerWidget::init_qss()
{
    //1、打开文件
    QFile file(":/qss/deptArchive.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}


// ===============================================================================================
// ====================================== 网络请求 =========================================
// ===============================================================================================

void DeptManagerWidget::refreshDeptData()
{
    DEPT_PAGE_REQ req = {};
    req.page = m_currentPage;
    req.page_size = m_pageSize;

    HEAD head = { SERVICE_TYPE::GET_DEPT_LIST, sizeof(req) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));

    m_netconn->sendPacket(packet);
    //qDebug() << "[DeptManagerWidget] 请求科室档案, page =" << m_currentPage;
}


// ===============================================================================================
// ====================================== 网络响应 =========================================
// ===============================================================================================

void DeptManagerWidget::onDeptArchiveDataReceived(const QByteArray &body)
{
    if (body.size() < (int)sizeof(DEPT_PAGE_RESP_HEAD)) return;
    const auto* head = reinterpret_cast<const DEPT_PAGE_RESP_HEAD*>(body.constData());
    if (head->code != 0) {
        QMessageBox::warning(this, "错误", "获取科室档案失败");
        return;
    }

    // 更新分页状态
    m_totalPage = qMax(1, (head->total + m_pageSize - 1) / m_pageSize);
    ui->lab_page->setText(QString("%1 / %2").arg(m_currentPage).arg(m_totalPage));

    // 清空旧的卡片
    ui->listWidget->clear();

    const char* p = body.constData() + sizeof(DEPT_PAGE_RESP_HEAD);
    for (int i = 0; i < head->count; ++i) {
        const auto* item = reinterpret_cast<const DEPT_PAGE_ITEM*>(p);
        p += sizeof(DEPT_PAGE_ITEM);

        // 1. 创建卡片 Widget
        auto* card = new QWidget;
        //设置穿透鼠标
        card->setAttribute(Qt::WA_TransparentForMouseEvents);
        card->setStyleSheet("background: transparent;");

        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(0, 0, 0, 0);

        // 科室名
        auto* nameLab = new QLabel(QString::fromUtf8(item->dept_name,
                                                     qstrnlen(item->dept_name, sizeof(item->dept_name))));
        nameLab->setAlignment(Qt::AlignCenter);
        nameLab->setAttribute(Qt::WA_TransparentForMouseEvents);
        nameLab->setStyleSheet("font-size: 15px; font-weight: bold; color: #303133; background: transparent; border: none;");
        // 创建时间
        auto* timeLab = new QLabel("档案创建时间: " + QString::fromUtf8(item->created_date,
                                                                        qstrnlen(item->created_date, sizeof(item->created_date))));
        timeLab->setAlignment(Qt::AlignCenter);
        timeLab->setAttribute(Qt::WA_TransparentForMouseEvents);
        timeLab->setStyleSheet("font-size: 12px; color: #606266; background: transparent; border: none;");

        layout->addStretch();
        layout->addWidget(nameLab);
        layout->addWidget(timeLab);
        layout->addStretch();

        // 2. 塞入 QListWidget
        auto* listItem = new QListWidgetItem(ui->listWidget);
        listItem->setSizeHint(QSize(320, 120));
        listItem->setData(Qt::UserRole, item->dept_id);
        ui->listWidget->setItemWidget(listItem, card);
    }
}



// ===============================================================================================
// ====================================== 按钮槽函数 =========================================
// ===============================================================================================

void DeptManagerWidget::on_btn_prev_clicked()
{
    if (m_currentPage <= 1) {
        QMessageBox::warning(this, "提示", "已经是第一页了！");
        return;
    }
    m_currentPage--;
    refreshDeptData();
}


void DeptManagerWidget::on_btn_next_clicked()
{
    if (m_currentPage >= m_totalPage) {
        QMessageBox::warning(this, "提示", "已经是最后一页了！");
        return;
    }
    m_currentPage++;
    refreshDeptData();
}


void DeptManagerWidget::on_btn_add_clicked()
{
    AddDepartmentDialog dlg(-1,this);
    if (dlg.exec() == QDialog::Accepted) {
        // 3. 新增成功后，重置回第一页，并刷新列表
        m_currentPage = 1;
        refreshDeptData();
    }
}


void DeptManagerWidget::on_btn_edit_clicked()
{
    // 1. 获取当前选中的网格项
    QListWidgetItem* currentItem = ui->listWidget->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "提示", "请先点击选择一个要编辑的科室！");
        return;
    }
    // 2. 从 UserRole 中取出我们之前存进去的 dept_id
    int deptId = currentItem->data(Qt::UserRole).toInt();
    if (deptId <= 0) return;

    // 3. 将 deptId 传入弹窗（构造函数会根据 >0 自动进入编辑模式）
    AddDepartmentDialog dlg(deptId, this);

    // 4. 如果编辑成功并点击了确定，刷新当前列表
    if (dlg.exec() == QDialog::Accepted) {
        refreshDeptData(); // 重新请求当前页的数据，更新显示
    }
}

