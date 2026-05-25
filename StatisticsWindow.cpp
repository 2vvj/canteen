#include "StatisticsWindow.h"
#include "ChartWidgets.h"
#include "DecoPainter.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QSqlQuery>
#include <QDebug>

StatisticsWindow::StatisticsWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle("数据统计");
    resize(680, 780);
    setStyleSheet(QString("QDialog { background-color: %1; }").arg(DecoPainter::paperBase().name()));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22, 18, 22, 18);
    mainLayout->setSpacing(12);

    // 标题 — 深红糖手写体
    QLabel *titleLabel = new QLabel("数 据 统 计", this);
    titleLabel->setStyleSheet(
        "font-size: 22px; font-weight: bold; color: #8B6F5E;"
        "padding: 12px 0 4px 0; letter-spacing: 10px;"
        "font-family: 'Microsoft YaHei';");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 滚动区域
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical {"
        "  background: #F0EDE8; width: 5px; border-radius: 2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #D8CEC2; border-radius: 2px; min-height: 30px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    QWidget *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(20);
    contentLayout->setContentsMargins(4, 8, 4, 8);

    // 空数据提示
    m_emptyLabel = new QLabel("暂无数据，请先在饮食日历中记录", this);
    m_emptyLabel->setStyleSheet("color: #C4B8A8; font-size: 14px; padding: 60px 0;"
                                "font-family: 'Microsoft YaHei';");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->hide();

    // 创建三个图表
    m_expenseChart = new LineChartWidget(this);
    m_expenseChart->setMinimumHeight(210);
    m_expenseChart->setYAxisLabel("金额 (元)");

    m_calorieChart = new LineChartWidget(this);
    m_calorieChart->setMinimumHeight(210);
    m_calorieChart->setYAxisLabel("热量 (千卡)");

    m_monthlyBarChart = new BarChartWidget(this);
    m_monthlyBarChart->setMinimumHeight(210);

    contentLayout->addWidget(m_expenseChart);
    contentLayout->addWidget(m_calorieChart);
    contentLayout->addWidget(m_monthlyBarChart);
    contentLayout->addWidget(m_emptyLabel);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    loadData();
}

void StatisticsWindow::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // ==========================================
    // 色块撞色背景
    // ==========================================
    // 主背景 — 暖米
    painter.setBrush(QColor(250, 247, 243, 200));
    painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0, 0, w, h), 3.0f, 7));

    // 右上角复古蓝装饰色块
    painter.setBrush(QColor(208, 227, 239, 35));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(w*0.7f, 0, w*0.35f, h*0.4f), 4.5f, 6));

    // 左下角芥末绿色块
    painter.setBrush(QColor(242, 233, 212, 40));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(-20, h*0.6f, w*0.4f, h*0.45f), 4.0f, 5));

    // 复古纸张纹理
    DecoPainter::drawPaperTexture(painter, QRect(0, 0, w, h));

    // 毛刺分割线
    DecoPainter::drawScratchyLine(painter, QPointF(w*0.12f, h*0.09f), QPointF(w*0.88f, h*0.09f),
                                  QColor(43, 43, 43, 35), 0.8f, 1.2f);

    // 角落装饰
    float s = qMin(w, h) / 28.0f;
    DecoPainter::drawSakura(painter, QPointF(w * 0.04f, h * 0.02f), s * 0.6f);
    DecoPainter::drawSesame(painter, QPointF(w * 0.93f, h * 0.03f), s * 0.3f, 15);
    DecoPainter::drawXiaolongbao(painter, QPointF(w * 0.03f, h * 0.93f), s * 0.7f);
    DecoPainter::drawTinyCat(painter, QPointF(w * 0.95f, h * 0.92f), s * 0.8f);

    // 不完美圆形
    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.02f), 12,
                                 QColor(200, 106, 90, 28));

    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.06f, h * 0.90f), s * 0.5f,
                                       QColor(250, 235, 215, 20));
}

void StatisticsWindow::loadData() {
    QSqlQuery q;

    // 检查是否有数据
    q.exec("SELECT COUNT(*) FROM DailyRecords");
    q.next();
    if (q.value(0).toInt() == 0) {
        m_expenseChart->hide();
        m_calorieChart->hide();
        m_monthlyBarChart->hide();
        m_emptyLabel->show();
        return;
    }
    m_emptyLabel->hide();
    m_expenseChart->show();
    m_calorieChart->show();
    m_monthlyBarChart->show();

    // 1. 每日支出折线图
    q.exec("SELECT date, expenses FROM DailyRecords ORDER BY date ASC");
    QVector<LineChartWidget::DataPoint> expensePts;
    while (q.next()) {
        QString dateStr = q.value(0).toString();
        expensePts.append({dateStr.mid(5), q.value(1).toDouble()});
    }
    m_expenseChart->setData(expensePts, "每日支出折线图", QColor(200, 160, 130));

    // 2. 每日热量折线图
    q.exec("SELECT date, calories FROM DailyRecords ORDER BY date ASC");
    QVector<LineChartWidget::DataPoint> caloriePts;
    while (q.next()) {
        QString dateStr = q.value(0).toString();
        caloriePts.append({dateStr.mid(5), q.value(1).toDouble()});
    }
    m_calorieChart->setData(caloriePts, "每日热量折线图", QColor(170, 160, 130));

    // 3. 每月支出柱状图
    q.exec("SELECT strftime('%Y-%m', date) AS month, SUM(expenses) "
           "FROM DailyRecords GROUP BY month ORDER BY month ASC");
    QVector<BarChartWidget::BarData> monthlyBars;
    while (q.next()) {
        monthlyBars.append({q.value(0).toString(), q.value(1).toDouble()});
    }
    m_monthlyBarChart->setData(monthlyBars, "每月支出条形图");
}
