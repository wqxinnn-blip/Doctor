#include "tonguereportdialog.h"
#include "winmanager.h"

TongueReportDialog::TongueReportDialog(int recordId, QWidget* parent)
    : QDialog(parent)
    , m_recordId(recordId)
{
    setUi();

    m_netconn = WinManager::getInstance()->getNetConn();

    connect(m_netconn, &NetConn::sigDataToTongueRecordDetail,
            this, &TongueReportDialog::onDetailReceived);
    connect(m_netconn, &NetConn::sigDataToTongueChunks,
            this, &TongueReportDialog::onChunkReceived);

    // 发详情请求
    requestDetail();
}

TongueReportDialog::~TongueReportDialog()
{
}

void TongueReportDialog::setUi()
{
    setWindowTitle("舌苔检测报告");
    setFixedSize(700, 500);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ===== 标题 =====
    auto* title = new QLabel("舌苔检测报告", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    // 左图右信息
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(24);

    // 左：图片
    m_imgLabel = new QLabel(this);
    m_imgLabel->setFixedSize(360, 360);
    m_imgLabel->setAlignment(Qt::AlignCenter);
    m_imgLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #f5f7fa;"
        "  border: 1px solid #e4e7ed;"
        "  border-radius: 8px;"
        "}");
    m_imgLabel->setText("加载中...");
    contentLayout->addWidget(m_imgLabel);

    // 右：信息
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(16);

    m_resultLabel = new QLabel(this);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(m_resultLabel);

    m_confLabel = new QLabel(this);
    m_confLabel->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(m_confLabel);

    m_timeLabel = new QLabel(this);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet("color: #909399;");
    infoLayout->addWidget(m_timeLabel);

    m_warnLabel = new QLabel(this);
    m_warnLabel->setAlignment(Qt::AlignCenter);
    m_warnLabel->setWordWrap(true);
    m_warnLabel->setStyleSheet(
        "color: #e74c3c;"
        "background-color: #fef0f0;"
        "border: 1px solid #fbc4c4;"
        "border-radius: 4px;"
        "padding: 8px;");
    m_warnLabel->hide();
    infoLayout->addWidget(m_warnLabel);

    infoLayout->addStretch();

    contentLayout->addLayout(infoLayout, 1);
    root->addLayout(contentLayout, 1);

    // ===== 关闭按钮 =====
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_btnClose = new QPushButton("关闭", this);
    m_btnClose->setObjectName("btn_close");
    m_btnClose->setFixedSize(100, 36);
    btnLayout->addWidget(m_btnClose);
    btnLayout->addStretch();
    root->addLayout(btnLayout);

    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    // 样式
    setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { color: #303133; }"
        "QPushButton#btn_close {"
        "  color: #ffffff;"
        "  background-color: #409eff;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton#btn_close:hover { background-color: #66b1ff; }");
}


void TongueReportDialog::setReportData(const QByteArray& imgData,
                                       int resultType,
                                       double confidence,
                                       const QString& createdAt,
                                       bool isSevere)
{
    // ===== 图片 =====
    QPixmap pix;
    pix.loadFromData(imgData);
    m_imgLabel->setPixmap(pix.scaled(m_imgLabel->size(),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
    // 分类名
    QString typeName = getTypeName(resultType);
    // 颜色显示
    QColor color = getTypeColor(resultType);
    // 结果文本
    QString resultHtml = QString(
                             "<div style='text-align:center;'>"
                             "<span style='font-size:14px; color:#909399;'>检测结果</span><br/>"
                             "<span style='font-size:22px; font-weight:bold; color:%1;'>%2</span>"
                             "</div>").arg(color.name(), typeName);


    // 填充信息
    m_resultLabel->setText(resultHtml);
    m_resultLabel->setTextFormat(Qt::RichText);

    // ===== 置信度 =====
    m_confLabel->setText(QString("置信度：%1%").arg(confidence, 0, 'f', 1));

    // ===== 时间 =====
    m_timeLabel->setText("检测时间：" + createdAt);

    // ===== 重症警告 =====
    if (isSevere) {
        m_warnLabel->setText("⚠️ 检测到重症舌苔，建议尽快就医");
        m_warnLabel->show();
    } else {
        m_warnLabel->hide();
    }
}

QString TongueReportDialog::getTypeName(int resultType)
{
    switch (resultType) {
    case 0: return "灰黑苔";
    case 1: return "镜面舌";
    case 2: return "薄白苔";
    case 3: return "白腻苔";
    case 4: return "黄腻苔";
    default: return "未识别";
    }
}

QColor TongueReportDialog::getTypeColor(int resultType)
{
    switch (resultType) {
    case 0: return QColor(0xC0, 0x39, 0x2B);  // 灰黑苔 - 深红
    case 1: return QColor(0xE7, 0x4C, 0x3C);  // 镜面舌 - 红
    case 2: return QColor(0x67, 0xC2, 0x3A);  // 薄白苔 - 绿（健康）
    case 3: return QColor(0xE6, 0xA2, 0x3C);  // 白腻苔 - 橙
    case 4: return QColor(0xF3, 0x9C, 0x12);  // 黄腻苔 - 黄
    default: return QColor(0x90, 0x93, 0x99); // 未识别 - 灰
    }
}


void TongueReportDialog::requestDetail()
{
    TONGUE_RECORD_DETAIL_REQ req = {};
    req.tongue_record_id = m_recordId;

    HEAD head = { SERVICE_TYPE::TONGUE_RECORD_DETAIL, sizeof(req) };
    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&head), sizeof(HEAD));
    packet.append(reinterpret_cast<const char*>(&req), sizeof(req));
    m_netconn->sendPacket(packet);
}

void TongueReportDialog::requestChunk(int chunkIndex)
{
    TONGUE_IMAGE_CHUNK_REQ req = {};
    req.tongue_record_id = m_recordId;
    req.chunk_index = chunkIndex;

    HEAD head = { SERVICE_TYPE::TONGUE_IMAGE_CHUNK, sizeof(req) };

    QByteArray packet;
    packet.resize(sizeof(HEAD) + sizeof(req));
    char* data = packet.data();
    memcpy(data, &head, sizeof(HEAD));
    memcpy(data + sizeof(HEAD), &req, sizeof(req));

    m_netconn->sendPacket(packet);
}

void TongueReportDialog::onDetailReceived(const QByteArray& body)
{
    if (body.size() < (int)sizeof(TONGUE_RECORD_DETAIL_RESP)) return;

    TONGUE_RECORD_DETAIL_RESP resp = {};
    memcpy(&resp, body.constData(), sizeof(resp));

    if (resp.code != 0) {
        QMessageBox::warning(this, "提示",QString::fromUtf8(resp.message,
                                    qstrnlen(resp.message, sizeof(resp.message))));
        reject();
        return;
    }

    // 暂存详情字段
    m_resultType  = resp.result_type;
    m_confidence  = resp.confidence;
    m_isSevere    = (resp.is_severe == 1);
    m_transferMode = resp.transfer_mode;
    m_imageSize   = resp.image_size;

    char createdAt[sizeof(resp.created_at) + 1] = {0};
    strncpy(createdAt, resp.created_at, sizeof(resp.created_at));
    m_createdAt = QString::fromUtf8(createdAt);

    if (resp.transfer_mode == 0) {
       // 不分片
        int expectedSize = sizeof(TONGUE_RECORD_DETAIL_RESP) + resp.image_size;
        if (body.size() < expectedSize) {
            qWarning() << "内联数据不完整";
            return;
        }
        QByteArray imgData = body.mid(sizeof(TONGUE_RECORD_DETAIL_RESP),
                                      resp.image_size);
        setReportData(imgData ,m_resultType,m_confidence,m_createdAt,m_isSevere);
    }
    else {
        // 分片
        m_chunkMap.clear();
        m_expectedChunks = resp.chunk_total;
        m_imgLabel->setText("图片加载中...");
        requestChunk(0);
    }
}

void TongueReportDialog::onChunkReceived(const QByteArray& body)
{
    if (body.size() != (int)sizeof(TONGUE_IMAGE_CHUNK_RESP)) return;

    TONGUE_IMAGE_CHUNK_RESP resp = {};
    memcpy(&resp, body.constData(), sizeof(resp));
    // qDebug() << "code =" << resp.code
    //          << "chunk_index =" << resp.chunk_index
    //          << "chunk_total =" << resp.chunk_total
    //          << "chunk_size =" << resp.chunk_size;

    if (resp.code != 0) {
        qWarning() << "分片" << resp.chunk_index << "失败:" << resp.msg;
        m_imgLabel->setText("图片加载失败");
        return;
    }

    if (resp.chunk_index < 0 || resp.chunk_index >= resp.chunk_total) {
        qWarning() << "非法 chunk_index:" << resp.chunk_index;
        return;
    }

    if (m_chunkMap.contains(resp.chunk_index)) {
        qDebug() << "分片" << resp.chunk_index << "已存在，忽略";
        return;
    }

    QByteArray chunk(resp.chunk_data, resp.chunk_size);
    m_chunkMap.insert(resp.chunk_index, chunk);

    qDebug() << "收到分片" << resp.chunk_index + 1 << "/" << resp.chunk_total
             << "，累计" << m_chunkMap.size() << "片";

    if (m_chunkMap.size() == m_expectedChunks) {
        assembleAndShow();
    } else {
        requestChunk(resp.chunk_index + 1);
    }
}


void TongueReportDialog::assembleAndShow()
{
    QByteArray fullImage;
    for (int i = 0; i < m_expectedChunks; ++i) {
        if (!m_chunkMap.contains(i)) {
            qWarning() << "缺少分片:" << i;
            m_imgLabel->setText("图片数据不完整");
            return;
        }
        fullImage.append(m_chunkMap.value(i));
    }

    qDebug() << "图片收齐，总大小:" << fullImage.size();
    setReportData(fullImage,m_resultType,m_confidence,m_createdAt,m_isSevere);
}