#include "statisticswindow.h"
#include "chartwidgets.h"
#include "decopainter.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QDate>
#include <algorithm>
#include <cmath>

// ========== ChartCardWidget ==========
ChartCardWidget::ChartCardWidget(const QString &title, const QColor &accentColor,
                                 QWidget *parent)
    : QWidget(parent), m_title(title), m_accentColor(accentColor)
{
    m_cardColor = QColor("#FAF8F5");
    setMinimumHeight(200);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 38, 16, 14);
    lay->setSpacing(6);
}

void ChartCardWidget::setChartWidget(QWidget *chart) {
    QVBoxLayout *lay = qobject_cast<QVBoxLayout*>(layout());
    if (lay) lay->addWidget(chart);
}

void ChartCardWidget::setSummaryLabel(QLabel *label) {
    QVBoxLayout *lay = qobject_cast<QVBoxLayout*>(layout());
    if (lay) {
        label->setStyleSheet(
            "color:#5D4B3A;font-size:12px;font-family:'Microsoft YaHei';"
            "padding-left:12px;border:none;background:transparent;");
        lay->addWidget(label);
    }
}

void ChartCardWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r = rect().adjusted(2, 2, -2, -2);

    // 卡片底色 — 微纸纹渐变
    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, m_cardColor);
    bg.setColorAt(0.6, QColor("#F7F3EC"));
    bg.setColorAt(1.0, QColor("#F3EEE6"));
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    QPainterPath cardPath = DecoPainter::makeOrganicRect(r, 2.2f, 17);
    p.drawPath(cardPath);

    // 手绘墨水边框
    QColor ink(43, 43, 43, 75);
    DecoPainter::drawSketchyBorder(&p, cardPath, ink, 2, 1.0f);

    // 顶部色条 — accent 色
    QRectF accentR(r.x() + 20, r.y() + 6, r.width() - 40, 4.0f);
    QPainterPath accentPath = DecoPainter::makeWavyRect(accentR, 0.8f);
    p.setBrush(m_accentColor);
    p.setPen(Qt::NoPen);
    p.setOpacity(0.5);
    p.drawPath(accentPath);
    p.setOpacity(1.0);

    // 标题文字 — 手写体
    DecoPainter::setHandwritingFont(p, 13, true);
    p.setPen(DecoPainter::titleBrown());
    p.drawText(QRectF(r.x() + 26, r.y() + 2, r.width() - 52, 28),
               Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // 淡色水彩晕染
    DecoPainter::drawWatercolorSplotch(p, QPointF(r.right() - 25, r.y() + 15), 12,
                                       QColor(250, 235, 215, 18));
    DecoPainter::drawWatercolorSplotch(p, QPointF(r.x() + 30, r.bottom() - 8), 9,
                                       QColor(208, 227, 239, 16));

    p.end();
    QWidget::paintEvent(event);
}

// ========== StatisticsWindow ==========
StatisticsWindow::StatisticsWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QString::fromUtf8("数据统计"));
    resize(800, 920);
    setStyleSheet("QDialog { background: transparent; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    // 标题
    QLabel *titleLabel = new QLabel(QString::fromUtf8("数 据 统 计"), this);
    titleLabel->setStyleSheet(
        "font-size:24px;font-weight:bold;color:#2B2B2B;padding:10px 0 4px 0;"
        "letter-spacing:10px;font-family:'Microsoft YaHei';");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 手绘分隔线占位
    QLabel *divider = new QLabel(this);
    divider->setFixedHeight(6);
    divider->setStyleSheet("background:transparent;border:none;");
    mainLayout->addWidget(divider);

    // 滚动区域
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: #E8E0D8; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #C8BAB0; border-radius: 4px; min-height: 40px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    QWidget *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(4, 8, 4, 8);

    // 空数据提示
    m_emptyLabel = new QLabel(QString::fromUtf8("暂无数据，请先在历史记录中查看"), this);
    m_emptyLabel->setStyleSheet(
        "color:#C4B8A8;font-size:14px;padding:60px 0;font-family:'Microsoft YaHei';");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->hide();

    // 每日支出卡片
    m_expenseCard = new ChartCardWidget(
        QString::fromUtf8("每日支出"), QColor("#C86A5A"), this);
    m_expenseChart = new LineChartWidget(this);
    m_expenseChart->setMinimumHeight(230);
    m_expenseChart->setYAxisLabel(QString::fromUtf8("金额 (元)"));
    m_expenseCard->setChartWidget(m_expenseChart);
    m_expenseAvgLabel = new QLabel(this);
    m_expenseCard->setSummaryLabel(m_expenseAvgLabel);
    contentLayout->addWidget(m_expenseCard);

    // 每日热量卡片
    m_calorieCard = new ChartCardWidget(
        QString::fromUtf8("每日热量"), QColor("#B8A080"), this);
    m_calorieChart = new LineChartWidget(this);
    m_calorieChart->setMinimumHeight(230);
    m_calorieChart->setYAxisLabel(QString::fromUtf8("热量 (千卡)"));
    m_calorieCard->setChartWidget(m_calorieChart);
    m_calorieAvgLabel = new QLabel(this);
    m_calorieCard->setSummaryLabel(m_calorieAvgLabel);
    contentLayout->addWidget(m_calorieCard);

    // 每月支出卡片
    m_monthlyCard = new ChartCardWidget(
        QString::fromUtf8("每月支出"), QColor("#80A0A8"), this);
    m_monthlyBarChart = new BarChartWidget(this);
    m_monthlyBarChart->setMinimumHeight(230);
    m_monthlyCard->setChartWidget(m_monthlyBarChart);
    contentLayout->addWidget(m_monthlyCard);

    contentLayout->addWidget(m_emptyLabel);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);
}

void StatisticsWindow::setRecords(const QMap<QString, DailyRecord> &records) {
    m_records = records;
    loadData();
}

void StatisticsWindow::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();

    // 主背景 — 米黄纸张
    painter.setBrush(QColor(250, 247, 243, 215));
    painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0, 0, w, h), 3.2f, 11));

    // 左上复古蓝撞色
    painter.setBrush(QColor(208, 227, 239, 55));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(-5, h * 0.08f, w * 0.28f, h * 0.15f), 4.5f, 13));

    // 右下芥末绿撞色
    painter.setBrush(QColor(242, 233, 212, 90));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(w * 0.72f, h * 0.86f, w * 0.32f, h * 0.18f), 4.0f, 17));

    // 纸张纹理
    DecoPainter::drawPaperTexture(painter, QRect(0, 0, w, h));

    // 标题下方分隔线
    DecoPainter::drawScratchyLine(painter,
        QPointF(w * 0.22f, 60), QPointF(w * 0.78f, 60),
        QColor(43, 43, 43, 35), 0.7f, 1.3f);

    // 装饰元素
    float s = qMin(w, h) / 29.0f;
    DecoPainter::drawSakura(painter, QPointF(w * 0.04f, h * 0.02f), s * 0.55f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.12f, h * 0.06f), s * 0.28f, 40);
    DecoPainter::drawXiaolongbao(painter, QPointF(w * 0.03f, h * 0.94f), s * 0.65f);
    DecoPainter::drawTinyCat(painter, QPointF(w * 0.96f, h * 0.93f), s * 0.72f);
    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.02f), 14, QColor(200, 106, 90, 28));
    DecoPainter::drawScallion(painter, QPointF(w * 0.93f, h * 0.55f), s * 0.42f, 18);
}

void StatisticsWindow::loadData() {
    if (m_records.isEmpty()) {
        m_expenseCard->hide();
        m_calorieCard->hide();
        m_monthlyCard->hide();
        m_emptyLabel->show();
        return;
    }
    m_emptyLabel->hide();
    m_expenseCard->show();
    m_calorieCard->show();
    m_monthlyCard->show();

    QStringList allDates = m_records.keys();
    std::sort(allDates.begin(), allDates.end());

    // 只取最近28天
    QDate today = QDate::currentDate();
    QDate cutoff = today.addDays(-28);
    QStringList dates;
    for (const auto &date : allDates) {
        QDate d = QDate::fromString(date, "yyyy-MM-dd");
        if (d.isValid() && d >= cutoff) dates.append(date);
    }

    // 每日支出 + 每日热量折线
    QVector<LineChartWidget::DataPoint> expensePts, caloriePts;
    double expSum = 0, calSum = 0;
    for (const auto &date : dates) {
        const auto &r = m_records[date];
        QString label = date.mid(5);
        expensePts.append({label, r.totalPrice});
        caloriePts.append({label, r.totalCalories});
        expSum += r.totalPrice;
        calSum += r.totalCalories;
    }
    m_expenseChart->setData(expensePts, QString(), QColor(200, 130, 110));
    m_calorieChart->setData(caloriePts, QString(), QColor(160, 140, 120));

    int n = dates.size();
    if (n > 0) {
        m_expenseAvgLabel->setText(
            QString::fromUtf8("过去%1天日均支出：¥%2 / 天")
                .arg(n).arg(expSum / n, 0, 'f', 1));
        m_calorieAvgLabel->setText(
            QString::fromUtf8("过去%1天日均热量：%2 kcal / 天")
                .arg(n).arg(calSum / n, 0, 'f', 0));
        m_expenseAvgLabel->show();
        m_calorieAvgLabel->show();
    } else {
        m_expenseAvgLabel->hide();
        m_calorieAvgLabel->hide();
    }

    // 每月支出柱状图
    QMap<QString, double> monthly;
    for (const auto &date : allDates) {
        QString month = date.left(7);
        monthly[month] += m_records[date].totalPrice;
    }
    QVector<BarChartWidget::BarData> monthlyBars;
    for (auto it = monthly.begin(); it != monthly.end(); ++it)
        monthlyBars.append({it.key(), it.value()});
    m_monthlyBarChart->setData(monthlyBars, QString());
}
