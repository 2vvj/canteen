#include "statisticswindow.h"
#include "chartwidgets.h"
#include "decopainter.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPainter>
#include <QDate>
#include <algorithm>
#include <cmath>

StatisticsWindow::StatisticsWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QString::fromUtf8("数据统计"));
    resize(780, 900);
    setStyleSheet("QDialog { background: #FAF8F5; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22,18,22,18); mainLayout->setSpacing(12);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("数 据 统 计"), this);
    titleLabel->setStyleSheet("font-size:22px;font-weight:bold;color:#8B6F5E;padding:12px 0 4px 0;letter-spacing:10px;font-family:'Microsoft YaHei';");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: #F0EDE8; width: 5px; border-radius: 2px; }"
        "QScrollBar::handle:vertical { background: #D8CEC2; border-radius: 2px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    QWidget *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(18); contentLayout->setContentsMargins(4,8,4,8);

    m_emptyLabel = new QLabel(QString::fromUtf8("暂无数据，请先在历史记录中查看"), this);
    m_emptyLabel->setStyleSheet("color:#C4B8A8;font-size:14px;padding:60px 0;font-family:'Microsoft YaHei';");
    m_emptyLabel->setAlignment(Qt::AlignCenter); m_emptyLabel->hide();

    m_expenseChart = new LineChartWidget(this);
    m_expenseChart->setMinimumHeight(240); m_expenseChart->setYAxisLabel(QString::fromUtf8("金额 (元)"));
    contentLayout->addWidget(m_expenseChart);
    m_expenseAvgLabel = new QLabel(this);
    m_expenseAvgLabel->setStyleSheet("color:#5D4B3A;font-size:12px;font-family:'Microsoft YaHei';padding-left:60px;");
    contentLayout->addWidget(m_expenseAvgLabel);

    m_calorieChart = new LineChartWidget(this);
    m_calorieChart->setMinimumHeight(240); m_calorieChart->setYAxisLabel(QString::fromUtf8("热量 (千卡)"));
    contentLayout->addWidget(m_calorieChart);
    m_calorieAvgLabel = new QLabel(this);
    m_calorieAvgLabel->setStyleSheet("color:#5D4B3A;font-size:12px;font-family:'Microsoft YaHei';padding-left:60px;");
    contentLayout->addWidget(m_calorieAvgLabel);

    m_monthlyBarChart = new BarChartWidget(this);
    m_monthlyBarChart->setMinimumHeight(240);
    contentLayout->addWidget(m_monthlyBarChart);

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
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    int w=width(), h=height();
    painter.setBrush(QColor(250,247,243,200)); painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0,0,w,h), 3.0f, 7));
    DecoPainter::drawPaperTexture(painter, QRect(0,0,w,h));
    float s = qMin(w,h)/28.0f;
    DecoPainter::drawSakura(painter, QPointF(w*0.04f,h*0.02f), s*0.6f);
    DecoPainter::drawXiaolongbao(painter, QPointF(w*0.03f,h*0.93f), s*0.7f);
}

void StatisticsWindow::loadData() {
    if (m_records.isEmpty()) {
        m_expenseChart->hide(); m_calorieChart->hide();
        m_monthlyBarChart->hide(); m_emptyLabel->show();
        m_expenseAvgLabel->hide(); m_calorieAvgLabel->hide();
        return;
    }
    m_emptyLabel->hide();
    m_expenseChart->show(); m_calorieChart->show(); m_monthlyBarChart->show();
    m_expenseAvgLabel->show(); m_calorieAvgLabel->show();

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
    m_expenseChart->setData(expensePts, QString::fromUtf8("每日支出 (近28天)"), QColor(200,160,130));
    m_calorieChart->setData(caloriePts, QString::fromUtf8("每日热量 (近28天)"), QColor(170,160,130));

    int n = dates.size();
    if (n > 0) {
        m_expenseAvgLabel->setText(QString::fromUtf8("过去%1天日均支出：¥%2 / 天")
            .arg(n).arg(expSum/n, 0, 'f', 1));
        m_calorieAvgLabel->setText(QString::fromUtf8("过去%1天日均热量：%2 kcal / 天")
            .arg(n).arg(calSum/n, 0, 'f', 0));
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
    m_monthlyBarChart->setData(monthlyBars, QString::fromUtf8("每月支出"));
}
