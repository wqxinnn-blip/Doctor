#include "workloadstatswidget.h"
#include "../winmanager.h"

WorkloadStatsWidget::WorkloadStatsWidget(QWidget *parent)
    : QWidget{parent}
    , m_currentMode(1)
{
    m_netconn = WinManager::getInstance()->getNetConn();
    connect(m_netconn, &NetConn::sigDataToWorkloadStats, this, &WorkloadStatsWidget::onStatsReceived);

    initUi();
    init_qss();
}

WorkloadStatsWidget::~WorkloadStatsWidget()
{
}

void WorkloadStatsWidget::initUi()
{
    // 主布局
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // 1. 创建滚动区域
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 自适应大小
    scrollArea->setFrameShape(QFrame::NoFrame); //去除边框
    scrollArea->setObjectName("mainScrollArea");

    // 2. 创建内容容器
    auto* contentWidget = new QWidget();
    contentWidget->setObjectName("contentWidget");
    // 给内容容器设置主布局（垂直布局）
    auto* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20); // 布局周围内边距
    mainLayout->setSpacing(15); // 内部空间的间距

    // ================= 1. 顶部工具栏与指标卡片 =================
    auto* topBarLayout = new QHBoxLayout();
    // 周/月 切换按钮
    m_btnWeek = new QPushButton("本周", this);
    m_btnMonth = new QPushButton("本月", this);
    m_btnWeek->setCheckable(true);
    m_btnMonth->setCheckable(true);
    m_btnWeek->setObjectName("tabBtn");
    m_btnMonth->setObjectName("tabBtn");

    connect(m_btnWeek, &QPushButton::clicked, this, [this](){
        m_currentMode = 1; // 本周
        onPeriodChanged();
    });
    connect(m_btnMonth, &QPushButton::clicked, this, [this](){
        m_currentMode = 2; // 本月
        onPeriodChanged();
    });

    topBarLayout->addWidget(m_btnWeek);
    topBarLayout->addWidget(m_btnMonth);
    topBarLayout->addSpacing(30);

    // 4个指标卡片
    topBarLayout->addWidget(createSummaryCard("总预约量", m_labBookingVal, m_trendBooking));
    topBarLayout->addWidget(createSummaryCard("总就诊量", m_labVisitVal, m_trendVisit));
    topBarLayout->addWidget(createSummaryCard("整体就诊率", m_labVisitRateVal, m_trendRate));
    topBarLayout->addWidget(createSummaryCard("整体失约率", m_labMissRateVal, m_trendMiss));
    topBarLayout->addStretch();

    // 导出按钮
    auto* btnExport = new QPushButton("📊 导出报表", this);
    btnExport->setObjectName("btnExport");
    btnExport->setCursor(Qt::PointingHandCursor);
    connect(btnExport, &QPushButton::clicked, this, &WorkloadStatsWidget::onExportClicked);
    topBarLayout->addWidget(btnExport);

    mainLayout->addLayout(topBarLayout);

    // ================= 2. 中间图表 =================
    auto* midLayout = new QHBoxLayout();
    midLayout->setSpacing(20);

    // 左：各科室预约量对比
    QWidget* card1 = createCard("各科室预约量对比", nullptr, m_volumeView);
    card1->setMinimumHeight(320);
    midLayout->addWidget(card1, 6);

    // 右：各科室转化率对比
    m_comboRateMetric = new QComboBox(this);
    m_comboRateMetric->addItems({"就诊率", "失约率", "取消率"});
    connect(m_comboRateMetric, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WorkloadStatsWidget::onRateMetricChanged);

    QWidget* card2 = createCard("科室转化率分析", m_comboRateMetric, m_rateView);
    card2->setMinimumHeight(320);
    midLayout->addWidget(card2, 4);

    mainLayout->addLayout(midLayout);

    // ================= 3. 底部图表 =================
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(20);

    // 左：医生接诊排行榜
    QWidget* card3 = createCard("医生接诊量 Top5", nullptr, m_docRankView);
    card3->setMinimumHeight(300);
    bottomLayout->addWidget(card3, 4);

    // 右：近30天接诊趋势
    QWidget* card4 = createCard("近30天接诊趋势", nullptr, m_trendView);
    card4->setMinimumHeight(300);
    bottomLayout->addWidget(card4, 6);

    mainLayout->addLayout(bottomLayout);
    mainLayout->addStretch();

    scrollArea->setWidget(contentWidget); // 把内容装进滚动区
    outerLayout->addWidget(scrollArea);   // 把滚动区装进主布局
}

void WorkloadStatsWidget::init_qss()
{
    //1、打开文件
    QFile file(":/qss/medicalStatics.qss");
    if(file.open(QFile::ReadOnly | QFile::Text)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

// 辅助构建图表卡片
QWidget* WorkloadStatsWidget::createCard(const QString& title, QWidget* topRightWidget, QChartView*& chartView)
{
    auto* card = new QWidget(this);
    card->setObjectName("chartCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    auto* header = new QHBoxLayout();
    auto* titleLab = new QLabel(title, card);
    titleLab->setObjectName("cardTitle");

    header->addWidget(titleLab);
    header->addStretch();
    if (topRightWidget) {
        header->addWidget(topRightWidget);
    }
    layout->addLayout(header);

    chartView = new QChartView(card);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView, 1);

    return card;
}


// 辅助构建指标卡片
QWidget* WorkloadStatsWidget::createSummaryCard(const QString& title, QLabel*& valueLabel, QLabel*& trendLabel)
{
    // 容器
    auto* card = new QWidget(this);
    card->setObjectName("summaryCard");
    card->setFixedSize(140, 70);
    // 布局
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);
    //
    auto* titleLab = new QLabel(title, card);
     titleLab->setObjectName("summaryTitle");

    auto* valLayout = new QHBoxLayout();
    valueLabel = new QLabel("0", card);
    valueLabel->setObjectName("summaryValue");

    trendLabel = new QLabel("", card);
    trendLabel->setObjectName("summaryTrend");
    trendLabel->setAlignment(Qt::AlignBottom | Qt::AlignRight);

    valLayout->addWidget(valueLabel);
    valLayout->addStretch();
    valLayout->addWidget(trendLabel);

    layout->addWidget(titleLab);
    layout->addLayout(valLayout);
    return card;
}



// ================== 网络与数据更新 ==================

void WorkloadStatsWidget::onEnter()
{
    requestData(m_currentMode);
}

void WorkloadStatsWidget::onPeriodChanged()
{
    m_btnWeek->setChecked(m_currentMode == 1);
    m_btnMonth->setChecked(m_currentMode == 2);
    requestData(m_currentMode);
}

void WorkloadStatsWidget::requestData(int mode)
{
    WORKLOAD_STATS_REQ req = {};
    req.mode = mode;

    HEAD head = { SERVICE_TYPE::GET_WORKLOAD_STATS, sizeof(req) };

    QByteArray packet;
    packet.resize(sizeof(HEAD) + sizeof(req));
    char* p = packet.data();

    memcpy(p, &head, sizeof(HEAD)); p += sizeof(HEAD);
    memcpy(p, &req, sizeof(req));

    m_netconn->sendPacket(packet);
    qDebug() << "[WorkloadStatsWidget] 请求工作量统计, mode =" << mode;
}


void WorkloadStatsWidget::updateUi(const WORKLOAD_STATS_RESP& resp, const QByteArray& body)
{
    // 更新顶部卡片 数值
    m_labBookingVal->setText(QString::number(resp.summary.total_bookings));
    m_labVisitVal->setText(QString::number(resp.summary.total_visits));
    m_labVisitRateVal->setText(QString::number(resp.summary.visit_rate, 'f', 1) + "%");
    m_labMissRateVal->setText(QString::number(resp.summary.miss_rate, 'f', 1) + "%");

    // 更新环比
    updateTrendLabel(m_trendBooking, resp.summary.booking_growth);
    updateTrendLabel(m_trendVisit, resp.summary.visit_growth);
    updateTrendLabel(m_trendRate, resp.summary.rate_growth, true); // 率是百分点
    updateTrendLabel(m_trendMiss, resp.summary.miss_rate_growth, true);

    // 更新图表
    renderVolumeChart(body, resp);
    renderRateChart(body, resp);
    renderDoctorRankChart(body, resp);
    renderTrendChart(body, resp);
}

void WorkloadStatsWidget::updateTrendLabel(QLabel *label, double value, bool isRate)
{
    if (value > 0) {
        label->setText(QString("+%1%").arg(value, 0, 'f', 1));
        label->setStyleSheet("color: #F56C6C;"); // 红色
    } else if (value < 0) {
        label->setText(QString("%1%").arg(value, 0, 'f', 1));
        label->setStyleSheet("color: #67C23A;"); // 绿色
    } else {
        label->setText("0.0%");
        label->setStyleSheet("color: #909399;"); // 灰色
    }

    //
    if (std::abs(value) > 999.0) {
        label->setText(">999%");
    } else if (!isRate && std::abs(value) > 100.0) {
        label->setText("暴增");
    }
}

// ================== 图表渲染 ==================

// 1. 双柱状图 (本期 vs 上期)
void WorkloadStatsWidget::renderVolumeChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp)
{
    // 根据当前模式动态设置图例名称
    QString nameCurrent = (m_currentMode == 1) ? "本周" : "本月";
    QString nameLast = (m_currentMode == 1) ? "上周" : "上月";

    auto* setCurrent = new QBarSet(nameCurrent);
    auto* setLast = new QBarSet(nameLast);
    // 两个柱状图数据集合
    setCurrent->setColor(QColor("#409EFF"));
    setLast->setColor(QColor("#C0C4CC"));

    QStringList categories;
    const char* p = body.constData() + sizeof(WORKLOAD_STATS_RESP);
    int maxNum = 0;
    // 遍历科室数量（已过滤没有数据的科室）
    for (int i = 0; i < resp.dept_volume_count; ++i) {
        const auto* item = reinterpret_cast<const DEPT_VOLUME_ITEM*>(p);
        // 添加科室名
        categories << QString::fromUtf8(item->dept_name);
        // 加入本期预约量
        *setCurrent << item->current_period;
        // 加入往期预约量
        *setLast << item->last_period;
        int max = item->current_period >= item->last_period ? item->current_period : item->last_period;
        if(max > maxNum) maxNum = max;
        p += sizeof(DEPT_VOLUME_ITEM);
    }

    // 构建数据系列 柱状图
    auto* series = new QBarSeries();
    series->append(setCurrent);
    series->append(setLast);

    series->setLabelsVisible(false); // 显示数字
    // 绑定悬停事件
    connect(series, &QBarSeries::hovered, this, [=](bool status, int index, QBarSet *barset) {
        if (status) {
            QToolTip::showText(QCursor::pos(),
                               QString("%1\n%2: %3").arg(categories[index]).arg(barset->label()).arg(barset->at(index)),
                               this);
        } else {
            QToolTip::hideText();
        }
    });
    auto* chart = new QChart();
    chart->addSeries(series);
    // 柱子增长动画
    chart->setAnimationOptions(QChart::SeriesAnimations);
    // 背景透明
    chart->setBackgroundVisible(false);
    // 图例放在底部
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Microsoft YaHei", 9));
    chart->legend()->setLabelColor(QColor("#606266"));

    auto* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor("#606266")); // 坐标轴文字用深灰色
    axisX->setLabelsFont(QFont("Microsoft YaHei", 9));
    // 倾斜横坐标文字
    if (categories.size() > 6) {
        axisX->setLabelsAngle(-45);
    }

    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);// 系列和轴绑定

    auto* axisY = new QValueAxis();
    axisY->setLabelsColor(QColor("#606266"));
    axisY->setLabelsFont(QFont("Microsoft YaHei", 9));
    axisY->setGridLineColor(QColor("#EBEEF5"));

    // 1. 强制标签格式为整数
    axisY->setLabelFormat("%d");
    // 2. 动态计算一个的整数最大值和刻度间隔
    int tickCount = 6;
    // 预留 20% 的顶部空间
    int maxRange = qCeil(maxNum * 1.2);
    // 如果全是 0，给个默认高度
    if (maxRange <= 0) maxRange = 10;

    // 计算每个区间的步长（向上取整，保证是整数）
    int interval = qCeil(static_cast<double>(maxRange) / (tickCount - 1));
    if (interval < 1) interval = 1; // 步长至少是 1

    // 设定 Y 轴范围 (0 到 步长 * 区间数)
    axisY->setRange(0, interval * (tickCount - 1));
    axisY->setTickCount(tickCount);
    // ============================================
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    m_volumeView->setChart(chart);
}

// 2. 堆叠柱状图 (转化率)
void WorkloadStatsWidget::renderRateChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp)
{
    // 0=就诊, 1=失约, 2=取消
    int metric = m_comboRateMetric->currentIndex();

    // 1. 根据下拉框当前选中的项，决定系列的颜色和名称
    QString setName;
    QColor setColor;
    if (metric == 0) {
        setName = "就诊率";
        setColor = QColor("#67C23A"); // 绿
    } else if (metric == 1) {
        setName = "失约率";
        setColor = QColor("#F56C6C"); // 红
    } else {
        setName = "取消率";
        setColor = QColor("#909399"); // 灰
    }

    auto* set = new QBarSet(setName);
    set->setColor(setColor);

    QStringList categories;
    const char* p = body.constData() + sizeof(WORKLOAD_STATS_RESP) + resp.dept_volume_count * sizeof(DEPT_VOLUME_ITEM);

    for (int i = 0; i < resp.dept_rate_count; ++i) {
        const auto* item = reinterpret_cast<const DEPT_RATE_ITEM*>(p);
        categories << QString::fromUtf8(item->dept_name, qstrnlen(item->dept_name, sizeof(item->dept_name)));
        // 2. 只取当前选中的指标，塞入数据
        if (metric == 0) {
            *set << qCeil(item->visit_rate);
        } else if (metric == 1) {
            *set << qCeil(item->miss_rate);
        } else {
            *set << qCeil(item->cancel_rate);
        }
        p += sizeof(DEPT_RATE_ITEM);
    }

    // 3. 使用普通的 QBarSeries，而不是 QStackedBarSeries
    auto* series = new QBarSeries();
    series->append(set);

    // 让柱子上显示具体数值 (如 "85.5%")
    series->setLabelsVisible(true);
    series->setLabelsFormat("@value%"); // 数字后面加%
    series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd); // 标签显示在柱子上方

    auto* chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);

    chart->legend()->setVisible(false);

    auto* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor("#606266"));
    axisX->setLabelsFont(QFont("Microsoft YaHei", 9));

    // 如果科室比较多，把文字倾斜防重叠
    if (categories.size() > 6) axisX->setLabelsAngle(-45);

    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d%%");
    axisY->setLabelsColor(QColor("#606266"));
    axisY->setLabelsFont(QFont("Microsoft YaHei", 9));
    axisY->setGridLineColor(QColor("#EBEEF5"));

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    m_rateView->setChart(chart);
}

// 3. 横向条形图 (医生 Top5)
void WorkloadStatsWidget::renderDoctorRankChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp)
{
    auto* set = new QBarSet("接诊量");
    set->setColor(QColor("#409EFF")); //蓝色

    QStringList categories;
    // 偏移到医生数据段
    int offset = sizeof(WORKLOAD_STATS_RESP)
                 + resp.dept_volume_count * sizeof(DEPT_VOLUME_ITEM)
                 + resp.dept_rate_count * sizeof(DEPT_RATE_ITEM);
    const char* p = body.constData() + offset;

    int maxVal = 0;
    // 添加数据
    for (int i = resp.doc_rank_count - 1; i >= 0; --i) {
        // 计算当前项的指针位置
        const auto* item = reinterpret_cast<const DOCTOR_RANK_ITEM*>(p + i * sizeof(DOCTOR_RANK_ITEM));
        // 提取字符串
        QString docName = QString::fromUtf8(item->doctor_name, qstrnlen(item->doctor_name, sizeof(item->doctor_name)));
        categories << docName;
        int count = item->visit_count;
        *set << count;
        if (count > maxVal) {
            maxVal = count;
        }
    }


    // 水平柱状图
    auto* series = new QHorizontalBarSeries();
    series->append(set);
    series->setLabelsVisible(true);
    series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd); // 标签显示在柱子外面
    series->setLabelsFormat("@value 人");

    auto* chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false); // 背景透明
    chart->legend()->setVisible(false); // 不显示图例

    // 医生姓名
    auto* axisY = new QBarCategoryAxis();
    axisY->append(categories);
    axisY->setLabelsColor(QColor("#606266"));
    axisY->setLabelsFont(QFont("Microsoft YaHei", 9));
    axisY->setGridLineVisible(false);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);


    // 值
    auto* axisX = new QValueAxis();
    int finalMax = maxVal;
    if (finalMax <= 5) finalMax = 10; // 数据太小，X轴直接给 0~10
    else finalMax = finalMax + 2;
    axisX->setRange(0, finalMax);

    axisX->setLabelFormat("%.0f");
    axisX->setTickCount(5);
    axisX->setLabelsColor(QColor("#606266"));
    axisX->setLabelsFont(QFont("Microsoft YaHei", 9));
    axisX->setGridLineColor(QColor("#EBEEF5"));
    axisX->setLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    m_docRankView->setChart(chart);
}

// 4. 面积折线图 (近30天)
void WorkloadStatsWidget::renderTrendChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp)
{
    auto* upperSeries = new QLineSeries();
    auto* lowerSeries = new QLineSeries(); // 用于填充面积

    // 偏移到日数据段 (最后30个int)
    const int* dailyData = reinterpret_cast<const int*>(body.constData() + body.size() - 30 * sizeof(int));

    int maxVal = 0;
    for (int i = 0; i < 30; ++i) {
        upperSeries->append(i + 1, dailyData[i]);
        lowerSeries->append(i + 1, 0);
        if (dailyData[i] > maxVal) maxVal = dailyData[i];
    }

    auto* areaSeries = new QAreaSeries(upperSeries, lowerSeries);
    areaSeries->setColor(QColor(64, 158, 255, 100)); // 带透明度的蓝色填充
    areaSeries->setBorderColor(QColor("#409EFF"));

    static QPoint lastHoverPos(-1, -1);
    connect(areaSeries, &QAreaSeries::hovered, this, [=](const QPointF &point) {
        // point 是实际的数据坐标 (x, y)
        int day = qRound(point.x());
        int count = qRound(point.y());

        if (day >= 1 && day <= 30) {
            // 防止鼠标微小移动导致高频重绘 Tooltip
            QPoint currentPos = QCursor::pos();
            if (lastHoverPos != currentPos) {
                QToolTip::showText(currentPos,QString("第 %1 天\n接诊量: %2 人").arg(day).arg(count),this);
                lastHoverPos = currentPos;
            }
        }
    });

    auto* chart = new QChart();
    chart->addSeries(areaSeries);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(false);
    // 开启抗锯齿
    chart->setMargins(QMargins(5, 5, 5, 5));

    auto* axisX = new QValueAxis();
    axisX->setRange(1, 30);
    axisX->setTickCount(6);
    axisX->setLabelFormat("%d");
    axisX->setLabelsColor(QColor("#606266"));
    axisX->setLabelsFont(QFont("Microsoft YaHei", 9));

    axisX->setLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    areaSeries->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    axisY->setRange(0, maxVal * 1.2);
    axisY->setLabelsColor(QColor("#606266"));
    axisY->setLabelsFont(QFont("Microsoft YaHei", 9));
    axisY->setGridLineColor(QColor("#EBEEF5"));
    axisY->setLineVisible(false);

    chart->addAxis(axisY, Qt::AlignLeft);
    areaSeries->attachAxis(axisY);

    m_trendView->setMouseTracking(true);
    m_trendView->installEventFilter(this);
    m_trendView->setChart(chart);
}

// ================== 槽函数 ==================

void WorkloadStatsWidget::onRateMetricChanged(int index)
{
    Q_UNUSED(index);
    // 指标切换时，重新渲染堆叠图
    if (m_lastBody.isEmpty()) return;
    renderRateChart(m_lastBody, m_lastResp);
}

void WorkloadStatsWidget::onExportClicked()
{
    QMessageBox::information(this, "导出成功", "工作量统计报表已导出至桌面！");
}

void WorkloadStatsWidget::onStatsReceived(const QByteArray &body)
{
    if (body.size() < (int)sizeof(WORKLOAD_STATS_RESP)) return;
    const auto* resp = reinterpret_cast<const WORKLOAD_STATS_RESP*>(body.constData());

    // 1. 保存最后一次的数据，供切换指标时重绘
    m_lastBody = body;
    m_lastResp = *resp;

    // 2. 校验变长包体长度是否匹配
    int expectedSize = sizeof(WORKLOAD_STATS_RESP)
                       + resp->dept_volume_count * sizeof(DEPT_VOLUME_ITEM)
                       + resp->dept_rate_count * sizeof(DEPT_RATE_ITEM)
                       + resp->doc_rank_count * sizeof(DOCTOR_RANK_ITEM)
                       + 30 * sizeof(int); // 30天趋势

    if (body.size() < expectedSize) {
        qWarning() << "工作量统计数据包不完整，可能被截断";
        return;
    }
    // 3. 刷新 UI
    updateUi(*resp, body);
}