#include "HistoryWindow.h"
#include "DecoPainter.h"
#include "SketchyButton.h"
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>

// ==========================================
// 艺术市集色板 — 复古蓝底 + 暖色卡片
// ==========================================
static const QList<QColor> kCardColors = {
    QColor("#FAF8F5"),  // 奶油白
    QColor("#F5F0E8"),  // 暖米
    QColor("#F2EAE4"),  // 淡粉
    QColor("#E8EEE8"),  // 淡绿
    QColor("#F0ECE4"),  // 淡杏
    QColor("#FAF5F0"),  // 暖白
};

// ==========================================
// 1. 自定义星级打分控件
// ==========================================
StarRatingWidget::StarRatingWidget(int maxStars, QWidget *parent)
    : QWidget(parent), m_maxStars(maxStars), m_rating(0),
      m_pendingRating(0), m_previewing(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);

    for (int i = 0; i < m_maxStars; ++i) {
        QPushButton *btn = new QPushButton("☆", this);
        btn->setStyleSheet(
            "QPushButton {"
            "  border: none; background: transparent;"
            "  color: #D0CCC6; font-size: 24px;"
            "  font-weight: normal; padding: 2px;"
            "}"
            "QPushButton:hover { color: #C86A5A; }");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("starIndex", i + 1);
        connect(btn, &QPushButton::clicked, this, &StarRatingWidget::onStarClicked);
        starButtons.append(btn);
        layout->addWidget(btn);
    }
    layout->addStretch();
}

void StarRatingWidget::onStarClicked()
{
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) return;
    int proposedRating = clickedBtn->property("starIndex").toInt();
    if (proposedRating == m_rating && !m_previewing) return;
    m_pendingRating = proposedRating;
    m_previewing = true;
    updateStarUIPreview(proposedRating);
    emit ratingRequested(proposedRating);
}

void StarRatingWidget::confirmRating(int rating)
{
    m_rating = rating;
    m_previewing = false;
    m_pendingRating = 0;
    updateStarUI();
    emit ratingChanged(rating);
}

void StarRatingWidget::cancelPreview()
{
    m_previewing = false;
    m_pendingRating = 0;
    updateStarUI();
}

void StarRatingWidget::flashEffect()
{
    QString flashBg =
        "StarRatingWidget { background-color: #F5E8DC; border: 2px solid #C86A5A; border-radius: 10px; }";
    QString normalBg;
    setStyleSheet(flashBg);
    QTimer::singleShot(150, this, [this, normalBg, flashBg]() {
        setStyleSheet(normalBg);
        QTimer::singleShot(120, this, [this, flashBg, normalBg]() {
            setStyleSheet(flashBg);
            QTimer::singleShot(150, this, [this, normalBg, flashBg]() {
                setStyleSheet(normalBg);
                QTimer::singleShot(120, this, [this, flashBg, normalBg]() {
                    setStyleSheet(flashBg);
                    QTimer::singleShot(180, this, [this, normalBg]() { setStyleSheet(normalBg); });
                });
            });
        });
    });
}

void StarRatingWidget::setRating(int rating)
{
    if (rating < 0) rating = 0;
    if (rating > m_maxStars) rating = m_maxStars;
    m_rating = rating;
    m_previewing = false;
    updateStarUI();
}

void StarRatingWidget::updateStarUI()
{
    for (int i = 0; i < m_maxStars; ++i) {
        if (i < m_rating) {
            starButtons[i]->setText("★");
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #C86A5A; font-size: 24px; padding: 2px; }"
                "QPushButton:hover { color: #B05848; }");
        } else {
            starButtons[i]->setText("☆");
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #D0CCC6; font-size: 24px; padding: 2px; }"
                "QPushButton:hover { color: #C86A5A; }");
        }
    }
}

void StarRatingWidget::updateStarUIPreview(int previewRating)
{
    for (int i = 0; i < m_maxStars; ++i) {
        if (i < previewRating) {
            starButtons[i]->setText("★");
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #C86A5A; font-size: 24px; padding: 2px; }"
                "QPushButton:hover { color: #B05848; }");
        } else {
            starButtons[i]->setText("☆");
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #D0CCC6; font-size: 24px; padding: 2px; }"
                "QPushButton:hover { color: #C86A5A; }");
        }
    }
}

// ==========================================
// 2. 单条历史记录控件（有机手绘卡片）
// ==========================================
HistoryItemWidget::HistoryItemWidget(int dishId, const QString& dishName,
                                     const QString& timestamp, const QColor& cardColor,
                                     int initialRating, QWidget *parent)
    : QWidget(parent), m_dishId(dishId), m_dishName(dishName), m_cardColor(cardColor)
{
    // 通过样式表设置基本颜色 + 有机边缘由 paintEvent 处理
    QString bgName = cardColor.name();
    setStyleSheet(
        QString(
        "HistoryItemWidget {"
        "  background-color: %1;"
        "  border: none;"
        "}").arg(bgName));

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(18, 12, 18, 12);
    mainLayout->setSpacing(12);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(3);

    QLabel *nameLabel = new QLabel(dishName, this);
    nameLabel->setStyleSheet(
        "font-size: 17px; font-weight: bold; border: none;"
        "background: transparent; color: #2B2B2B;"
        "font-family: 'Microsoft YaHei';");

    QLabel *dateLabel = new QLabel(timestamp, this);
    dateLabel->setStyleSheet(
        "font-size: 12px; border: none;"
        "background: transparent; color: #8B8B8B;"
        "font-family: 'Microsoft YaHei';");

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(dateLabel);
    mainLayout->addLayout(infoLayout, 1);

    m_starWidget = new StarRatingWidget(5, this);
    m_starWidget->setRating(0);
    connect(m_starWidget, &StarRatingWidget::ratingRequested,
            this, &HistoryItemWidget::onRatingRequested);

    mainLayout->addWidget(m_starWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    setMinimumHeight(68);
}

void HistoryItemWidget::paintEvent(QPaintEvent *event)
{
    // 先绘制有机背景 + sketchy 边框
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r = rect().adjusted(1, 1, -1, -1);

    // 底色
    p.setBrush(m_cardColor);
    p.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeWavyRect(r, 1.8f);
    p.drawPath(bgPath);

    // sketchy 边框 (2层排线)
    QColor ink(43, 43, 43, 100);
    DecoPainter::drawSketchyBorder(p, QPainterPath(bgPath), ink, 2, 1.0f);

    // 左侧色条 (手绘感)
    float barW = 4.0f;
    QRectF barR(r.x() + 6, r.y() + r.height() * 0.15f,
                barW, r.height() * 0.7f);
    p.setBrush(m_cardColor.darker(130));
    p.setPen(Qt::NoPen);
    QPainterPath barPath = DecoPainter::makeWavyRect(barR, 0.8f);
    p.drawPath(barPath);

    // 让子控件绘制在前面
    p.end();
    QWidget::paintEvent(event);
}

void HistoryItemWidget::onRatingRequested(int proposedRating)
{
    QString starStr;
    for (int i = 0; i < proposedRating; ++i) starStr += QString::fromUtf8("★ ");
    for (int i = proposedRating; i < 5; ++i) starStr += QString::fromUtf8("☆ ");

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认评分");
    msgBox.setText(QString::fromUtf8(
        "<div style='text-align:center;'>"
        "<p style='font-size:16px; color:#2B2B2B; margin-bottom:10px;'>"
        "确认给 <b>%1</b> 打 %2 颗星吗？</p>"
        "<p style='font-size:26px; color:#C86A5A; letter-spacing:4px;'>%3</p>"
        "</div>")
        .arg(m_dishName).arg(proposedRating).arg(starStr.trimmed()));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setButtonText(QMessageBox::Yes, "确认");
    msgBox.setButtonText(QMessageBox::No,  "取消");
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #FAF8F5; border: 1px solid #E0D8D0; border-radius: 12px; }"
        "QLabel { min-width: 240px; }"
        "QPushButton {"
        "  padding: 8px 24px; font-size: 14px; border-radius: 10px;"
        "  border: 1.5px solid #D5C8B8; background-color: #F5F0E8; color: #2B2B2B;"
        "  font-family: 'Microsoft YaHei';"
        "}"
        "QPushButton:hover { background-color: #EDE4D8; }");

    if (msgBox.exec() == QMessageBox::Yes) {
        m_starWidget->confirmRating(proposedRating);
        m_starWidget->flashEffect();
        emit dishRated(m_dishId, proposedRating);
    } else {
        m_starWidget->cancelPreview();
    }
}

// ==========================================
// 3. 历史记录主窗口 — 艺术市集风格
// ==========================================
HistoryWindow::HistoryWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle("干饭档案");
    resize(500, 640);
    setStyleSheet(QString("QDialog { background: transparent; }"));

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 18, 24, 18);
    mainLayout->setSpacing(14);

    // 顶部标题
    QLabel *titleLabel = new QLabel("干 饭 档 案", this);
    titleLabel->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: #2B2B2B;"
        "padding: 14px 0 2px 0; letter-spacing: 10px;"
        "font-family: 'Microsoft YaHei';");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QLabel *subTitleLabel = new QLabel("点击星星为你吃过的菜打分", this);
    subTitleLabel->setStyleSheet(
        "font-size: 13px; color: #8B8B8B; padding-bottom: 6px;"
        "font-family: 'Microsoft YaHei';");
    subTitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subTitleLabel);

    // 滚动区域
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: #E8E0D8; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #C8BAB0; border-radius: 4px; min-height: 40px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("background: transparent;");
    listLayout = new QVBoxLayout(scrollWidget);
    listLayout->setSpacing(14);
    listLayout->setContentsMargins(8, 10, 8, 10);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    loadFromDatabase();
}

void HistoryWindow::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // ==========================================
    // 色块撞色背景：复古蓝 + 暖米色条
    // ==========================================
    // 主背景色块
    painter.setBrush(QColor(208, 227, 239, 180));
    painter.setPen(Qt::NoPen);
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0, 0, w, h), 3.0f, 7));

    // 右侧装饰色条（芥末绿）
    painter.setBrush(QColor(242, 233, 212, 80));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(w * 0.85f, 0, w * 0.18f, h), 4.0f, 6));

    // 底部装饰色块（复古红褐极淡）
    painter.setBrush(QColor(200, 106, 90, 18));
    painter.drawPath(DecoPainter::makeOrganicRect(QRectF(0, h * 0.85f, w, h * 0.18f), 3.0f, 5));

    // 复古纸张纹理
    DecoPainter::drawPaperTexture(painter, QRect(0, 0, w, h));

    // ==========================================
    // 毛刺分割线
    // ==========================================
    DecoPainter::drawScratchyLine(painter, QPointF(w*0.15f, h*0.12f), QPointF(w*0.85f, h*0.12f),
                                  QColor(43, 43, 43, 45), 0.8f, 1.2f);

    // ==========================================
    // 角落涂鸦装饰
    // ==========================================
    float s = qMin(w, h) / 28.0f;
    DecoPainter::drawSakura(painter, QPointF(w * 0.05f, h * 0.04f), s * 0.6f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.12f, h * 0.09f), s * 0.35f, 35);
    DecoPainter::drawSesame(painter, QPointF(w * 0.90f, h * 0.04f), s * 0.25f, 20);
    DecoPainter::drawXiaolongbao(painter, QPointF(w * 0.04f, h * 0.93f), s * 0.7f);
    DecoPainter::drawTinyCat(painter, QPointF(w * 0.95f, h * 0.92f), s * 0.8f);
    DecoPainter::drawPetal(painter, QPointF(w * 0.87f, h * 0.95f), s * 0.3f, -25);
    DecoPainter::drawScallion(painter, QPointF(w * 0.94f, h * 0.50f), s * 0.5f, 20);

    // 不完美圆形装饰
    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.02f), 15,
                                 QColor(200, 106, 90, 35));
    DecoPainter::drawRoughCircle(painter, QPointF(w * 0.50f, h * 0.97f), 12,
                                 QColor(208, 227, 239, 50));

    // 水彩晕染
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.05f, h * 0.90f), s * 0.5f,
                                       QColor(250, 235, 215, 22));
}

void HistoryWindow::loadFromDatabase()
{
    QSqlQuery query;
    query.exec("SELECT d.id, d.name, "
               "COALESCE((SELECT h.timestamp FROM History h WHERE h.dish_id = d.id), "
               "         '尚未评分') as ts "
               "FROM Dishes d ORDER BY d.id");

    bool hasData = false;
    int colorIndex = 0;
    while (query.next()) {
        hasData = true;
        int dishId = query.value(0).toInt();
        QString dishName = query.value(1).toString();
        QString timestamp = query.value(2).toString();
        QColor cardColor = kCardColors[colorIndex % kCardColors.size()];
        colorIndex++;

        HistoryItemWidget *itemWidget = new HistoryItemWidget(dishId, dishName, timestamp, cardColor, 0, this);
        listLayout->addWidget(itemWidget);
        connect(itemWidget, &HistoryItemWidget::dishRated,
                this, &HistoryWindow::handleDishRatingUpdate);
    }

    if (!hasData) {
        QLabel *emptyLabel = new QLabel("还没有菜品数据，请先导入 Excel 数据", this);
        emptyLabel->setStyleSheet("color: #8B8B8B; font-size: 14px; padding: 40px 0;"
                                  "font-family: 'Microsoft YaHei';");
        emptyLabel->setAlignment(Qt::AlignCenter);
        listLayout->addWidget(emptyLabel);
    }
    listLayout->addStretch();
}

// 星级 → 美味度分数映射
static int starToTasteScore(int stars) {
    switch (stars) {
    case 1:  return 70;
    case 2:  return 75;
    case 3:  return 80;
    case 4:  return 85;
    case 5:  return 90;
    default: return 70;
    }
}

void HistoryWindow::handleDishRatingUpdate(int dishId, int newRating)
{
    qDebug() << "=====================================";
    qDebug() << "[评价触发] 菜品ID:" << dishId << "最新评分:" << newRating << "星";

    // ---- 星级 → 美味度：更新菜品 taste 字段 ----
    int newTaste = starToTasteScore(newRating);
    QSqlQuery tasteUpdate;
    tasteUpdate.prepare("UPDATE Dishes SET taste = ? WHERE id = ?");
    tasteUpdate.addBindValue(newTaste);
    tasteUpdate.addBindValue(dishId);
    if (tasteUpdate.exec()) {
        qDebug() << "[美味度更新] 菜品ID:" << dishId
                 << newRating << "星 → 美味度 =" << newTaste;
    }

    QSqlQuery q;
    q.prepare("SELECT d.name, "
              "COALESCE((SELECT COUNT(*) FROM History h WHERE h.dish_id = d.id), 0), "
              "COALESCE((SELECT SUM(rating) FROM History h2 WHERE h2.dish_id = d.id), 0) "
              "FROM Dishes d WHERE d.id = ?");
    q.addBindValue(dishId);
    if (q.exec() && q.next()) {
        QString name = q.value(0).toString();
        int cnt = q.value(1).toInt();
        double total = q.value(2).toDouble();
        double avg = cnt > 0 ? total / cnt : 0.0;

        QSqlQuery tq;
        tq.exec("SELECT COUNT(*) FROM History");
        int T = tq.next() ? tq.value(0).toInt() : 1;

        double n = qMax(cnt, 1);
        double exploration = 2.0 * qSqrt(qLn(static_cast<double>(T)) / n);
        double exploitation = (cnt > 0) ? avg : 3.0;

        qDebug() << "[UCB1 调试] 菜名:" << name
                 << "| n:" << cnt << "| x̄:" << QString::number(avg, 'f', 2)
                 << "| 利用:" << QString::number(exploitation, 'f', 2)
                 << "| 探索:" << QString::number(exploration, 'f', 2)
                 << "| UCB1:" << QString::number(exploitation + exploration, 'f', 2);
    }
    qDebug() << "=====================================\n";
    emit dishRated(dishId, newRating);
}
