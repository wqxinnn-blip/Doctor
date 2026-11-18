#ifndef WORKLOADSTATSWIDGET_H
#define WORKLOADSTATSWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFile>
#include <QChartView>
#include <QChart>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QBarSeries>
#include <QScrollArea>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QHorizontalBarSeries>
#include <QLineSeries>
#include <QAreaSeries>
#include <QPieSeries>
#include <QToolTip>
#include "../protocol.h"
#include "../netconn.h"

class WorkloadStatsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WorkloadStatsWidget(QWidget *parent = nullptr);
    ~WorkloadStatsWidget();

    void onEnter(); // 每次进入页面调用

private slots:
    void onPeriodChanged(); // 切换周/月
    void onRateMetricChanged(int index); // 切换比例指标
    void onExportClicked(); // 导出报表

    void onStatsReceived(const QByteArray& body);

private:
    void initUi();
    void init_qss();
    void requestData(int mode);
    void updateUi(const WORKLOAD_STATS_RESP& resp, const QByteArray& body);
    void updateTrendLabel(QLabel* label, double value, bool isRate = false);

    // 构建各种图表的函数
    QWidget* createCard(const QString& title, QWidget* topRightWidget, QChartView*& chartView);
    QWidget* createSummaryCard(const QString& title, QLabel*& valueLabel, QLabel*& trendLabel);
    void renderVolumeChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp);
    void renderRateChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp);
    void renderDoctorRankChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp);
    void renderTrendChart(const QByteArray& body, const WORKLOAD_STATS_RESP& resp);

    NetConn* m_netconn;

    // 顶部按钮和切换控件
    QPushButton* m_btnWeek;
    QPushButton* m_btnMonth;
    QComboBox* m_comboRateMetric;

    // 指标卡片的值
    QLabel* m_labBookingVal;
    QLabel* m_labVisitVal;
    QLabel* m_labVisitRateVal;
    QLabel* m_labMissRateVal;

    QLabel* m_trendBooking;
    QLabel* m_trendVisit;
    QLabel* m_trendRate;
    QLabel* m_trendMiss;

    // 图表视图
    QChartView* m_volumeView;
    QChartView* m_rateView;
    QChartView* m_docRankView;
    QChartView* m_trendView;

    // 缓存数据以便切换
    QByteArray m_lastBody;
    WORKLOAD_STATS_RESP m_lastResp;
    int m_currentMode; // 1=周, 2=月
};

#endif // WORKLOADSTATSWIDGET_H
