#include "aireportdialog.h"
#include "decopainter.h"
#include "sketchyui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPainter>
#include <QPainterPath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QDate>
#include <QUrl>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QTextCursor>
#include <QFrame>
#include <QRandomGenerator>
#include <algorithm>

static const QColor C_CREAM  = QColor("#FDFBF7");
static const QColor C_INK    = QColor("#2B2B2B");
static const QColor C_SHADOW = QColor("#3A3530");

// ═══════════════════════════════════════════
//  Helper: report JSON 存储
// ═══════════════════════════════════════════

static QString reportPath() {
    return QCoreApplication::applicationDirPath() + "/diet_reports.json";
}

static QVector<ReportEntry> loadReportEntries() {
    QVector<ReportEntry> entries;
    QFile f(reportPath());
    if (!f.open(QIODevice::ReadOnly)) return entries;
    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const auto &v : arr) {
        QJsonObject o = v.toObject();
        entries.push_back({ o["date"].toString(), o["type"].toString(), o["content"].toString() });
    }
    return entries;
}

static void saveReportEntry(const QString &date, const QString &type, const QString &content) {
    auto entries = loadReportEntries();
    ReportEntry e{ date, type, content };
    // 最新生成的放最前面
    entries.insert(entries.begin(), e);
    // 最多保留 50 条
    if (entries.size() > 50) entries.resize(50);

    QJsonArray arr;
    for (const auto &r : entries) {
        QJsonObject o;
        o["date"] = r.date;
        o["type"] = r.type;
        o["content"] = r.content;
        arr.append(o);
    }
    QFile f(reportPath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

// ── Helper: API Key 存储 ──
static QString configPath() {
    return QCoreApplication::applicationDirPath() + "/ai_config.json";
}

static QString loadApiKey() {
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) return {};
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    return obj["api_key"].toString();
}

static void saveApiKey(const QString &key) {
    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly)) return;
    QJsonObject obj;
    obj["api_key"] = key;
    f.write(QJsonDocument(obj).toJson());
}

// ═══════════════════════════════════════════
//  ReportHistoryDialog（手绘风格，无顶部留白）
// ═══════════════════════════════════════════

ReportHistoryDialog::ReportHistoryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(680, 480);
    resize(760, 540);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    // 内容容器（与手绘卡片边框配合）
    QVBoxLayout *inner = new QVBoxLayout;
    inner->setContentsMargins(36, 24, 36, 20);
    inner->setSpacing(8);

    // 标题行：标题 + 关闭按钮
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *title = new QLabel(QString::fromUtf8("历史饮食报告"), this);
    title->setStyleSheet("font-size:20px;font-weight:bold;color:#2B2B2B;"
                         "font-family:'Microsoft YaHei';border:none;background:transparent;");
    titleRow->addWidget(title);
    titleRow->addStretch();

    m_closeBtn = new QPushButton(QString::fromUtf8("×"), this);
    m_closeBtn->setFixedSize(28, 28);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton { border: 1.5px solid #C8BEB4; border-radius: 8px;"
        "  background-color: #F5F0E8; color: #2B2B2B; font-size: 16px; font-weight: bold;"
        "  font-family:'Microsoft YaHei'; }"
        "QPushButton:hover { background-color: #EDE4D8; border-color: #B8A898; }");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    titleRow->addWidget(m_closeBtn);
    inner->addLayout(titleRow);

    // 分割线
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("border:none;border-top:1.5px solid #C8BEB4;background:transparent;");
    line->setFixedHeight(2);
    inner->addWidget(line);

    // 分割面板
    QSplitter *split = new QSplitter(Qt::Horizontal, this);

    m_list = new QListWidget(this);
    m_list->setMinimumWidth(160);
    m_list->setStyleSheet(
        "QListWidget { border: 1px solid #C8BEB4; border-radius: 8px; padding: 6px;"
        "  font-family:'Microsoft YaHei'; font-size:13px; background:#FAF8F5; }"
        "QListWidget::item { padding: 8px 10px; }"
        "QListWidget::item:selected { background-color: #D0DDE8; color: #2B2B2B; }");
    split->addWidget(m_list);

    m_viewer = new QTextEdit(this);
    m_viewer->setReadOnly(true);
    m_viewer->setStyleSheet(
        "QTextEdit { border: 1px solid #C8BEB4; border-radius: 8px; padding: 12px;"
        "  color: #2B2B2B; font-size: 13px; font-family:'Microsoft YaHei';"
        "  background-color: #FAF8F5; }");
    split->addWidget(m_viewer);

    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 3);
    inner->addWidget(split, 1);

    lay->addLayout(inner);

    connect(m_list, &QListWidget::currentRowChanged, this, &ReportHistoryDialog::showReport);

    loadReports();
}

void ReportHistoryDialog::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 61;

    QPainterPath sp = sketchyRect(card.translated(2.5, 3.5), seed + 100, 2.8);
    p.setBrush(QColor("#3A3530"));
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, QColor("#FDFBF7"), 18);
    drawInkBorder(&p, cp, QColor("#2B2B2B"), 3, 0.7);

    p.end();
}

void ReportHistoryDialog::loadReports() {
    m_entries = loadReportEntries();
    m_list->clear();
    for (const auto &e : m_entries) {
        QString label = QString("%1  %2").arg(e.date, e.type);
        m_list->addItem(label);
    }
    if (!m_entries.isEmpty()) {
        m_list->setCurrentRow(m_entries.size() - 1);
    }
}

void ReportHistoryDialog::showReport(int index) {
    if (index < 0 || index >= m_entries.size()) return;
    m_viewer->setMarkdown(m_entries[index].content);
}

// ═══════════════════════════════════════════
//  AiReportDialog
// ═══════════════════════════════════════════

AiReportDialog::AiReportDialog(const QMap<QString, DailyRecord> &records,
                                 double bmr, QWidget *parent)
    : QDialog(parent), m_records(records), m_bmr(bmr)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(640, 740);

    m_networkManager = new QNetworkAccessManager(this);
    m_loadingTimer = new QTimer(this);
    m_loadingTimer->setInterval(500);
    connect(m_loadingTimer, &QTimer::timeout, this, &AiReportDialog::onLoadingTick);

    // ── 布局 ──
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 30, 36, 20);
    mainLayout->setSpacing(8);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("饮 食 报 告"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size:22px;font-weight:bold;color:#2B2B2B;"
        "letter-spacing:8px;font-family:'Microsoft YaHei';"
        "border:none;background:transparent;");
    mainLayout->addWidget(titleLabel);

    // ── 模式选择 ──
    QHBoxLayout *modeRow = new QHBoxLayout;
    modeRow->setSpacing(16);
    modeRow->addStretch();

    m_todayBtn = new QPushButton(QString::fromUtf8("  今日饮食报告  "), this);
    m_todayBtn->setCursor(Qt::PointingHandCursor);
    m_todayBtn->setMinimumSize(160, 40);
    connect(m_todayBtn, &QPushButton::clicked, this, [this]{ onModeSelected(Today); });
    modeRow->addWidget(m_todayBtn);

    m_weekBtn = new QPushButton(QString::fromUtf8("  一周饮食报告  "), this);
    m_weekBtn->setCursor(Qt::PointingHandCursor);
    m_weekBtn->setMinimumSize(160, 40);
    connect(m_weekBtn, &QPushButton::clicked, this, [this]{ onModeSelected(Week); });
    modeRow->addWidget(m_weekBtn);

    modeRow->addStretch();
    mainLayout->addLayout(modeRow);

    // 状态标签
    m_statusLabel = new QLabel(QString::fromUtf8("点击【生成报告】生成你的今日饮食报告"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(
        "color:#8A7A6A;font-size:12px;font-family:'Microsoft YaHei';"
        "border:none;background:transparent;");
    mainLayout->addWidget(m_statusLabel);

    // 报告文本区
    m_reportText = new QTextEdit(this);
    m_reportText->setReadOnly(true);
    m_reportText->setStyleSheet(
        "QTextEdit {"
        "  background-color: #FAF8F5;"
        "  border: 1.5px solid #C8BEB4; border-radius: 10px;"
        "  padding: 14px;"
        "  color: #2B2B2B; font-size: 13px;"
        "  font-family: 'Microsoft YaHei'; line-height: 1.6;"
        "}");
    m_reportText->setMinimumHeight(380);
    mainLayout->addWidget(m_reportText);

    // ── 按钮行 ──
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();

    m_historyBtn = new QPushButton(QString::fromUtf8("历史报告"), this);
    m_historyBtn->setCursor(Qt::PointingHandCursor);
    m_historyBtn->setFixedSize(90, 36);
    m_historyBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1.5px solid #C8BEB4; border-radius: 8px;"
        "  background-color: #E8DDD0;"
        "  color: #5D4E3A; font-size: 12px;"
        "  font-family: 'Microsoft YaHei'; padding: 4px 12px;"
        "}"
        "QPushButton:hover { background-color: #DDD0C0; }"
        "QPushButton:pressed { background-color: #D0C0AC; }");
    connect(m_historyBtn, &QPushButton::clicked, this, &AiReportDialog::onHistoryClicked);
    btnRow->addWidget(m_historyBtn);

    btnRow->addSpacing(12);

    m_generateBtn = new QPushButton(QString::fromUtf8("生成报告"), this);
    m_generateBtn->setMinimumSize(130, 40);
    m_generateBtn->setCursor(Qt::PointingHandCursor);
    m_generateBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1.5px solid #B8A898; border-radius: 10px;"
        "  background-color: #D0DDE8;"
        "  color: #2B2B2B; font-size: 14px; font-weight: bold;"
        "  font-family: 'Microsoft YaHei'; padding: 6px 24px;"
        "}"
        "QPushButton:hover { background-color: #C0D0DC; }"
        "QPushButton:pressed { background-color: #B0C0CC; }"
        "QPushButton:disabled { background-color: #E0D8D0; color: #B0A898; }");
    connect(m_generateBtn, &QPushButton::clicked, this, &AiReportDialog::onGenerateReport);
    btnRow->addWidget(m_generateBtn);

    btnRow->addSpacing(12);

    m_closeBtn = new QPushButton(QString::fromUtf8("关闭"), this);
    m_closeBtn->setMinimumSize(100, 40);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1.5px solid #C8BEB4; border-radius: 10px;"
        "  background-color: #F0EBE4;"
        "  color: #2B2B2B; font-size: 14px;"
        "  font-family: 'Microsoft YaHei'; padding: 6px 20px;"
        "}"
        "QPushButton:hover { background-color: #E5E0D8; }"
        "QPushButton:pressed { background-color: #D8D0C8; }");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    btnRow->addWidget(m_closeBtn);

    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    // ── 右上角 × ──
    QPushButton *xBtn = new QPushButton(QString::fromUtf8("×"), this);
    xBtn->setObjectName("closeXBtn");
    xBtn->setFixedSize(28, 28);
    xBtn->setCursor(Qt::PointingHandCursor);
    xBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1.5px solid #C8BEB4; border-radius: 8px;"
        "  background-color: #F5F0E8;"
        "  color: #2B2B2B; font-size: 16px; font-weight: bold;"
        "  font-family: 'Microsoft YaHei';"
        "}"
        "QPushButton:hover { background-color: #EDE4D8; border-color: #B8A898; }"
        "QPushButton:pressed { background-color: #E0D4C4; }");
    connect(xBtn, &QPushButton::clicked, this, &QDialog::close);
    xBtn->move(width() - 38, 28);

    onModeSelected(Today);
    m_apiKey = loadApiKey();
}

// ── 模式选择 ──
void AiReportDialog::onModeSelected(ReportMode mode) {
    m_mode = mode;
    updateModeButtonStyles();

    QString hint = (mode == Today)
        ? QString::fromUtf8("点击【生成报告】生成你的今日饮食报告")
        : QString::fromUtf8("点击【生成报告】生成你的一周饮食报告");
    m_statusLabel->setText(hint);
    m_statusLabel->setStyleSheet(
        "color:#6A7A8A;font-size:12px;font-family:'Microsoft YaHei';"
        "border:none;background:transparent;");
}

// ── 历史报告 ──
void AiReportDialog::onHistoryClicked() {
    ReportHistoryDialog dlg(this);
    dlg.exec();
}

// ── 生成报告 ──
void AiReportDialog::onGenerateReport() {
    // 今日无记录 → 弹窗
    if (m_mode == Today) {
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        if (!m_records.contains(today) || m_records[today].dishes.isEmpty()) {
            QMessageBox box(this);
            box.setWindowTitle(QString::fromUtf8("提示"));
            box.setText(QString::fromUtf8("今天还没有饮食记录，快去吃饭吧！"));
            box.setIcon(QMessageBox::Information);
            box.setStyleSheet(
                "QLabel{font-family:'Microsoft YaHei';font-size:14px;}"
                "QPushButton{font-family:'Microsoft YaHei';padding:4px 20px;}");
            box.exec();
            return;
        }
    }

    // 检查 API Key
    if (m_apiKey.isEmpty()) {
        bool ok = false;
        QString key = QInputDialog::getText(
            this,
            QString::fromUtf8("请输入 API Key"),
            QString::fromUtf8("请在智谱AI控制台创建 API Key 并粘贴到这里：\n"
                              "https://open.bigmodel.cn/usercenter/apikeys"),
            QLineEdit::Password, {}, &ok);
        if (!ok || key.isEmpty()) return;
        m_apiKey = key;
        saveApiKey(key);
    }

    setButtonsEnabled(false);
    m_reportText->clear();
    m_fullText.clear();
    m_sseBuffer.clear();

    // 加载动画
    m_loadingDots = 0;
    m_reportText->setPlainText(QString::fromUtf8("正在生成报告"));
    m_statusLabel->setText(QString::fromUtf8("⏳ 报告生成中…"));
    m_loadingTimer->start();

    QString prompt = (m_mode == Today) ? buildTodayPrompt() : buildWeekPrompt();
    callApi(prompt);
}

// ── 加载动画 tick ──
void AiReportDialog::onLoadingTick() {
    m_loadingDots = (m_loadingDots + 1) % 6;
    QString dots;
    for (int i = 0; i <= m_loadingDots; ++i) dots += ".";
    m_reportText->setPlainText(QString::fromUtf8("正在生成报告%1\n\n请稍候，正在分析您的饮食数据…").arg(dots));
}

// ═══════════════════════════════════════════
//  饮食质量分析（用于自适应语气）
// ═══════════════════════════════════════════

void AiReportDialog::analyzeDietQuality(QString &toneHint, QString &dietQuality) const {
    if (m_records.isEmpty()) {
        toneHint = QString::fromUtf8("温和鼓励");
        dietQuality = QString::fromUtf8("暂无数据");
        return;
    }

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    double calRatio = 1.0;
    int vegCount = 0, meatCount = 0, stapleCount = 0;
    int totalDays = 0;

    QStringList recentDates = m_records.keys();
    std::sort(recentDates.begin(), recentDates.end());
    recentDates = recentDates.mid(qMax(0, recentDates.size() - 7));

    for (const auto &d : recentDates) {
        const auto &r = m_records[d];
        ++totalDays;
        for (const auto &dish : r.dishes) {
            // 粗略判断菜品类别
            if (dish.contains(QString::fromUtf8("菜")) || dish.contains(QString::fromUtf8("蔬"))
                || dish.contains(QString::fromUtf8("瓜")) || dish.contains(QString::fromUtf8("叶"))
                || dish.contains(QString::fromUtf8("番茄")) || dish.contains(QString::fromUtf8("青"))
                || dish.contains(QString::fromUtf8("白")) || dish.contains(QString::fromUtf8("萝")))
                ++vegCount;
            else if (dish.contains(QString::fromUtf8("肉")) || dish.contains(QString::fromUtf8("鸡"))
                     || dish.contains(QString::fromUtf8("鸭")) || dish.contains(QString::fromUtf8("鱼"))
                     || dish.contains(QString::fromUtf8("虾")) || dish.contains(QString::fromUtf8("牛"))
                     || dish.contains(QString::fromUtf8("羊")) || dish.contains(QString::fromUtf8("猪"))
                     || dish.contains(QString::fromUtf8("蛋")) || dish.contains(QString::fromUtf8("豆")))
                ++meatCount;
            if (dish.contains(QString::fromUtf8("饭")) || dish.contains(QString::fromUtf8("面"))
                || dish.contains(QString::fromUtf8("粉")) || dish.contains(QString::fromUtf8("包"))
                || dish.contains(QString::fromUtf8("饼")) || dish.contains(QString::fromUtf8("饺")))
                ++stapleCount;
        }
        if (m_bmr > 0 && r.totalCalories > 0) {
            calRatio = r.totalCalories / m_bmr;
        }
    }

    // 判断饮食质量
    bool hasVeg = (vegCount > 0);
    bool hasMeat = (meatCount > 0);
    bool hasStaple = (stapleCount > 0);
    bool isHighCal = (calRatio > 1.3);
    bool isLowCal = (calRatio < 0.6);
    bool diverse = (totalDays > 0 && (vegCount + meatCount + stapleCount) >= totalDays * 2);

    if (hasVeg && hasMeat && hasStaple && !isHighCal && diverse) {
        toneHint = QString::fromUtf8("大力鼓励、充分肯定");
        dietQuality = QString::fromUtf8("均衡健康、非常棒");
    } else if (hasVeg && hasMeat && !isHighCal) {
        toneHint = QString::fromUtf8("鼓励为主、温和提点");
        dietQuality = QString::fromUtf8("整体不错、有优化空间");
    } else if (isHighCal || (!hasVeg && hasMeat)) {
        toneHint = QString::fromUtf8("先肯定、再温柔提醒、最后鼓励");
        dietQuality = QString::fromUtf8("偏重油腻、需要温和调整");
    } else if (isLowCal || (!hasStaple)) {
        toneHint = QString::fromUtf8("关心为主、避免批评、温和提醒规律饮食");
        dietQuality = QString::fromUtf8("摄入偏少、需要关注");
    } else {
        toneHint = QString::fromUtf8("温和鼓励、正向引导");
        dietQuality = QString::fromUtf8("普通日常、平稳");
    }
}

// ═══════════════════════════════════════════
//  Prompt 构造
// ═══════════════════════════════════════════

QString AiReportDialog::buildTodayPrompt() const {
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    const auto &r = m_records[today];

    // 分析饮食质量
    QString toneHint, dietQuality;
    analyzeDietQuality(toneHint, dietQuality);

    QString prompt = QString::fromUtf8(
        "你是一位温暖的饮食陪伴者与营养师。请根据以下今日饮食数据，生成一份「当日饮食报告」。\n\n"
        "**用户今日饮食数据**\n"
        "- 日期：%1\n"
        "- 菜品：%2\n"
        "- 总热量：%3 kcal\n"
        "- 总花费：%4 元\n"
    ).arg(today)
     .arg(r.dishes.join("、"))
     .arg(r.totalCalories, 0, 'f', 0)
     .arg(r.totalPrice, 0, 'f', 1);

    if (m_bmr > 0) {
        prompt += QString::fromUtf8("- 基础代谢率(BMR)：%1 kcal/天\n").arg(m_bmr, 0, 'f', 0);
    }

    prompt += QString::fromUtf8(
        "\n**饮食状态评估**（基于数据分析）：\n"
        "- 整体评价：%1\n"
        "- 语气策略：%2\n"
        "\n请根据以上评估调整语气——饮食质量高则多鼓励肯定，需要改进则温柔提点。\n\n"
    ).arg(dietQuality, toneHint);

    prompt += buildTodayTemplate();
    return prompt;
}

QString AiReportDialog::buildTodayTemplate() const {
    return QString::fromUtf8(
        "请严格按照以下**章节结构**生成一份完整的当日饮食报告，每期内容必须重新创作，"
        "不要复制固定的句子，所有文字根据数据和风格即兴生成：\n\n"
        "首先，以「## 今日食光 · xxxx」为格式写一个标题（xxxx 替换为贴合今日数据的小短语，如「暖暖的一餐」「清淡小日子」等），"
        "然后写一段 1-2 句话的开篇语，记录日期和今日开销，营造专属感，今天的每一餐都是认真生活的小证据。\n\n"
        "---\n"
        "## 一、今日食光\n"
        "根据菜品数据概括今日的整体饮食风格（如温暖饱腹/清淡简约/重油碳水/随性干饭等），"
        "为今日生成一个独一无二的「食光标签」，给出综合状态评价（优/良/尚可/待微调）。\n\n"
        "---\n"
        "## 二、今日能量\n"
        "列出总热量摄入，对比用户的基础代谢率，说明今日能量处于什么状态。"
        "将菜品自动分类为主食/荤食/蔬菜/水果/小食饮品并列出，分析今日营养配比的亮点和缺失项。\n\n"
        "---\n"
        "## 三、饮食丰富度\n"
        "统计不同食材种类数，评价丰富度，给出食材选择偏向的建议。\n\n"
        "---\n"
        "## 四、今日开销小记\n"
        "列出今日餐饮花费金额，评价在日常合理区间中的位置（适中/偏低/偏高），"
        "语气温暖地看待开销这件事。\n\n"
        "---\n"
        "## 五、温柔叮嘱\n"
        "根据实际饮食数据，温柔指出 2-3 个可以让身体更舒服的小细节。"
        "先肯定正常的食欲和选择，再给出温和的微调方向，无负面打压。\n\n"
        "---\n"
        "## 六、明日期许\n"
        "给出 2-3 条简单可落地的温柔建议，不需要用户突然改变，只是一点点微调。\n\n"
        "---\n"
        "### 今日食光寄语\n"
        "以你温暖的风格，写一句独特的寄语作为收尾。要结合今日的实际数据或饮食特点，"
        "每次都不一样，不要重复固定的句子。\n\n"
        "---\n"
        "风格规则（严格遵守）：\n"
        "1. 先肯定、再提点、最后鼓励，绝对不负面批判\n"
        "2. 将'超标、失衡、很差'转化为：有待微调、可以更舒适、小优化空间\n"
        "3. 所有数据拟人化：食物=陪伴、三餐=生活仪式、记录=自爱\n"
        "4. 使用中文，语气温暖治愈，像朋友在轻声聊天\n"
        "5. 每期内容必须原创生成，不要使用固定的句式或重复的寄语\n"
        "6. 根据实际数据填写，不确定的标注'数据暂缺'\n"
        "7. 如果用户饮食健康均衡，多给予肯定和鼓励；如果不健康，温柔叮嘱但不要批评\n"
    );
}

QString AiReportDialog::buildWeekPrompt() const {
    // 取最近 7 天数据（不含今天，从昨天往前数 7 天）
    QStringList allDates = m_records.keys();
    std::sort(allDates.begin(), allDates.end());
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    // 去掉今天的记录
    allDates.erase(std::remove(allDates.begin(), allDates.end(), today), allDates.end());
    QStringList weekDates = allDates.mid(qMax(0, allDates.size() - 7));

    // 分析饮食质量
    QString toneHint, dietQuality;
    analyzeDietQuality(toneHint, dietQuality);

    QString prompt;
    if (weekDates.isEmpty()) {
        prompt = QString::fromUtf8(
            "请生成一份「一周饮食报告」，先说明最近七天暂无饮食记录，"
            "然后以温柔鼓励的语气建议用户开始记录。\n\n"
        );
        prompt += buildWeekTemplate();
        return prompt;
    }

    QString firstDate = weekDates.first();
    QString lastDate = weekDates.last();
    prompt = QString::fromUtf8(
        "你是一位温暖的饮食陪伴者。请根据以下最近七天的饮食数据生成「一周饮食报告」。\n\n"
        "**统计周期**：%1 — %2\n\n"
    ).arg(firstDate, lastDate);

    double totalCal = 0, totalPrice = 0;
    QStringList allDishes;
    for (const auto &date : weekDates) {
        const auto &r = m_records[date];
        prompt += QString("- %1：【%2】%3 kcal %4 元\n")
                      .arg(date, r.dishes.join("、"))
                      .arg(r.totalCalories, 0, 'f', 0)
                      .arg(r.totalPrice, 0, 'f', 1);
        totalCal += r.totalCalories;
        totalPrice += r.totalPrice;
        allDishes.append(r.dishes);
    }

    int dayCount = weekDates.size();
    prompt += QString::fromUtf8("\n**近七天汇总**\n- 记录天数：%1 天\n- 日均热量：%2 kcal\n- 日均花费：%3 元\n")
                  .arg(dayCount)
                  .arg(dayCount > 0 ? totalCal / dayCount : 0, 0, 'f', 0)
                  .arg(dayCount > 0 ? totalPrice / dayCount : 0, 0, 'f', 1);

    if (m_bmr > 0) {
        prompt += QString::fromUtf8("- 基础代谢率(BMR)：%1 kcal/天\n").arg(m_bmr, 0, 'f', 0);
    }

    prompt += QString::fromUtf8(
        "\n**饮食状态评估**（基于数据分析）：\n"
        "- 整体评价：%1\n"
        "- 语气策略：%2\n"
        "\n请根据以上评估调整语气。\n\n"
    ).arg(dietQuality, toneHint);

    prompt += buildWeekTemplate();
    return prompt;
}

QString AiReportDialog::buildWeekTemplate() const {
    return QString::fromUtf8(
        "请严格按照以下**章节结构**生成一份完整的一周饮食报告，每期内容必须重新创作，"
        "不要复制固定的句子，所有文字根据数据和风格即兴生成：\n\n"
        "首先，以「## 本周食光 · xxxx」为格式写一个标题（xxxx 替换为贴合本周数据的小短语，如「慢慢变好的你」「烟火日常」「自律的一周」等），"
        "然后写一段 1-2 句话的开篇语，包含统计周期、本周总开销和日均开销，"
        "营造「一周三餐藏着生活状态」的仪式感。\n\n"
        "---\n"
        "## 一、饮食人格\n"
        "根据近七天数据，概括用户的饮食风格人格（如安稳饱腹型/清淡自律型/随性松弛型/偶尔放纵型等），"
        "生成 3 个专属食光关键词，给出本周整体评价（优/良/平稳/稳步进步）。\n"
        "用一段话描述这一周的饮食轨迹给人什么感受。\n\n"
        "---\n"
        "## 二、能量轨迹\n"
        "列出本周日均摄入热量，对比用户的基础代谢率，说明能量状态。"
        "统计本周出现频率最高的 TOP3 菜品，描述这些反复出现的食物带来的感受。\n\n"
        "---\n"
        "## 三、成长记录\n"
        "统计本周尝试的不同食物种数，评价丰富度变化趋势。"
        "列出本周做得好的 2 个方面（如三餐规律、营养均衡等），"
        "再给出 2 个可以温柔优化的小方向。\n\n"
        "---\n"
        "## 四、消费记录\n"
        "列出本周日均餐饮消费金额，评价在生活节奏中是否合理。\n\n"
        "---\n"
        "## 五、温柔叮嘱\n"
        "基于一周的稳定数据，温柔指出 2 个身体正在接收的饮食信号。\n"
        "要拟人化、不吓人、治愈式提示。\n\n"
        "---\n"
        "## 六、下周期待\n"
        "给出 2 个即刻可做的微调小习惯，和 2 个长期温柔养成方向。"
        "语气轻松，不要给人压力。\n\n"
        "---\n"
        "### 本周食光寄语\n"
        "以你温暖的风格，写一段独特的寄语作为收尾。要结合本周的实际数据或饮食变化，"
        "每次都不一样，不要复制固定的句子。要有网易云年度报告结尾的治愈感和仪式感。\n\n"
        "---\n"
        "风格规则（严格遵守）：\n"
        "1. 先肯定、再提点、最后鼓励，绝对不负面批判\n"
        "2. 将负面词汇转化为：有待微调、可以更舒适、小优化空间\n"
        "3. 数据拟人化：食物=陪伴、三餐=仪式、记录=自爱\n"
        "4. 贴近网易云/QQ音乐年度报告的治愈风格\n"
        "5. 每期内容必须原创生成，不要使用固定的句式或重复的寄语\n"
        "6. 根据实际数据填写，不确定的标注'数据暂缺'\n"
        "7. 健康均衡则多鼓励肯定，需要改进则温柔叮嘱不批评\n"
    );
}

// ═══════════════════════════════════════════
//  API 调用（流式）
// ═══════════════════════════════════════════

void AiReportDialog::callApi(const QString &prompt) {
    QUrl url("https://open.bigmodel.cn/api/paas/v4/chat/completions");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    req.setTransferTimeout(120000);

    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = QString::fromUtf8(
        "你是一位温暖的饮食陪伴者与营养师。你的回复原则："
        "先肯定、再提点、最后鼓励。使用温柔治愈的中文，将数据拟人化。"
        "不要负面批判用户。如果用户饮食健康请多鼓励肯定，如果需要改进请温柔叮嘱。"
        "严格按照用户指定的模板输出，保持 Markdown 格式。"
    );

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;

    QJsonArray messages;
    messages.append(sysMsg);
    messages.append(userMsg);

    QJsonObject body;
    body["model"] = "glm-4.5-flash";
    body["messages"] = messages;
    body["temperature"] = 0.8;
    body["max_tokens"] = 8192;
    body["stream"] = true;

    m_currentReply = m_networkManager->post(req, QJsonDocument(body).toJson());

    connect(m_currentReply, &QNetworkReply::readyRead, this, [this]() {
        m_sseBuffer.append(m_currentReply->readAll());

        while (true) {
            int idx = m_sseBuffer.indexOf('\n');
            if (idx < 0) break;

            QByteArray line = m_sseBuffer.left(idx);
            m_sseBuffer.remove(0, idx + 1);

            if (line.trimmed().isEmpty()) continue;
            if (!line.startsWith("data: ")) continue;

            QByteArray jsonStr = line.mid(6);
            if (jsonStr.trimmed() == "[DONE]") continue;

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr, &err);
            if (err.error != QJsonParseError::NoError) continue;

            QJsonObject obj = doc.object();
            QJsonArray choices = obj["choices"].toArray();
            if (choices.isEmpty()) continue;

            QString delta = choices[0].toObject()["delta"].toObject()["content"].toString();
            if (!delta.isEmpty()) m_fullText += delta;
        }
    });

    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        // 处理 buffer 中残余数据
        if (!m_sseBuffer.isEmpty()) {
            QString leftover = QString::fromUtf8(m_sseBuffer);
            int idx = leftover.indexOf("\"content\":\"");
            while (idx >= 0) {
                idx += 11;
                int end = leftover.indexOf('"', idx);
                if (end < 0) break;
                QString seg = leftover.mid(idx, end - idx);
                seg.replace("\\n", "\n").replace("\\\"", "\"");
                m_fullText += seg;
                idx = leftover.indexOf("\"content\":\"", end);
            }
        }

        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        m_loadingTimer->stop();

        if (m_fullText.trimmed().isEmpty()) {
            m_reportText->setPlainText(QString::fromUtf8("生成失败，请检查网络连接或 API Key 是否有效。"));
            m_statusLabel->setText(QString::fromUtf8("✗ 生成失败"));
            m_statusLabel->setStyleSheet(
                "color:#C86A5A;font-size:12px;"
                "font-family:'Microsoft YaHei';border:none;background:transparent;");
            setButtonsEnabled(true);
            return;
        }

        // 直接渲染为 Markdown，不显示原始 AI 文本
        m_reportText->setMarkdown(m_fullText);
        m_statusLabel->setText(QString::fromUtf8("✓ 报告生成完成"));
        m_statusLabel->setStyleSheet(
            "color:#5D8A5C;font-size:13px;font-weight:bold;"
            "font-family:'Microsoft YaHei';border:none;background:transparent;");
        setButtonsEnabled(true);

        // 保存到历史
        QString typeLabel = (m_mode == Today)
            ? QString::fromUtf8("当日报告")
            : QString::fromUtf8("本周报告");
        saveReportEntry(QDate::currentDate().toString("yyyy-MM-dd"), typeLabel, m_fullText);
    });
}

void AiReportDialog::saveReport(const QString &content) {
    QString typeLabel = (m_mode == Today)
        ? QString::fromUtf8("当日报告")
        : QString::fromUtf8("本周报告");
    saveReportEntry(QDate::currentDate().toString("yyyy-MM-dd"), typeLabel, content);
}

void AiReportDialog::setButtonsEnabled(bool enabled) {
    m_generateBtn->setEnabled(enabled);
    m_todayBtn->setEnabled(enabled);
    m_weekBtn->setEnabled(enabled);
    m_generateBtn->setText(enabled
        ? QString::fromUtf8("生成报告")
        : QString::fromUtf8("生成中…"));
}

void AiReportDialog::setApiKey(const QString &key) {
    m_apiKey = key;
    saveApiKey(key);
}

// ── 绘制 ──
void AiReportDialog::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 61;

    QPainterPath sp = sketchyRect(card.translated(2.5, 3.5), seed + 100, 2.8);
    p.setBrush(C_SHADOW);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, C_CREAM, 18);
    drawInkBorder(&p, cp, C_INK, 3, 0.7);

    QPushButton *xBtn = findChild<QPushButton*>("closeXBtn");
    if (xBtn) xBtn->move(width() - 38, 28);

    p.end();
}

// ═══════════════════════════════════════════
//  updateModeButtonStyles
// ═══════════════════════════════════════════

void AiReportDialog::updateModeButtonStyles() {
    QString activeStyle =
        "QPushButton {"
        "  border: 2px solid #8A7A6A; border-radius: 12px;"
        "  background-color: #D0DDE8;"
        "  color: #2B2B2B; font-size: 14px; font-weight: bold;"
        "  font-family: 'Microsoft YaHei'; padding: 6px 18px;"
        "}"
        "QPushButton:hover { background-color: #C0D0DC; }";
    QString inactiveStyle =
        "QPushButton {"
        "  border: 2px solid #C8BEB4; border-radius: 12px;"
        "  background-color: #F5F0E8;"
        "  color: #8A7A6A; font-size: 14px;"
        "  font-family: 'Microsoft YaHei'; padding: 6px 18px;"
        "}"
        "QPushButton:hover { background-color: #EDE4D8; }";

    m_todayBtn->setStyleSheet(m_mode == Today ? activeStyle : inactiveStyle);
    m_weekBtn->setStyleSheet(m_mode == Week ? activeStyle : inactiveStyle);
}
