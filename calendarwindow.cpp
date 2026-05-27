#include "calendarwindow.h"
#include "statisticswindow.h"
#include "decopainter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <cmath>

CalendarWindow::CalendarWindow(QWidget *parent) : QDialog(parent), m_currentDate(QDate::currentDate())
{
    setWindowTitle(QString::fromUtf8("历史记录"));
    resize(500, 620);
    setStyleSheet("QDialog { background: transparent; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(22,16,22,16);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("饮 食 历 史"), this);
    titleLabel->setStyleSheet(
        "font-size:22px;font-weight:bold;color:#2B2B2B;padding:8px 0 2px 0;"
        "letter-spacing:8px;font-family:'Microsoft YaHei';");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    m_calendar = new QCalendarWidget(this);
    m_calendar->setGridVisible(false);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setSelectedDate(m_currentDate);
    m_calendar->setMaximumHeight(280);
    m_calendar->setStyleSheet(
        "QCalendarWidget { background-color:#FAF8F5; border:1.5px solid #E0D8D0; border-radius:12px; padding:4px; }"
        "QCalendarWidget QToolButton { color:#2B2B2B; font-size:13px; font-weight:bold; padding:4px 12px;"
        "  border:1.5px solid #D5C8B8; border-radius:8px; background-color:#F5F0E8; font-family:'Microsoft YaHei'; }"
        "QCalendarWidget QToolButton:hover { background-color:#EDE4D8; }"
        "QCalendarWidget QAbstractItemView:enabled { font-family:'Microsoft YaHei'; font-size:12px;"
        "  color:#5D4B3A; selection-background-color:#E0D0C0; selection-color:#2B2B2B; background-color:#FAF8F5; }");
    connect(m_calendar, &QCalendarWidget::clicked, this, &CalendarWindow::onDateSelected);
    mainLayout->addWidget(m_calendar);

    // 当天记录
    QFrame *recordFrame = new QFrame(this);
    recordFrame->setStyleSheet("QFrame { background-color:#FAF8F5; border:1.5px solid #E0D8D0; border-radius:10px; }");
    QVBoxLayout *frameLayout = new QVBoxLayout(recordFrame);
    frameLayout->setContentsMargins(16,12,16,12); frameLayout->setSpacing(6);

    m_dateLabel = new QLabel(m_currentDate.toString("yyyy-MM-dd"), this);
    m_dateLabel->setStyleSheet("font-size:14px;font-weight:bold;border:none;background:transparent;color:#2B2B2B;font-family:'Microsoft YaHei';");
    frameLayout->addWidget(m_dateLabel);

    m_dishesLabel = new QLabel(QString::fromUtf8("菜品：无记录"), this);
    m_dishesLabel->setStyleSheet("font-size:12px;border:none;background:transparent;color:#5D4B3A;font-family:'Microsoft YaHei';");
    m_dishesLabel->setWordWrap(true);
    frameLayout->addWidget(m_dishesLabel);

    m_caloriesLabel = new QLabel(QString::fromUtf8("热量：— kcal"), this);
    m_caloriesLabel->setStyleSheet("font-size:12px;border:none;background:transparent;color:#5D4B3A;font-family:'Microsoft YaHei';");
    frameLayout->addWidget(m_caloriesLabel);

    m_priceLabel = new QLabel(QString::fromUtf8("价格：— 元"), this);
    m_priceLabel->setStyleSheet("font-size:12px;border:none;background:transparent;color:#5D4B3A;font-family:'Microsoft YaHei';");
    frameLayout->addWidget(m_priceLabel);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton *statsBtn = new QPushButton(QString::fromUtf8("数据统计"));
    statsBtn->setMinimumHeight(40);
    statsBtn->setStyleSheet("QPushButton{background:#C86A5A;color:white;font-size:14px;font-weight:bold;border-radius:8px;padding:6px 20px;}"
                            "QPushButton:hover{background:#B05848;}");
    connect(statsBtn, &QPushButton::clicked, this, &CalendarWindow::onStatisticsClicked);
    btnRow->addWidget(statsBtn);
    frameLayout->addLayout(btnRow);
    mainLayout->addWidget(recordFrame);

    showRecordForDate(m_currentDate);
}

void CalendarWindow::setRecords(const QMap<QString, DailyRecord> &records) {
    m_records = records;
    showRecordForDate(m_currentDate);
}

void CalendarWindow::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    int w=width(), h=height();
    painter.setBrush(QColor(242,233,212,200)); painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0,0,w,h), 3.5f, 7));
    painter.setBrush(QColor(208,227,239,50));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(w*0.55f,h*0.65f,w*0.5f,h*0.4f), 4.0f, 6));
    DecoPainter::drawPaperTexture(painter, QRect(0,0,w,h));
    float s = qMin(w,h)/26.0f;
    DecoPainter::drawSakura(painter, QPointF(w*0.04f,h*0.03f), s*0.6f);
    DecoPainter::drawXiaolongbao(painter, QPointF(w*0.04f,h*0.94f), s*0.7f);
    DecoPainter::drawTinyCat(painter, QPointF(w*0.95f,h*0.93f), s*0.8f);
}

void CalendarWindow::onDateSelected(const QDate &date) {
    m_currentDate = date;
    m_dateLabel->setText(date.toString("yyyy-MM-dd"));
    showRecordForDate(date);
}

void CalendarWindow::showRecordForDate(const QDate &date) {
    QString key = date.toString("yyyy-MM-dd");
    if (m_records.contains(key)) {
        const auto &r = m_records[key];
        m_dishesLabel->setText(QString::fromUtf8("菜品：%1").arg(r.dishes.join(", ")));
        m_caloriesLabel->setText(QString::fromUtf8("热量：%1 kcal").arg(r.totalCalories,0,'f',0));
        m_priceLabel->setText(QString::fromUtf8("价格：¥%1").arg(r.totalPrice,0,'f',1));
    } else {
        m_dishesLabel->setText(QString::fromUtf8("菜品：无记录"));
        m_caloriesLabel->setText(QString::fromUtf8("热量：— kcal"));
        m_priceLabel->setText(QString::fromUtf8("价格：— 元"));
    }
}

void CalendarWindow::onStatisticsClicked() {
    StatisticsWindow *statsWin = new StatisticsWindow(this);
    statsWin->setAttribute(Qt::WA_DeleteOnClose);
    statsWin->setRecords(m_records);
    statsWin->exec();
}
