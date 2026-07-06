#include "statisticswindow.h"
#include "chartwidgets.h"
#include "decopainter.h"
#include "sketchyui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QDate>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

static const QColor C_CREAM_S  = QColor("#FDFBF7");
static const QColor C_INK_S    = QColor("#2B2B2B");
static const QColor C_SHADOW_S = QColor("#3A3530");

ChartCardWidget::ChartCardWidget(const QString &title, const QColor &accentColor,
                                 QWidget *parent)
    : QWidget(parent), m_title(title), m_accentColor(accentColor)
{
    m_cardColor = QColor("#FAF8F5");
    setMinimumHeight(350);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 44, 16, 22);
    lay->setSpacing(8);
}

void ChartCardWidget::setChartWidget(QWidget *chart) {
    QVBoxLayout *lay = qobject_cast<QVBoxLayout*>(layout());
    if (lay) lay->addWidget(chart);
}

void ChartCardWidget::setSummaryLabel(QLabel *label) {
    QVBoxLayout *lay = qobject_cast<QVBoxLayout*>(layout());
    if (lay) {
        label->setStyleSheet(
            "color:#6B5C4F;font-size:13px;font-family:'Microsoft YaHei';"
            "padding:4px 0 2px 18px;border:none;background:transparent;"
            "font-style:italic;letter-spacing:1px;");
        lay->addWidget(label);
    }
}

void ChartCardWidget::setDateRange(const QString &text) {
    if (!m_rangeLabel) {
        m_rangeLabel = new QLabel(this);
        m_rangeLabel->setAlignment(Qt::AlignCenter);
        m_rangeLabel->setStyleSheet(
            "color:#A09890; font-size:12px; font-family:'Microsoft YaHei'; "
            "border:none; background:transparent;");
        QVBoxLayout *lay = qobject_cast<QVBoxLayout*>(layout());
        if (lay) {
            int chartIdx = -1;
            for (int i = 0; i < lay->count(); ++i) {
                if (lay->itemAt(i)->widget() && lay->itemAt(i)->widget() != m_rangeLabel)
                    { chartIdx = i; break; }
            }
            if (chartIdx >= 0)
                lay->insertWidget(chartIdx, m_rangeLabel);
            else
                lay->addWidget(m_rangeLabel);
        }
    }
    m_rangeLabel->setText(text);
}

void ChartCardWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r = rect().adjusted(2, 2, -2, -2);

    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, m_cardColor);
    bg.setColorAt(0.6, QColor("#F7F3EC"));
    bg.setColorAt(1.0, QColor("#F3EEE6"));
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    QPainterPath cardPath = DecoPainter::makeOrganicRect(r, 2.2f, 17);
    p.drawPath(cardPath);

    QColor ink(43, 43, 43, 75);
    DecoPainter::drawSketchyBorder(&p, cardPath, ink, 2, 1.0f);

    QRectF accentR(r.x() + 20, r.y() + 6, r.width() - 40, 4.0f);
    QPainterPath accentPath = DecoPainter::makeWavyRect(accentR, 0.8f);
    p.setBrush(m_accentColor);
    p.setPen(Qt::NoPen);
    p.setOpacity(0.5);
    p.drawPath(accentPath);
    p.setOpacity(1.0);

    DecoPainter::setHandwritingFont(p, 13, true);
    p.setPen(DecoPainter::titleBrown());
    p.drawText(QRectF(r.x() + 26, r.y() + 2, r.width() - 52, 28),
               Qt::AlignLeft | Qt::AlignVCenter, m_title);

    DecoPainter::drawWatercolorSplotch(p, QPointF(r.right() - 25, r.y() + 15), 12,
                                       QColor(250, 235, 215, 18));
    DecoPainter::drawWatercolorSplotch(p, QPointF(r.x() + 30, r.bottom() - 8), 9,
                                       QColor(208, 227, 239, 16));

    p.end();
    QWidget::paintEvent(event);
}

StatisticsWindow::StatisticsWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QString::fromUtf8("数据统计"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QScreen *screen = QGuiApplication::primaryScreen();
    int screenH = screen ? screen->availableGeometry().height() : 900;
    setFixedSize(960, screenH);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 36, 36, 28);
    mainLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("数 据 统 计"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size:20px;font-weight:bold;color:#2B2B2B;"
        "letter-spacing:8px;font-family:'Microsoft YaHei';"
        "border:none;background:transparent;");
    mainLayout->addWidget(titleLabel);

    m_closeBtn = new SketchyButton(QString::fromUtf8("×"),
        QColor("#E0D7CC"), QColor("#3A3530"), this);
    m_closeBtn->setFixedSize(36, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "font-size:18px;font-weight:bold;color:#2B2B2B;"
        "font-family:'Microsoft YaHei';");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    m_closeBtn->move(width() - 36 - 36, 30);

    mainLayout->addSpacing(4);

    mainLayout->addWidget(new ScratchyDivider(this));

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px; }"
        "QScrollBar::handle:vertical { background: #C8BAB0; border-radius: 5px; min-height: 36px; }"
        "QScrollBar::handle:vertical:hover { background: #B0A090; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");

    QWidget *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(4, 8, 4, 8);

    m_emptyLabel = new QLabel(QString::fromUtf8("暂无数据，请先在历史记录中查看"), this);
    m_emptyLabel->setStyleSheet(
        "color:#B0A090;font-size:15px;padding:60px 0;font-family:'Microsoft YaHei';"
        "background:transparent;border:none;");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->hide();

    m_expenseCard = new ChartCardWidget(
        QString::fromUtf8("每日支出"), QColor("#C86A5A"), this);
    m_expenseChart = new LineChartWidget(this);
    m_expenseChart->setMinimumHeight(290);
    m_expenseChart->setYAxisLabel(QString::fromUtf8("金额 (元)"));
    m_expenseCard->setChartWidget(m_expenseChart);
    contentLayout->addWidget(m_expenseCard);
    connect(m_expenseChart, &LineChartWidget::swiped, this, &StatisticsWindow::onExpenseSwiped);

    m_calorieCard = new ChartCardWidget(
        QString::fromUtf8("每日热量"), QColor("#B0C29A"), this);
    m_calorieChart = new LineChartWidget(this);
    m_calorieChart->setMinimumHeight(290);
    m_calorieChart->setYAxisLabel(QString::fromUtf8("热量 (千卡)"));
    m_calorieCard->setChartWidget(m_calorieChart);
    contentLayout->addWidget(m_calorieCard);
    connect(m_calorieChart, &LineChartWidget::swiped, this, &StatisticsWindow::onCalorieSwiped);

    m_monthlyCard = new ChartCardWidget(
        QString::fromUtf8("每月支出"), QColor("#80A0A8"), this);
    m_monthlyBarChart = new BarChartWidget(this);
    m_monthlyBarChart->setMinimumHeight(290);
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

void StatisticsWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 59;

    QRectF shadow = card.translated(2.5, 3.5);
    QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
    p.setBrush(C_SHADOW_S);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, C_CREAM_S, 18);
    drawInkBorder(&p, cp, C_INK_S, 3, 0.7);
}

void StatisticsWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QWidget *child = childAt(e->pos());
        if (!child || child == this) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
        }
    }
    QDialog::mousePressEvent(e);
}

void StatisticsWindow::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
        if (delta.manhattanLength() > 4)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    QDialog::mouseMoveEvent(e);
}

void StatisticsWindow::mouseReleaseEvent(QMouseEvent *e) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
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

    loadExpenseChart();
    loadCalorieChart();

    QStringList allDates = m_records.keys();
    QMap<QString, double> monthly;
    for (const auto &date : allDates) {
        QString month = date.left(7);
        monthly[month] += m_records[date].totalPrice;
    }
    QVector<BarChartWidget::BarData> monthlyBars;
    double monthlySum = 0;
    for (auto it = monthly.begin(); it != monthly.end(); ++it) {
        monthlyBars.append({it.key(), it.value()});
        monthlySum += it.value();
    }
    double monthlyAvg = monthly.isEmpty() ? 0 : monthlySum / monthly.size();
    m_monthlyBarChart->setData(monthlyBars, QString());
    m_monthlyBarChart->setAverageLine(monthlyAvg, QColor("#80A0A8"));
}

static void buildLineChartData(const QMap<QString, DailyRecord> &records, int offset,
                                QVector<LineChartWidget::DataPoint> &pts,
                                double &sum, int &count, QDate &wStart, QDate &wEnd) {
    QDate today = QDate::currentDate();
    wEnd = today.addDays(offset);
    wStart = wEnd.addDays(-27);
    sum = 0;
    count = 0;
    for (QDate d = wStart; d <= wEnd; d = d.addDays(1)) {
        QString dateStr = d.toString("yyyy-MM-dd");
        const auto &r = records[dateStr];
        QString label = dateStr.mid(5);
        pts.append({label, r.totalPrice});
        if (r.totalPrice > 0) { sum += r.totalPrice; count++; }
    }
}

static void buildCalorieChartData(const QMap<QString, DailyRecord> &records, int offset,
                                   QVector<LineChartWidget::DataPoint> &pts,
                                   double &sum, int &count, QDate &wStart, QDate &wEnd) {
    QDate today = QDate::currentDate();
    wEnd = today.addDays(offset);
    wStart = wEnd.addDays(-27);
    sum = 0;
    count = 0;
    for (QDate d = wStart; d <= wEnd; d = d.addDays(1)) {
        QString dateStr = d.toString("yyyy-MM-dd");
        const auto &r = records[dateStr];
        QString label = dateStr.mid(5);
        pts.append({label, r.totalCalories});
        if (r.totalCalories > 0) { sum += r.totalCalories; count++; }
    }
}

void StatisticsWindow::loadExpenseChart() {
    QVector<LineChartWidget::DataPoint> pts;
    double sum = 0;
    int count = 0;
    QDate wStart, wEnd;
    buildLineChartData(m_records, m_expenseOffset, pts, sum, count, wStart, wEnd);

    double avg = count > 0 ? sum / count : 0;
    if (m_expenseOffset == 0 && !pts.isEmpty() && pts.last().value == 0)
        pts.last().value = avg;

    m_expenseChart->setData(pts, QString(), QColor(160, 140, 120));
    m_expenseChart->setAverageLine(avg, QColor("#C86A5A"));
}

void StatisticsWindow::loadCalorieChart() {
    QVector<LineChartWidget::DataPoint> pts;
    double sum = 0;
    int count = 0;
    QDate wStart, wEnd;
    buildCalorieChartData(m_records, m_calorieOffset, pts, sum, count, wStart, wEnd);

    double avg = count > 0 ? sum / count : 0;
    if (m_calorieOffset == 0 && !pts.isEmpty() && pts.last().value == 0)
        pts.last().value = avg;

    m_calorieChart->setData(pts, QString(), QColor(160, 140, 120));
    m_calorieChart->setAverageLine(avg, QColor("#B0C29A"));
}

void StatisticsWindow::onExpenseSwiped(int direction) {
    m_expenseOffset += direction * 28;
    loadExpenseChart();
}

void StatisticsWindow::onCalorieSwiped(int direction) {
    m_calorieOffset += direction * 28;
    loadCalorieChart();
}
