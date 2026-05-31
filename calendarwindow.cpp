#include "calendarwindow.h"
#include "statisticswindow.h"
#include "aireportdialog.h"
#include "decopainter.h"
#include "sketchyui.h"
#include "userdata.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QTextCharFormat>
#include <QMouseEvent>
#include <cmath>

static const QColor C_CREAM  = QColor("#FDFBF7");
static const QColor C_INK    = QColor("#2B2B2B");
static const QColor C_SHADOW = QColor("#3A3530");

// ========== ScratchyDivider ==========
// 参考开屏界面：单条 quadTo 柔和曲线，双层墨线叠涂
ScratchyDivider::ScratchyDivider(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(14);
    setStyleSheet("background: transparent; border: none;");
}

void ScratchyDivider::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    float x0 = width() * 0.26f;
    float x1 = width() * 0.74f;
    float y  = height() * 0.5f;

    QPainterPath sepPath;
    sepPath.moveTo(x0, y);
    sepPath.quadTo((x0 + x1) / 2.0f, y - 1.2f, x1, y + 0.6f);

    QPen sepPen(QColor("#4A4540"));
    sepPen.setWidthF(1.2);
    sepPen.setCapStyle(Qt::RoundCap);
    p.setPen(sepPen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(sepPath);

    sepPen.setWidthF(0.6);
    sepPen.setColor(QColor("#7A7570"));
    p.setPen(sepPen);
    p.drawPath(sepPath);
}

// ========== CalendarFrameWidget ==========
CalendarFrameWidget::CalendarFrameWidget(QCalendarWidget *calendar, QWidget *parent)
    : QWidget(parent), m_calendar(calendar)
{
    setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->addWidget(m_calendar);
}

void CalendarFrameWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = rect().adjusted(2, 2, -2, -2);

    p.setBrush(C_CREAM);
    p.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeOrganicRect(r, 2.2f, 19);
    p.drawPath(bgPath);

    QColor ink(43, 43, 43, 75);
    DecoPainter::drawSketchyBorder(&p, bgPath, ink, 3, 1.0f);
}

// ========== RecordCardWidget ==========
RecordCardWidget::RecordCardWidget(QWidget *parent)
    : QWidget(parent)
{
    m_cardColor = C_CREAM;
    setMinimumHeight(140);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 14, 22, 12);
    layout->setSpacing(6);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setStyleSheet(
        "font-size:15px;font-weight:bold;border:none;background:transparent;"
        "color:#2B2B2B;font-family:'Microsoft YaHei';");
    layout->addWidget(m_dateLabel);

    m_dishesLabel = new QLabel(this);
    m_dishesLabel->setStyleSheet(
        "font-size:13px;border:none;background:transparent;"
        "color:#5D4B3A;font-family:'Microsoft YaHei';");
    m_dishesLabel->setWordWrap(true);
    layout->addWidget(m_dishesLabel);

    m_caloriesLabel = new QLabel(this);
    m_caloriesLabel->setStyleSheet(
        "font-size:13px;border:none;background:transparent;"
        "color:#5D4B3A;font-family:'Microsoft YaHei';");
    layout->addWidget(m_caloriesLabel);

    m_priceLabel = new QLabel(this);
    m_priceLabel->setStyleSheet(
        "font-size:13px;border:none;background:transparent;"
        "color:#5D4B3A;font-family:'Microsoft YaHei';");
    layout->addWidget(m_priceLabel);
}

void RecordCardWidget::setDateText(const QString &text) {
    m_dateLabel->setText(text);
}

void RecordCardWidget::setDishesText(const QString &text) {
    m_dishesLabel->setText(text);
}

void RecordCardWidget::setCaloriesText(const QString &text) {
    m_caloriesLabel->setText(text);
}

void RecordCardWidget::setPriceText(const QString &text) {
    m_priceLabel->setText(text);
}

void RecordCardWidget::addButton(SketchyButton *btn) {
    QVBoxLayout *lay = qobject_cast<QVBoxLayout*>(layout());
    if (lay) {
        QHBoxLayout *row = new QHBoxLayout;
        row->addStretch();
        row->addWidget(btn);
        lay->addLayout(row);
    }
}

void RecordCardWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r = rect().adjusted(3, 3, -3, -3);

    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, m_cardColor);
    bg.setColorAt(0.5, QColor("#F9F5F0"));
    bg.setColorAt(1.0, QColor("#F5F0EA"));
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeOrganicRect(r, 2.5f, 13);
    p.drawPath(bgPath);

    QColor ink(43, 43, 43, 80);
    DecoPainter::drawSketchyBorder(&p, bgPath, ink, 2, 1.0f);

    float barW = 5.0f;
    QRectF barR(r.x() + 10, r.y() + r.height() * 0.32f, barW, r.height() * 0.55f);
    QPainterPath barPath = DecoPainter::makeWavyRect(barR, 1.0f);
    p.setBrush(QColor("#D0DDE8"));
    p.setPen(Qt::NoPen);
    p.setOpacity(0.7);
    p.drawPath(barPath);
    p.setOpacity(1.0);
}

// ========== CalendarWindow ==========
CalendarWindow::CalendarWindow(QWidget *parent)
    : QDialog(parent), m_currentDate(QDate::currentDate())
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(480, 570);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 36, 36, 28);
    mainLayout->setSpacing(10);

    // ── 标题 — 占满宽度，文字居中 ──
    QLabel *titleLabel = new QLabel(QString::fromUtf8("饮 食 历 史"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size:20px;font-weight:bold;color:#2B2B2B;"
        "letter-spacing:8px;font-family:'Microsoft YaHei';"
        "border:none;background:transparent;");
    mainLayout->addWidget(titleLabel);

    // ── 关闭按钮 — SketchyButton，匹配手绘风格 ──
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

    // ── 手绘分隔线 ──
    mainLayout->addWidget(new ScratchyDivider(this));

    // ── 日历 — 有机手绘框架包裹 ──
    m_calendar = new QCalendarWidget();
    m_calendar->setGridVisible(false);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setSelectedDate(m_currentDate);
    m_calendar->setMaximumHeight(280);
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    m_calendar->setStyleSheet(
        "QCalendarWidget {"
        "  background-color: #FDFBF7; border: none;"
        "}"
        // 月份弹出菜单
        "QCalendarWidget QMenu {"
        "  background-color: #FDFBF7;"
        "  border: 1.5px solid #C8BEB4;"
        "  padding: 4px;"
        "}"
        "QCalendarWidget QMenu::item {"
        "  padding: 4px 20px;"
        "  color: #2B2B2B;"
        "  font-family: 'Microsoft YaHei';"
        "}"
        "QCalendarWidget QMenu::item:selected {"
        "  background-color: #E8D8C8;"
        "}"
        // 导航栏按钮
        "QCalendarWidget QWidget#qt_calendar_navigationbar > QToolButton {"
        "  color: #2B2B2B; font-size: 14px; font-weight: bold;"
        "  padding: 5px 14px;"
        "  border: 1.5px solid #C8BEB4; border-radius: 8px;"
        "  background-color: #F5F0E8;"
        "  font-family: 'Microsoft YaHei';"
        "  margin: 2px;"
        "}"
        "QCalendarWidget QWidget#qt_calendar_navigationbar > QToolButton:hover {"
        "  background-color: #EDE4D8; border-color: #B8A898;"
        "}"
        "QCalendarWidget QWidget#qt_calendar_navigationbar > QToolButton:pressed {"
        "  background-color: #E0D4C4;"
        "}"
        // 年份输入框 — 隐藏上下按钮，纯文本输入
        "QCalendarWidget QSpinBox {"
        "  background-color: #F5F0E8;"
        "  border: 1.5px solid #C8BEB4; border-radius: 8px;"
        "  padding: 3px 8px;"
        "  color: #2B2B2B; font-size: 13px;"
        "  font-family: 'Microsoft YaHei';"
        "}"
        "QCalendarWidget QSpinBox::up-button,"
        "QCalendarWidget QSpinBox::down-button {"
        "  width: 0px; height: 0px; border: none; background: transparent;"
        "}"
        "QCalendarWidget QSpinBox::up-arrow,"
        "QCalendarWidget QSpinBox::down-arrow {"
        "  width: 0px; height: 0px;"
        "}"
        // 日期区域
        "QCalendarWidget QAbstractItemView:enabled {"
        "  font-family: 'Microsoft YaHei'; font-size: 12px;"
        "  color: #5D4B3A;"
        "  selection-background-color: #E8D8C8;"
        "  selection-color: #2B2B2B;"
        "  background-color: #FDFBF7;"
        "  alternate-background-color: #F9F5EF;"
        "  outline: none;"
        "}"
        "QCalendarWidget QAbstractItemView:disabled {"
        "  color: #D0C8C0;"
        "}"
        "QCalendarWidget QWidget#qt_calendar_navigationbar {"
        "  background-color: #FDFBF7; border: none;"
        "}");

    // 周末文字颜色
    QTextCharFormat weekendFmt;
    weekendFmt.setForeground(QColor("#B08070"));
    m_calendar->setWeekdayTextFormat(Qt::Saturday, weekendFmt);
    m_calendar->setWeekdayTextFormat(Qt::Sunday, weekendFmt);

    // 今天高亮 — 柔和复古蓝底色
    QTextCharFormat todayFmt;
    todayFmt.setBackground(QColor("#D0DDE8"));
    todayFmt.setForeground(QColor("#2B2B2B"));
    QFont todayFont;
    todayFont.setBold(true);
    todayFmt.setFont(todayFont);
    m_calendar->setDateTextFormat(QDate::currentDate(), todayFmt);

    connect(m_calendar, &QCalendarWidget::clicked, this, &CalendarWindow::onDateSelected);

    m_calendarFrame = new CalendarFrameWidget(m_calendar, this);
    mainLayout->addWidget(m_calendarFrame);

    // ── 记录卡片 ──
    m_recordCard = new RecordCardWidget(this);
    mainLayout->addWidget(m_recordCard);

    mainLayout->addStretch();

    // ── 底部按钮行：饮食报告 + 历史报告 + 数据统计 ──
    QHBoxLayout *btnRow = new QHBoxLayout;
    SketchyButton *aiBtn = new SketchyButton(
        QString::fromUtf8("饮食报告"),
        QColor("#EAD7D2"),
        C_SHADOW,
        this);
    aiBtn->setMinimumSize(120, 42);
    aiBtn->setStyleSheet(
        "font-size:14px;font-weight:bold;color:#2B2B2B;"
        "font-family:'Microsoft YaHei';");
    connect(aiBtn, &QPushButton::clicked, this, &CalendarWindow::onReportClicked);
    btnRow->addWidget(aiBtn);

    btnRow->addStretch();

    SketchyButton *statsBtn = new SketchyButton(
        QString::fromUtf8("数据统计"),
        QColor("#D0DDE8"),
        C_SHADOW,
        this);
    statsBtn->setMinimumSize(130, 42);
    statsBtn->setStyleSheet(
        "font-size:15px;font-weight:bold;color:#2B2B2B;"
        "font-family:'Microsoft YaHei';");
    connect(statsBtn, &QPushButton::clicked, this, &CalendarWindow::onStatisticsClicked);
    btnRow->addWidget(statsBtn);
    mainLayout->addLayout(btnRow);

    showRecordForDate(m_currentDate);
}

void CalendarWindow::setRecords(const QMap<QString, DailyRecord> &records) {
    m_records = records;
    showRecordForDate(m_currentDate);
}

// ── 参照 SettingsDialog 的卡片式绘制 ──
void CalendarWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 61;

    // 阴影层
    QRectF shadow = card.translated(2.5, 3.5);
    QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
    p.setBrush(C_SHADOW);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    // 卡片层 — 奶油纸色 + 墨水晕染 + 黑边
    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, C_CREAM, 18);
    drawInkBorder(&p, cp, C_INK, 3, 0.7);
}

void CalendarWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QWidget *child = childAt(e->pos());
        if (!child || child == this) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
        }
    }
    QDialog::mousePressEvent(e);
}

void CalendarWindow::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
        if (delta.manhattanLength() > 4)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    QDialog::mouseMoveEvent(e);
}

void CalendarWindow::mouseReleaseEvent(QMouseEvent *e) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}

void CalendarWindow::onDateSelected(const QDate &date) {
    m_currentDate = date;
    showRecordForDate(date);
}

void CalendarWindow::showRecordForDate(const QDate &date) {
    m_recordCard->setDateText(date.toString("yyyy-MM-dd"));

    QString key = date.toString("yyyy-MM-dd");
    if (m_records.contains(key)) {
        const auto &r = m_records[key];
        m_recordCard->setDishesText(
            QString::fromUtf8("菜品：%1").arg(r.dishes.join(QString::fromUtf8("、"))));
        m_recordCard->setCaloriesText(
            QString::fromUtf8("热量：%1 kcal").arg(r.totalCalories, 0, 'f', 0));
        m_recordCard->setPriceText(
            QString::fromUtf8("价格：%1 元").arg(r.totalPrice, 0, 'f', 1));
    } else {
        m_recordCard->setDishesText(QString::fromUtf8("菜品：无记录"));
        m_recordCard->setCaloriesText(QString::fromUtf8("热量：— kcal"));
        m_recordCard->setPriceText(QString::fromUtf8("价格：— 元"));
    }
}

void CalendarWindow::onStatisticsClicked() {
    StatisticsWindow *statsWin = new StatisticsWindow(this);
    statsWin->setAttribute(Qt::WA_DeleteOnClose);
    statsWin->setRecords(m_records);
    statsWin->exec();
}

void CalendarWindow::onReportClicked() {
    // 读取用户数据获取 BMR
    UserData ud;
    double bmr = 0;
    if (ud.load("user.json")) {
        const auto &s = ud.settings;
        if (s.gender == QString::fromUtf8("男") || s.gender == "male") {
            bmr = 10 * s.weight + 6.25 * s.height - 5 * s.age + 5;
        } else {
            bmr = 10 * s.weight + 6.25 * s.height - 5 * s.age - 161;
        }
    }

    AiReportDialog *dlg = new AiReportDialog(m_records, bmr, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}