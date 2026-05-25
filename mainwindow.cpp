#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "RadarChartWidget.h"
#include "HistoryWindow.h"
#include "CalendarWindow.h"
#include "DecoPainter.h"
#include "SketchyButton.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
// 归一化常量
static constexpr float kMaxPrice    = 50.0f;   // 价格上限（元）
static constexpr float kMaxCalories = 1200.0f; // 热量上限（kcal）

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentDishId(-1)
    , m_userLocation("dorm")
{
    ui->setupUi(this);

    setStyleSheet(
        "QMainWindow { background-color: #FDFBF7; }"
        "QLabel {"
        "  font-family: 'Microsoft YaHei';"
        "  color: #2B2B2B;"
        "}");

    ui->timeLabel->setStyleSheet(
        "font-size: 38px; font-weight: bold; color: #2B2B2B;"
        "background: transparent; padding: 6px 0; letter-spacing: 2px;"
        "font-family: 'Microsoft YaHei';");

    // 雷达图
    m_radarChart = new RadarChartWidget(this);
    m_radarChart->setMinimumSize(350, 350);
    m_radarChart->setStyleSheet("background: transparent;");
    ui->verticalLayout->insertWidget(0, m_radarChart);

    // ==========================================
    // 替换 .ui 按钮为 SketchyButton
    // ==========================================
    auto *layout = ui->verticalLayout;

    layout->removeWidget(ui->openHistoryButton);
    layout->removeWidget(ui->openCalendarButton);
    delete ui->openHistoryButton;  ui->openHistoryButton = nullptr;
    delete ui->openCalendarButton; ui->openCalendarButton = nullptr;

    SketchyButton *historyBtn = new SketchyButton("干饭档案", this, SketchyButton::OffsetShadow);
    SketchyButton *calendarBtn = new SketchyButton("饮食日历", this, SketchyButton::OffsetShadow);

    connect(historyBtn, &QPushButton::clicked, this, &MainWindow::on_openHistoryButton_clicked);
    connect(calendarBtn, &QPushButton::clicked, this, &MainWindow::on_openCalendarButton_clicked);

    layout->addWidget(historyBtn);
    layout->addWidget(calendarBtn);

    // 时钟
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    m_clockTimer->start(1000);
    updateClock();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// 单菜品雷达更新 — 使用实际数据归一化
// ============================================================
void MainWindow::updateRadarByDishId(int dishId, const QString& userLocation)
{
    m_currentDishId = dishId;
    m_userLocation  = userLocation;

    QSqlQuery query;
    query.prepare("SELECT d.name, d.taste, d.price, d.experience, d.calories, "
                  "r.dist_dorm, r.dist_library, r.dist_building "
                  "FROM Dishes d "
                  "JOIN Restaurants r ON d.restaurant_id = r.id "
                  "WHERE d.id = ?");
    query.addBindValue(dishId);

    if (!query.exec() || !query.next()) {
        qDebug() << "菜品数据查询失败:" << query.lastError().text();
        return;
    }

    QString dishName   = query.value(0).toString();
    float tasteRaw     = query.value(1).toFloat();   // 0-100
    float priceYuan    = query.value(2).toFloat();   // 元
    float experience   = query.value(3).toFloat();   // 0-100
    float caloriesKcal = query.value(4).toFloat();   // kcal

    int rawDist = 0;
    if (userLocation == "dorm")
        rawDist = query.value(5).toInt();
    else if (userLocation == "library")
        rawDist = query.value(6).toInt();
    else
        rawDist = query.value(7).toInt();

    // 归一化：低价=高分，低热量=高分，近距离=高分
    float priceScore    = qBound(0.0f, 100.0f - (priceYuan    / kMaxPrice)    * 100.0f, 100.0f);
    float caloriesScore = qBound(0.0f, 100.0f - (caloriesKcal / kMaxCalories) * 100.0f, 100.0f);
    float distScore     = qBound(0.0f, 100.0f - rawDist / 10.0f, 100.0f);

    RadarData data(tasteRaw, priceScore, experience, caloriesScore, distScore);
    data.actualTaste      = tasteRaw;
    data.actualPrice      = priceYuan;
    data.actualExperience = experience;
    data.actualCalories   = caloriesKcal;
    data.actualDistance   = rawDist;
    data.showActualValues = true;

    m_radarChart->setData(data, dishName);
}

// ============================================================
// 早餐组合雷达 — 饮品 + 食物
// 总价(标注)、美味&体验取均值、热量取总和、距离标注
// ============================================================
void MainWindow::updateRadarForBreakfastCombo(int drinkId, int foodId, const QString& userLocation)
{
    m_userLocation = userLocation;

    auto queryDish = [](int id, QString &name, float &taste, float &price,
                         float &exp, float &cal, int &dist) -> bool {
        QSqlQuery q;
        q.prepare("SELECT d.name, d.taste, d.price, d.experience, d.calories, "
                  "r.dist_dorm, r.dist_library, r.dist_building "
                  "FROM Dishes d "
                  "JOIN Restaurants r ON d.restaurant_id = r.id "
                  "WHERE d.id = ?");
        q.addBindValue(id);
        if (!q.exec() || !q.next()) return false;
        name  = q.value(0).toString();
        taste = q.value(1).toFloat();
        price = q.value(2).toFloat();
        exp   = q.value(3).toFloat();
        cal   = q.value(4).toFloat();

        // 取最近距离
        int dd = q.value(5).toInt();
        int dl = q.value(6).toInt();
        int db = q.value(7).toInt();
        dist = qMin(dd, qMin(dl, db));
        return true;
    };

    QString name1, name2;
    float t1, p1, e1, c1, t2, p2, e2, c2;
    int dist1, dist2;

    if (!queryDish(drinkId, name1, t1, p1, e1, c1, dist1) ||
        !queryDish(foodId,  name2, t2, p2, e2, c2, dist2)) {
        qDebug() << "早餐组合查询失败";
        return;
    }

    // 组合计算
    float totalPrice    = p1 + p2;              // 总价
    float avgTaste      = (t1 + t2) / 2.0f;     // 美味取均值
    float avgExperience = (e1 + e2) / 2.0f;     // 体验取均值
    float totalCalories = c1 + c2;              // 热量取总和
    int   bestDist      = qMin(dist1, dist2);   // 取更近的

    // 归一化
    float priceScore    = qBound(0.0f, 100.0f - (totalPrice    / kMaxPrice)    * 100.0f, 100.0f);
    float caloriesScore = qBound(0.0f, 100.0f - (totalCalories / kMaxCalories) * 100.0f, 100.0f);
    float distScore     = qBound(0.0f, 100.0f - bestDist / 10.0f, 100.0f);

    RadarData data(avgTaste, priceScore, avgExperience, caloriesScore, distScore);
    data.actualTaste      = avgTaste;
    data.actualPrice      = totalPrice;
    data.actualExperience = avgExperience;
    data.actualCalories   = totalCalories;
    data.actualDistance   = bestDist;
    data.showActualValues = true;

    QString comboName = name1 + QString::fromUtf8(" + ") + name2;
    m_radarChart->setData(data, comboName);
}

// ============================================================
// 历史记录 + 权重联动
// ============================================================
void MainWindow::recordHistoryAndAdjustWeight(int dishId, int rating)
{
    QString currentTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO History (dish_id, rating, timestamp) VALUES (?, ?, ?)");
    query.addBindValue(dishId);
    query.addBindValue(rating);
    query.addBindValue(currentTimeStr);

    if (!query.exec()) {
        qDebug() << "历史记录更新失败:" << query.lastError().text();
        return;
    }

    qDebug() << "=====================================";
    qDebug() << "[评分记录] 菜品ID:" << dishId << "评分:" << rating << "星"
             << "时间:" << currentTimeStr;

    bool ok = Recommend::recalculateAllWeights();

    if (ok) {
        qDebug() << "[权重更新] UCB1 全量权重已重新计算并写回数据库";
    } else {
        qDebug() << "[权重更新] 失败！";
    }
    qDebug() << "=====================================";
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    float s = qMin(w, h) / 30.0f;

    float stripH = h * 0.15f;
    QRectF stripRect(0, h - stripH, w, stripH);
    painter.setBrush(QColor(200, 106, 90, 40));
    painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(stripRect, 3.0f));

    QRectF stripRect2(0, h - stripH - 15, w, stripH + 10);
    painter.setBrush(QColor(208, 227, 239, 25));
    painter.drawPath(DecoPainter::makeOrganicRect(stripRect2, 4.0f, 6));

    DecoPainter::drawPaperTexture(painter, QRect(0, 0, w, h));

    int timeY = h * 0.24f;
    DecoPainter::drawScratchyLine(painter, QPointF(w*0.2f, timeY), QPointF(w*0.8f, timeY),
                                  QColor(43, 43, 43, 60), 1.0f, 1.5f);

    DecoPainter::drawSakura(painter, QPointF(w * 0.04f, h * 0.03f), s * 0.8f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.10f, h * 0.07f), s * 0.5f, 25);
    DecoPainter::drawSesame(painter, QPointF(w * 0.92f, h * 0.04f), s * 0.35f, 15);
    DecoPainter::drawSesame(painter, QPointF(w * 0.95f, h * 0.09f), s * 0.3f, -20);
    DecoPainter::drawScallion(painter, QPointF(w * 0.96f, h * 0.06f), s * 0.7f, 30);
    DecoPainter::drawXiaolongbao(painter, QPointF(w * 0.04f, h * 0.94f), s * 0.9f);
    DecoPainter::drawSesame(painter, QPointF(w * 0.12f, h * 0.96f), s * 0.3f, -10);
    DecoPainter::drawTinyCat(painter, QPointF(w * 0.95f, h * 0.93f), s * 1.0f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.88f, h * 0.96f), s * 0.4f, -30);
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.08f, h * 0.90f), s * 0.6f,
                                       QColor(250, 235, 215, 30));
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.92f, h * 0.10f), s * 0.5f,
                                       QColor(250, 240, 220, 25));
    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.97f), s * 0.3f,
                                 QColor(200, 106, 90, 40));
}

void MainWindow::updateClock()
{
    QString timeText = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->timeLabel->setText(timeText);
}

void MainWindow::on_openHistoryButton_clicked()
{
    HistoryWindow *historyDlg = new HistoryWindow(this);
    historyDlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(historyDlg, &HistoryWindow::dishRated, this, [this](int dishId, int rating) {
        m_currentDishId = dishId;
        recordHistoryAndAdjustWeight(dishId, rating);
    });
    historyDlg->exec();
}

void MainWindow::on_openCalendarButton_clicked()
{
    CalendarWindow *calWin = new CalendarWindow(this);
    calWin->setAttribute(Qt::WA_DeleteOnClose);
    calWin->exec();
}
