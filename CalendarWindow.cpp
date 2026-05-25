#include "CalendarWindow.h"
#include "StatisticsWindow.h"
#include "DecoPainter.h"
#include "SketchyButton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>

CalendarWindow::CalendarWindow(QWidget *parent)
    : QDialog(parent), m_currentDate(QDate::currentDate())
{
    setWindowTitle("饮食日历");
    resize(520, 640);
    setStyleSheet("QDialog { background: transparent; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(22, 16, 22, 16);

    // 标题
    QLabel *titleLabel = new QLabel("饮 食 日 历", this);
    titleLabel->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: #2B2B2B;"
        "padding: 10px 0 2px 0; letter-spacing: 10px;"
        "font-family: 'Microsoft YaHei';");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QLabel *subTitle = new QLabel("点击日期记录你的饮食", this);
    subTitle->setStyleSheet(
        "font-size: 13px; color: #8B8B8B; padding-bottom: 4px;"
        "font-family: 'Microsoft YaHei';");
    subTitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subTitle);

    // --- 日历组件 ---
    m_calendar = new QCalendarWidget(this);
    m_calendar->setGridVisible(false);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setSelectedDate(m_currentDate);

    QTextCharFormat todayFmt;
    todayFmt.setBackground(QColor("#E8D8C8"));
    todayFmt.setForeground(QColor("#2B2B2B"));
    m_calendar->setDateTextFormat(QDate::currentDate(), todayFmt);

    // 周六周日黑色文字（覆盖默认红色）
    QTextCharFormat weekendFmt;
    weekendFmt.setForeground(QColor("#5D4B3A"));
    QDate d(2024, 1, 1);
    QDate end(2030, 12, 31);
    while (d <= end) {
        int dow = d.dayOfWeek();
        if (dow == 6 || dow == 7) {
            m_calendar->setDateTextFormat(d, weekendFmt);
        }
        d = d.addDays(1);
    }

    m_calendar->setStyleSheet(
        "QCalendarWidget {"
        "  background-color: #FAF8F5;"
        "  border: 1.5px solid #E0D8D0;"
        "  border-radius: 12px;"
        "  padding: 4px;"
        "}"
        "QCalendarWidget QToolButton {"
        "  color: #2B2B2B; font-size: 14px; font-weight: bold;"
        "  padding: 6px 14px; border: 1.5px solid #D5C8B8;"
        "  border-radius: 8px; background-color: #F5F0E8;"
        "  font-family: 'Microsoft YaHei';"
        "}"
        "QCalendarWidget QToolButton:hover {"
        "  background-color: #EDE4D8;"
        "}"
        "QCalendarWidget QToolButton::menu-indicator { image: none; }"
        "QCalendarWidget QSpinBox {"
        "  color: #2B2B2B; font-size: 12px; border: 1.5px solid #D5C8B8;"
        "  border-radius: 6px; background-color: #FAF8F5; padding: 2px 8px;"
        "}"
        "QCalendarWidget QAbstractItemView:enabled {"
        "  font-family: 'Microsoft YaHei'; font-size: 13px; color: #5D4B3A;"
        "  selection-background-color: #E0D0C0;"
        "  selection-color: #2B2B2B;"
        "  background-color: #FAF8F5; outline: none;"
        "}"
        "QCalendarWidget QAbstractItemView:disabled { color: #C8BAB0; }"
        "QCalendarWidget QHeaderView::section {"
        "  color: #5D4B3A; font-size: 12px; font-family: 'Microsoft YaHei';"
        "}"
    );
    connect(m_calendar, &QCalendarWidget::clicked,
            this, &CalendarWindow::onDateSelected);
    mainLayout->addWidget(m_calendar);

    // --- 记录表单卡片 ---
    QFrame *recordFrame = new QFrame(this);
    recordFrame->setObjectName("recordFrame");
    recordFrame->setStyleSheet(
        "QFrame#recordFrame {"
        "  background-color: #FAF8F5;"
        "  border: none;"
        "  border-radius: 12px;"
        "}");

    QVBoxLayout *frameLayout = new QVBoxLayout(recordFrame);
    frameLayout->setContentsMargins(18, 14, 18, 14);
    frameLayout->setSpacing(10);

    m_dateLabel = new QLabel(m_currentDate.toString("yyyy-MM-dd"), this);
    m_dateLabel->setStyleSheet(
        "font-size: 15px; font-weight: bold; border: none;"
        "background: transparent; color: #2B2B2B;"
        "font-family: 'Microsoft YaHei';");
    frameLayout->addWidget(m_dateLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QString inputStyle =
        "QLineEdit, QDoubleSpinBox {"
        "  padding: 8px 12px; border: 1.5px solid #D5C8B8; border-radius: 10px;"
        "  background-color: #F7F5F0; color: #2B2B2B; font-size: 13px;"
        "  font-family: 'Microsoft YaHei';"
        "}"
        "QLineEdit:focus, QDoubleSpinBox:focus {"
        "  border-color: #C0B0A0; background-color: #FFFFFF;"
        "}";

    QString labelStyle =
        "font-size: 13px; color: #5D4B3A; border: none; background: transparent;"
        "font-family: 'Microsoft YaHei';";

    QString readOnlySpinStyle =
        "QDoubleSpinBox {"
        "  padding: 8px 12px; border: 1.5px solid #E8E0D8; border-radius: 10px;"
        "  background-color: #F0EDE6; color: #5D4B3A; font-size: 13px;"
        "  font-family: 'Microsoft YaHei';"
        "}";

    m_dishesEdit = new QLineEdit(this);
    m_dishesEdit->setPlaceholderText("例：牛肉拉面、麻辣香锅");
    m_dishesEdit->setStyleSheet(inputStyle);
    QLabel *dishesLabel = new QLabel("菜品:", this);
    dishesLabel->setStyleSheet(labelStyle);
    formLayout->addRow(dishesLabel, m_dishesEdit);

    m_caloriesSpin = new QDoubleSpinBox(this);
    m_caloriesSpin->setRange(0, 99999);
    m_caloriesSpin->setSuffix(" kcal");
    m_caloriesSpin->setReadOnly(true);
    m_caloriesSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_caloriesSpin->setStyleSheet(readOnlySpinStyle);
    QLabel *calLabel = new QLabel("热量:", this);
    calLabel->setStyleSheet(labelStyle);
    formLayout->addRow(calLabel, m_caloriesSpin);

    m_expenseSpin = new QDoubleSpinBox(this);
    m_expenseSpin->setRange(0, 99999);
    m_expenseSpin->setPrefix("¥ ");
    m_expenseSpin->setDecimals(1);
    m_expenseSpin->setReadOnly(true);
    m_expenseSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_expenseSpin->setStyleSheet(readOnlySpinStyle);
    QLabel *expLabel = new QLabel("支出:", this);
    expLabel->setStyleSheet(labelStyle);
    formLayout->addRow(expLabel, m_expenseSpin);

    frameLayout->addLayout(formLayout);

    // 按钮行
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    m_saveBtn = new SketchyButton("保存记录", this, SketchyButton::OffsetShadow);
    connect(m_saveBtn, &QPushButton::clicked, this, &CalendarWindow::onSaveClicked);
    btnLayout->addWidget(m_saveBtn);

    btnLayout->addStretch();

    m_statsBtn = new SketchyButton("查看数据统计", this, SketchyButton::SketchyBorder);
    m_statsBtn->setMinimumWidth(142);
    connect(m_statsBtn, &QPushButton::clicked,
            this, &CalendarWindow::onStatisticsClicked);
    btnLayout->addWidget(m_statsBtn);

    frameLayout->addLayout(btnLayout);
    mainLayout->addWidget(recordFrame);

    loadRecordForDate(m_currentDate);
}

void CalendarWindow::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // 色块撞色：芥末绿底 + 复古蓝/红褐点缀
    painter.setBrush(QColor(242, 233, 212, 200));
    painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0, 0, w, h), 3.5f, 7));

    painter.setBrush(QColor(208, 227, 239, 50));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(w*0.6f, h*0.7f, w*0.45f, h*0.35f), 4.0f, 6));

    painter.setBrush(QColor(200, 106, 90, 15));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(-20, -20, w*0.4f, h*0.35f), 5.0f, 6));

    DecoPainter::drawPaperTexture(painter, QRect(0, 0, w, h));

    DecoPainter::drawScratchyLine(painter, QPointF(w*0.15f, h*0.11f), QPointF(w*0.85f, h*0.11f),
                                  QColor(43, 43, 43, 40), 0.8f, 1.2f);

    float s = qMin(w, h) / 26.0f;
    DecoPainter::drawSakura(painter, QPointF(w * 0.04f, h * 0.03f), s * 0.6f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.10f, h * 0.08f), s * 0.35f, 30);
    DecoPainter::drawSesame(painter, QPointF(w * 0.91f, h * 0.03f), s * 0.3f, 15);
    DecoPainter::drawXiaolongbao(painter, QPointF(w * 0.04f, h * 0.94f), s * 0.7f);
    DecoPainter::drawTinyCat(painter, QPointF(w * 0.95f, h * 0.93f), s * 0.8f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.87f, h * 0.95f), s * 0.3f, -30);
    DecoPainter::drawScallion(painter, QPointF(w * 0.93f, h * 0.40f), s * 0.5f, 20);

    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.02f), 14,
                                 QColor(200, 106, 90, 30));
    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.97f), 10,
                                 QColor(208, 227, 239, 45));

    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.05f, h * 0.88f), s * 0.5f,
                                       QColor(250, 235, 215, 20));
}

void CalendarWindow::onDateSelected(const QDate &date) {
    m_currentDate = date;
    m_dateLabel->setText(date.toString("yyyy-MM-dd"));
    loadRecordForDate(date);
}

void CalendarWindow::loadRecordForDate(const QDate &date) {
    QSqlQuery query;
    query.prepare("SELECT dishes, calories, expenses FROM DailyRecords WHERE date = ?");
    query.addBindValue(date.toString("yyyy-MM-dd"));
    if (query.exec() && query.next()) {
        m_dishesEdit->setText(query.value(0).toString());
        m_caloriesSpin->setValue(query.value(1).toDouble());
        m_expenseSpin->setValue(query.value(2).toDouble());
    } else {
        clearForm();
    }
}

void CalendarWindow::clearForm() {
    m_dishesEdit->clear();
    m_caloriesSpin->setValue(0);
    m_expenseSpin->setValue(0);
}

void CalendarWindow::onSaveClicked() {
    QString dateStr = m_currentDate.toString("yyyy-MM-dd");
    QString dishes = m_dishesEdit->text().trimmed();

    // 根据菜品名称自动统计热量和支出
    double autoCalories = 0;
    double autoExpense  = 0;

    if (!dishes.isEmpty()) {
        // 按常见分隔符拆分菜名
        QStringList names;
        if (dishes.contains("、")) {
            names = dishes.split("、");
        } else if (dishes.contains(",")) {
            names = dishes.split(",");
        } else if (dishes.contains("，")) {
            names = dishes.split("，");
        } else {
            names << dishes;
        }

        for (const QString &n : names) {
            QString trimmed = n.trimmed();
            if (trimmed.isEmpty()) continue;
            QSqlQuery q;
            q.prepare("SELECT calories, price FROM Dishes WHERE name LIKE ? LIMIT 1");
            q.addBindValue("%" + trimmed + "%");
            if (q.exec() && q.next()) {
                autoCalories += q.value(0).toDouble();
                autoExpense  += q.value(1).toDouble();
            }
        }
    }

    m_caloriesSpin->setValue(autoCalories);
    m_expenseSpin->setValue(autoExpense);

    QSqlQuery query;
    query.prepare(
        "INSERT INTO DailyRecords (date, dishes, calories, expenses) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT(date) DO UPDATE SET "
        "  dishes=excluded.dishes, calories=excluded.calories, expenses=excluded.expenses");
    query.addBindValue(dateStr);
    query.addBindValue(dishes);
    query.addBindValue(autoCalories);
    query.addBindValue(autoExpense);

    if (query.exec()) {
        qDebug() << "日历记录保存成功:" << dateStr
                 << "热量:" << autoCalories << "支出:" << autoExpense;
    } else {
        QMessageBox::warning(this, "保存失败",
                             "保存记录时出错:\n" + query.lastError().text());
    }
}

void CalendarWindow::onStatisticsClicked() {
    StatisticsWindow *statsWin = new StatisticsWindow(this);
    statsWin->setAttribute(Qt::WA_DeleteOnClose);
    statsWin->exec();
}
