#include "mainwindow.h"
#include "mealpage.h"
#include "historywindow.h"
#include "zonemanager.h"
#include "zoneeditor.h"
#include "distancedb.h"
#include "zonecommon.h"
#include "settingsdialog.h"
#include "manualwindow.h"
#include "decopainter.h"
#include "achievementmanager.h"
#include "achievementpage.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QRandomGenerator>
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QToolTip>
#include <QTime>
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <cmath>
#include <algorithm>

static const QColor C_CREAM = QColor("#FDFBF7");
static const QColor C_INK = QColor("#2B2B2B");
static const QColor C_INK_LIGHT = QColor("#4A4540");
static const QColor C_SHADOW_DK = QColor("#3A3530");
static const QColor C_CARD_SAGE = QColor("#DCE4D3");
static const QColor C_CARD_BLUE = QColor("#D0DDE8");
static const QColor C_CARD_ROSE = QColor("#EAD7D2");
static const QColor C_CARD_TAUPE = QColor("#E0D7CC");
static const QColor C_CARD_GOLD = QColor("#F2E5C7");
static const QColor C_CARD_PURPLE = QColor("#D5CFDF");

CharacterItem::CharacterItem(const QString &spritePath, int size, QGraphicsItem *parent)
    : QGraphicsObject(parent), m_size(size) { m_sprite = QPixmap(spritePath); }

QRectF CharacterItem::boundingRect() const {
    int d = m_size + 8; return QRectF(-d/2.0, -d/2.0, d, d);
}

void CharacterItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    if (m_sprite.isNull()) return;
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    int bob = static_cast<int>(std::sin(m_walkPhase) * 2.0);
    painter->drawPixmap(QRectF(-m_size/2.0, -m_size/2.0+bob, m_size, m_size), m_sprite, m_sprite.rect());
}

void CharacterItem::setSprite(const QString &path) {
    m_sprite = QPixmap(path);
    update();
}

// 开屏界面
WelcomePage::WelcomePage(QWidget *parent) : QWidget(parent) {
    setCursor(Qt::PointingHandCursor);
    m_bgPixmap = QPixmap("start_page_background.png");
    m_titleArt = QPixmap("title-art.png");
    m_fadeAnim = new QPropertyAnimation(this, "fadeIn", this);
    m_fadeAnim->setDuration(900); m_fadeAnim->setStartValue(0.0);
    m_fadeAnim->setEndValue(1.0); m_fadeAnim->setEasingCurve(QEasingCurve::OutCubic);
}
void WelcomePage::setFadeIn(double v) { m_fadeIn = v; update(); }
void WelcomePage::showEvent(QShowEvent *) { m_fadeAnim->start(); }

void WelcomePage::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height(); double f = m_fadeIn;
    if (!m_bgPixmap.isNull()) {
        p.setOpacity(f*0.42); p.setRenderHint(QPainter::SmoothPixmapTransform);
        QSizeF bgSize = m_bgPixmap.size();
        double scale = qMin(w/bgSize.width(), h/bgSize.height());
        double bw = bgSize.width()*scale, bh = bgSize.height()*scale;
        p.drawPixmap(QRectF((w-bw)/2.0,(h-bh)/2.0,bw,bh), m_bgPixmap, m_bgPixmap.rect());
        p.setOpacity(f);
    } else p.fillRect(rect(), C_CREAM);

    p.save(); p.setOpacity(f);
    double cardW=w*0.52, cardH=h*0.34, cardX=(w-cardW)/2.0, cardY=h*0.28;
    QRectF cardRect(cardX, cardY, cardW, cardH);
    double sx=5.0, sy=6.0;
    p.setBrush(QColor("#3A3530")); p.setPen(Qt::NoPen);
    p.drawPath(sketchyRect(QRectF(cardX+sx,cardY+sy,cardW,cardH), 87, 2.8));
    QPainterPath cardPath = sketchyRect(cardRect, 42, 2.8);
    drawInkWash(&p, cardPath, QColor("#FAF7F0"), 15);
    drawInkBorder(&p, cardPath, C_INK, 3, 0.8);
    if (!m_titleArt.isNull()) {
        double pad = 5;
        double maxW = cardW - pad * 2, maxH = cardH - pad * 2;
        QSizeF sz = m_titleArt.size();
        double s = qMin(maxW / sz.width(), maxH / sz.height());
        double iw = sz.width() * s, ih = sz.height() * s;
        double imgX = cardX + pad + (maxW - iw) / 2.0 + 3;
        p.drawPixmap(QRectF(imgX, cardY + pad + (maxH - ih) / 2.0, iw, ih),
                     m_titleArt, m_titleArt.rect());
    } else {
        QFont titleFont; titleFont.setPointSize(qMin(h*0.075,36.0)); titleFont.setWeight(QFont::Bold);
        p.setFont(titleFont); p.setPen(C_INK);
        p.drawText(QRectF(cardX,cardY+cardH*0.08,cardW,cardH*0.38), Qt::AlignCenter,
                   QString::fromUtf8("今天吃什么"));
    }
    QFont hintFont; hintFont.setPointSize(qMin(h*0.020,10.0)); p.setFont(hintFont);
    p.setPen(QColor("#9A9590"));
    p.drawText(QRectF(cardX,cardY+cardH,cardW,h*0.08), Qt::AlignCenter,
               QString::fromUtf8("—— 点击任意位置进入 ——"));
    p.restore();
}

// 侧边栏
Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setFixedWidth(255);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20,40,20,32); layout->setSpacing(16);

    QLabel *label = new QLabel(QString::fromUtf8("菜  单"));
    QFont lblFont; lblFont.setPointSize(12);
    lblFont.setLetterSpacing(QFont::AbsoluteSpacing,4); label->setFont(lblFont);
    label->setStyleSheet(QString("color: %1; padding-left: 4px;").arg(C_INK_LIGHT.name()));
    layout->addWidget(label); layout->addSpacing(6);

    m_cardBtn = new SketchyButton(QString::fromUtf8("去吃饭"), C_CARD_SAGE, C_SHADOW_DK);
    m_historyBtn = new SketchyButton(QString::fromUtf8("历史记录"), C_CARD_BLUE, C_SHADOW_DK);
    m_reviewBtn = new SketchyButton(QString::fromUtf8("菜品评价"), C_CARD_PURPLE, C_SHADOW_DK);
    m_achievementBtn = new SketchyButton(QString::fromUtf8("勋章成就"), C_CARD_ROSE, C_SHADOW_DK);
    m_settingsBtn = new SketchyButton(QString::fromUtf8("设置"), C_CARD_GOLD, C_SHADOW_DK);
    m_manualBtn = new SketchyButton(QString::fromUtf8("用户手册"), C_CARD_TAUPE, C_SHADOW_DK);

    for (auto *b : {m_cardBtn, m_historyBtn, m_reviewBtn, m_settingsBtn, m_achievementBtn, m_manualBtn}) {
        b->setIconType(ICON_NONE);
        b->setFixedHeight(56);
    }
    connect(m_cardBtn, &QPushButton::clicked, this, &Sidebar::goEatClicked);
    connect(m_historyBtn, &QPushButton::clicked, this, &Sidebar::historyClicked);
    connect(m_reviewBtn, &QPushButton::clicked, this, &Sidebar::reviewClicked);
    connect(m_achievementBtn, &QPushButton::clicked, this, &Sidebar::achievementsClicked);
    connect(m_settingsBtn, &QPushButton::clicked, this, &Sidebar::settingsClicked);
    connect(m_manualBtn, &QPushButton::clicked, this, &Sidebar::manualClicked);

    layout->addWidget(m_cardBtn);
    layout->addWidget(m_historyBtn);
    layout->addWidget(m_reviewBtn);
    layout->addWidget(m_achievementBtn);
    layout->addWidget(m_settingsBtn);
    layout->addWidget(m_manualBtn);

    m_achievementDot = new QLabel(m_achievementBtn);
    m_achievementDot->setFixedSize(10, 10);
    m_achievementDot->setStyleSheet("background:#D45A3A; border-radius:5px;");
    m_achievementDot->move(m_achievementBtn->width() - 16, 6);
    m_achievementDot->hide();

    layout->addStretch();

    QFrame *sep = new QFrame; sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("QFrame { color: #C4BDB3; }");
    layout->addWidget(sep); layout->addSpacing(12);

    QHBoxLayout *userRow = new QHBoxLayout;
    m_avatarLabel = new QLabel; m_avatarLabel->setFixedSize(50,50);
    QPixmap defAv(50,50); defAv.fill(Qt::transparent);
    { QPainter p(&defAv); p.setRenderHint(QPainter::Antialiasing);
      p.setBrush(QColor("#C4BDB3")); p.setPen(Qt::NoPen); p.drawEllipse(0,0,50,50); }
    m_avatarLabel->setPixmap(defAv); userRow->addWidget(m_avatarLabel);
    QFont infoFont; infoFont.setPointSize(12);
    infoFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);

    m_nameLabel = new QLabel(QString::fromUtf8("用户"));
    m_nameLabel->setFont(infoFont);
    m_nameLabel->setStyleSheet(QString("color:%1;font-weight:bold;").arg(C_INK.name()));
    userRow->addWidget(m_nameLabel); userRow->addStretch(); layout->addLayout(userRow);
    layout->addSpacing(4);

    QFont subFont; subFont.setPointSize(10);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);

    m_calorieLabel = new QLabel(QString::fromUtf8("今日已摄入 0 kcal"));
    m_calorieLabel->setFont(subFont);
    m_calorieLabel->setStyleSheet("color:#9A9590;padding-left:4px;");
    m_calorieLabel->setWordWrap(true); layout->addWidget(m_calorieLabel);
    m_bmrLabel = new QLabel(QString::fromUtf8("基础代谢 —  kcal/天"));
    m_bmrLabel->setFont(subFont);
    m_bmrLabel->setStyleSheet("color:#B5AFA8;padding-left:4px;");
    m_bmrLabel->setWordWrap(true); layout->addWidget(m_bmrLabel);
}

static QPixmap circleCrop(const QPixmap &src, int size) {
    if (src.isNull()) return {};
    QPixmap result(size,size); result.fill(Qt::transparent); QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing); QPainterPath path; path.addEllipse(0,0,size,size);
    p.setClipPath(path);
    p.drawPixmap(0,0,size,size,src.scaled(size,size,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
    return result;
}
void Sidebar::setAvatar(const QPixmap &pixmap) { if(!pixmap.isNull()) m_avatarLabel->setPixmap(circleCrop(pixmap,50)); }
void Sidebar::setUserName(const QString &name) { m_nameLabel->setText(name); }
void Sidebar::setTodayCalories(int kcal) {
    m_todayCalories = kcal;
    m_calorieLabel->setText(QString::fromUtf8("今日已摄入 %1 kcal").arg(kcal));
}
void Sidebar::setBMR(int bmr) {
    m_bmrLabel->setText(QString::fromUtf8("建议摄入 %1  kcal/天").arg(bmr));
}
void Sidebar::setAchievementDot(bool show) {
    if (m_achievementDot) m_achievementDot->setVisible(show);
}
void Sidebar::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    drawInkWash(&p, sketchyRect(rect().adjusted(0,0,4,0),13,4.0), C_CREAM, 12);
    QPainterPath line; double x = width()-0.5; line.moveTo(x+1.0,0);
    for (double y=0; y<height(); y+=8.0) {
        double jx = x + (std::sin(y*0.3)*1.2) + ((y/8.0-qFloor(y/8.0))*2.0-1.0);
        double jy = y + (std::sin(x*1.7+y*0.1)*1.5);
        line.lineTo(jx,jy);
    }
    QPen linePen(C_INK); linePen.setWidthF(1.0); p.setPen(linePen);
    p.setBrush(Qt::NoBrush); p.drawPath(line);
}

int ReviewDialog::findZoneForRestaurant(const QString &restaurant) const {
    return m_restaurantZoneMap.value(restaurant, -1);
}

double ReviewDialog::getDistance(int zoneA, int zoneB) const {
    if (!m_distanceDB || zoneA < 0 || zoneB < 0 || zoneA == zoneB) return 0;
    return m_distanceDB->getDistance(zoneA, zoneB);
}

ReviewDialog::ReviewDialog(const QVector<Dish> &dishes, int userZoneId,
                           ZoneManager *zoneMgr, DistanceDB *distDB,
                           const UserSettings &settings,
                           const QMap<QString, int> &restaurantZoneMap,
                           QWidget *parent)
    : QDialog(parent), m_zoneManager(zoneMgr), m_distanceDB(distDB),
      m_userZoneId(userZoneId), m_userData(settings),
      m_restaurantZoneMap(restaurantZoneMap)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(520, 700);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 40, 36, 32);
    layout->setSpacing(12);

    QFont titleFont; titleFont.setPointSize(16);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
    titleFont.setWeight(QFont::Bold);
    QFont bodyFont; bodyFont.setPointSize(11);
    bodyFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    QFont hintFont; hintFont.setPointSize(10);
    hintFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("你的今日搭配"));
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QString("color: %1;").arg(C_INK.name()));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    layout->addSpacing(4);

    double totalPrice=0, totalCalories=0, avgTaste=0, avgExp=0, avgHealth=0;
    QString restaurant;
    for (const auto &d : dishes) {
        totalPrice+=d.price; totalCalories+=d.calories;
        avgTaste+=(d.tastyScore>0?d.tastyScore:5.0); avgExp+=d.experienceScore; avgHealth+=d.healthScore;
        if(restaurant.isEmpty()) restaurant=d.restaurant;
    }
    int n=dishes.size();
    if(n>0){ avgTaste/=n; avgExp/=n; avgHealth/=n; }

    int hour = QTime::currentTime().hour();
    bool isBreakfast = (hour < 10);
    double priceBaseline = isBreakfast ? 10.0 : (hour < 16 ? 18.0 : 16.0);
    double calBaseline = 2500;
    if (m_userData.height>0 && m_userData.weight>0 && m_userData.age>0 && !m_userData.gender.isEmpty()) {
        double bmr;
        if (m_userData.gender == QString::fromUtf8("女"))
            bmr = 10.0*m_userData.weight + 6.25*m_userData.height - 5.0*m_userData.age - 161;
        else
            bmr = 10.0*m_userData.weight + 6.25*m_userData.height - 5.0*m_userData.age + 5;
        calBaseline = (isBreakfast ? bmr*0.3 : (hour < 16 ? bmr*0.4 : bmr*0.3)) * 1.35;
    }

    int restZone = findZoneForRestaurant(restaurant);
    double distMeters = 0;
    double avgDist = 200;
    if (restZone >= 0 && m_userZoneId >= 0 && m_distanceDB) {
        distMeters = getDistance(m_userZoneId, restZone) * 1000.0;
        double totalDist=0; int count=0;
        const auto zones = m_zoneManager->allZones();
        for (const auto &z : zones) {
            if (z.category.isEmpty()) continue;
            double d = getDistance(m_userZoneId, z.id);
            if (d >= 0) { totalDist += d; count++; }
        }
        if (count>0) avgDist = (totalDist/count) * 1000.0;
    }

    float priceScore = qBound(0.0f, 100.0f * (1.0f - static_cast<float>(totalPrice) / (3.0f * static_cast<float>(priceBaseline))), 100.0f);
    float calScore = qBound(0.0f,
        100.0f * (1.0f - static_cast<float>(qAbs(totalCalories-calBaseline)) / (2.0f * static_cast<float>(calBaseline))), 100.0f);
    float distScore;
    if (restZone < 0 || m_userZoneId < 0) {
        distScore = 50.0f;
    } else {
        distScore = qBound(0.0f, 100.0f * (1.0f - static_cast<float>(distMeters) / (3.0f * static_cast<float>(avgDist))), 100.0f);
    }

    RadarData data(static_cast<float>(avgTaste*10), priceScore,
                   static_cast<float>(avgExp*10), calScore, distScore);
    data.actualTaste = static_cast<float>(avgTaste);
    data.actualPrice = static_cast<float>(totalPrice);
    data.actualExperience = static_cast<float>(avgExp);
    data.actualCalories = static_cast<float>(totalCalories);
    data.actualDistance = static_cast<float>(distMeters);
    data.showActualValues = true;

    m_radar = new RadarChartWidget(this);
    m_radar->setMinimumSize(380, 380);
    m_radar->setData(data, QString::fromUtf8("综合搭配"));
    layout->addWidget(m_radar);

    QString dishNames;
    for (const auto &d : dishes) {
        if(!dishNames.isEmpty()) dishNames += ", ";
        dishNames += d.name;
    }
    QString mealType = isBreakfast ? QString::fromUtf8("早餐") :
                       (hour < 16 ? QString::fromUtf8("午餐") : QString::fromUtf8("晚餐"));
    m_summaryLabel = new QLabel(QString::fromUtf8("餐段：%1\n菜品：%2\n总价：¥%3 | 热量：%4 kcal（建议%5）\n食堂：%6 | 距离：%7m")
        .arg(mealType).arg(dishNames)
        .arg(totalPrice,0,'f',1).arg(totalCalories,0,'f',0).arg(calBaseline,0,'f',0)
        .arg(restaurant).arg(distMeters,0,'f',0));
    m_summaryLabel->setFont(hintFont);
    m_summaryLabel->setStyleSheet(QString("color: %1;background:transparent;").arg(C_INK_LIGHT.name()));
    m_summaryLabel->setWordWrap(true); m_summaryLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_summaryLabel);
    layout->addSpacing(8);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QFont btnFont; btnFont.setPointSize(10);
    btnFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    SketchyButton *cancelBtn = new SketchyButton(QString::fromUtf8("算了，重新选"),
        C_CARD_TAUPE, C_SHADOW_DK, this);
    cancelBtn->setFixedSize(130, 44);
    cancelBtn->setFont(btnFont);
    SketchyButton *confirmBtn = new SketchyButton(QString::fromUtf8("就这些！开吃！"),
        C_CARD_SAGE, C_SHADOW_DK, this);
    confirmBtn->setFixedSize(130, 44);
    confirmBtn->setFont(btnFont);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(confirmBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, this, [this](){ m_confirmed=false; reject(); });
    connect(confirmBtn, &QPushButton::clicked, this, [this](){ m_confirmed=true; accept(); });
}

void ReviewDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 89;

    QRectF shadow = card.translated(2.5, 3.5);
    QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
    p.setBrush(C_SHADOW_DK);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, C_CREAM, 18);
    drawInkBorder(&p, cp, C_INK, 3, 0.7);
}

void ReviewDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QWidget *child = childAt(e->pos());
        if (!child || child == this) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
        }
    }
    QDialog::mousePressEvent(e);
}

void ReviewDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
        if (delta.manhattanLength() > 4)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    QDialog::mouseMoveEvent(e);
}

void ReviewDialog::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_allDishes = DishData::loadDishes("dishes.json");
    if(m_allDishes.isEmpty())
        QMessageBox::warning(this, QString::fromUtf8("数据错误"), QString::fromUtf8("无法加载 dishes.json"));

    m_stack = new QStackedWidget(this);
    m_stack->setStyleSheet(QString("background:%1;").arg(C_CREAM.name()));
    setCentralWidget(m_stack);

    m_welcomePage = new WelcomePage; m_welcomePage->installEventFilter(this);
    m_zoneManager = new ZoneManager(this); m_distanceDB = new DistanceDB;
    setupMapPage();
    m_zoneEditor = new ZoneEditor(m_mapView, m_mapScene, m_zoneManager, this);

    m_mealPage = new MealPage(m_allDishes, m_userProfile);
    connect(m_mealPage, &MealPage::mealReadyForReview, this, &MainWindow::onMealReadyForReview);
    connect(m_mealPage, &MealPage::backToMap, this, &MainWindow::backFromMeal);

    m_achievementManager = new AchievementManager(this);
    m_achievementManager->load();

    m_achievementPage = new AchievementPage(m_achievementManager);
    m_stack->addWidget(m_achievementPage);

    connect(m_sidebar, &Sidebar::achievementsClicked, this, [this]() {
        m_achievementPage->setIsObese(m_isObese);
        m_achievementPage->refresh();
        m_stack->setCurrentWidget(m_achievementPage);
    });

    connect(m_achievementPage, &AchievementPage::backToMap, this, [this]() {
        m_stack->setCurrentWidget(m_mapPage);
        m_sidebar->setAchievementDot(m_achievementManager->hasNewAchievements());
        m_sidebar->setTodayCalories(static_cast<int>(m_userProfile.todayCalories));
    });

    connect(m_achievementManager, &AchievementManager::activeSkinChanged, this, [this](const QString &) {
        updateLionSprite();
    });

    m_stack->addWidget(m_welcomePage); m_stack->addWidget(m_mapPage);
    m_stack->addWidget(m_mealPage); m_stack->setCurrentWidget(m_welcomePage);

    setMinimumSize(800,600); setWindowTitle(QString::fromUtf8("寻味燕园"));
    showMaximized();

    m_zoneManager->loadFromFile("zones.json");
    m_distanceDB->open("distances.db");
    m_distanceDB->loadFromJson("distances.json");
    m_userData.load("user.json");
    // 加载评分数据
    {
        QFile f("user.json");
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
            QJsonObject ratingsObj = obj["ratings"].toObject();
            for (auto it = ratingsObj.begin(); it != ratingsObj.end(); ++it)
                m_userProfile.ratings[it.key()] = it.value().toDouble();
            // 恢复今日热量
            QString savedDate = obj["calorieDate"].toString();
            if (savedDate == QDate::currentDate().toString("yyyy-MM-dd"))
                m_userProfile.todayCalories = obj["todayCalories"].toDouble();
            f.close();
        }
    }
    applyUserSettings();
    loadDailyRecords();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    if (m_dailyRecords.contains(today)) {
        double drCal = m_dailyRecords[today].totalCalories;
        if (m_userProfile.todayCalories != drCal) {
            m_userProfile.todayCalories = drCal;
            updateSidebarUserInfo();
            saveRatingsToUserFile();
        }
    }
    {
        QFile f("rating_dates.json");
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
            for (auto it = obj.begin(); it != obj.end(); ++it)
                m_ratingDates[it.key()] = it.value().toString();
            f.close();
        }
    }
    loadEatingTimes();

    {
        QFile f("restaurant_zone_map.json");
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
            for (auto it = obj.begin(); it != obj.end(); ++it)
                m_restaurantZoneMap[it.key()] = it.value().toInt(-1);
            f.close();
        }
    }

    // BGM
    m_bgmPlayer = new QMediaPlayer(this);
    m_bgmOutput = new QAudioOutput(this);
    m_bgmPlayer->setAudioOutput(m_bgmOutput);
    QString bgmPath = QCoreApplication::applicationDirPath() + "/久石让+-+Summer+-+菊次郎的夏天+-+钢琴独奏版.mp3";
    m_bgmPlayer->setSource(QUrl::fromLocalFile(bgmPath));
    m_bgmPlayer->setLoops(QMediaPlayer::Infinite);
    {
        int vol = qBound(0, m_userData.settings.bgmVolume, 100);
        m_bgmOutput->setVolume(vol / 100.0);
        if (vol > 0) m_bgmPlayer->play();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if(obj==m_welcomePage && event->type()==QEvent::MouseButtonPress){ enterMap(); return true; }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setupMapPage() {
    m_mapPage = new QWidget;
    m_mapScene = new QGraphicsScene(this);
    m_mapView = new MapView(m_mapScene);
    connect(m_mapView, &MapView::sceneLeftClicked, this, &MainWindow::onSceneLeftClicked);
    m_mapView->setRenderHint(QPainter::Antialiasing);
    m_mapView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_mapView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_mapView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_mapView->setStyleSheet(QString("border:none;background:%1;").arg(C_CREAM.name()));
    m_mapView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    QPixmap mapPix("map_origin.jpg");
    QGraphicsPixmapItem *mapItem = m_mapScene->addPixmap(mapPix);
    m_mapScene->setSceneRect(mapPix.rect());
    double initScale = m_mapView->viewport()->height() / m_mapScene->sceneRect().height();
    m_mapView->scale(initScale, initScale);
    m_mapView->centerOn(m_mapScene->sceneRect().center());

    m_character = new CharacterItem("lion_slim.png", 90);
    m_mapScene->addItem(m_character);
    m_character->setPos(QRandomGenerator::global()->bounded(mapPix.width()),
                        QRandomGenerator::global()->bounded(mapPix.height()));
    m_character->setZValue(10); m_targetPos = m_character->pos();

    m_roamTimer = new QTimer(this); m_roamTimer->setInterval(33);
    connect(m_roamTimer, &QTimer::timeout, this, &MainWindow::tickCharacter);
    m_roamTimer->start();

    m_sidebar = new Sidebar;
    connect(m_sidebar, &Sidebar::goEatClicked,    this, &MainWindow::onGoEat);
    connect(m_sidebar, &Sidebar::historyClicked,  this, &MainWindow::onHistory);
    connect(m_sidebar, &Sidebar::reviewClicked,   this, &MainWindow::onReview);
    connect(m_sidebar, &Sidebar::settingsClicked, this, &MainWindow::onSettings);
    connect(m_sidebar, &Sidebar::manualClicked,  this, &MainWindow::onManual);

    QHBoxLayout *mainLayout = new QHBoxLayout(m_mapPage);
    mainLayout->setContentsMargins(0,0,0,0); mainLayout->setSpacing(0);
    mainLayout->addWidget(m_sidebar); mainLayout->addWidget(m_mapView,1);
}

void MainWindow::enterMap() { m_stack->setCurrentWidget(m_mapPage);
    m_mapView->resetTransform();
    double s = m_mapView->viewport()->height() / m_mapScene->sceneRect().height();
    m_mapView->scale(s, s);
    m_mapView->centerOn(m_mapScene->sceneRect().center());
    m_mapView->setFocus(); }

void MainWindow::tickCharacter() {
    QRectF bounds = m_mapScene->sceneRect(); if(bounds.isEmpty()) return;
    QPointF cur = m_character->pos();
    double dx=m_targetPos.x()-cur.x(), dy=m_targetPos.y()-cur.y();
    double dist=std::sqrt(dx*dx+dy*dy), speed=m_roaming?100.0:200.0, step=speed*0.033;
    if(dist<step){ m_character->setPos(m_targetPos); m_character->resetWalkPhase();
        if(m_roaming){ double margin=60;
            m_targetPos=QPointF(QRandomGenerator::global()->bounded(bounds.width()-margin*2)+margin,
                                QRandomGenerator::global()->bounded(bounds.height()-margin*2)+margin); }
    } else { m_character->setPos(cur.x()+dx/dist*step, cur.y()+dy/dist*step); m_character->advanceWalkPhase(); }
}

void MainWindow::onGoEat() {
    if (m_userZoneId < 0) {
        showNoZoneDialog();
        return;
    }
    m_stack->setCurrentWidget(m_mealPage);
}

void MainWindow::showNoZoneDialog() {
    struct NoZoneDialog : QDialog {
        NoZoneDialog(QWidget *parent) : QDialog(parent) {
            setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
            setAttribute(Qt::WA_TranslucentBackground);
            setFixedSize(420, 220);
            setModal(true);

            auto *btn = new SketchyButton(QString::fromUtf8("我知道了"),
                                          C_CARD_SAGE, C_SHADOW_DK, this);
            btn->setFixedSize(110, 42);
            int bx = (width() - btn->width()) / 2;
            btn->move(bx, height() - btn->height() - 28);
            connect(btn, &QPushButton::clicked, this, &QDialog::accept);
        }

    protected:
        void paintEvent(QPaintEvent *) override {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);

            QRectF card(18, 18, width() - 36, height() - 36);
            int seed = 73;

            QRectF shadow = card.translated(2.5, 3.5);
            QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
            p.setBrush(C_SHADOW_DK);
            p.setPen(Qt::NoPen);
            p.drawPath(sp);

            QPainterPath cp = sketchyRect(card, seed, 2.8);
            drawInkWash(&p, cp, C_CREAM, 18);
            drawInkBorder(&p, cp, C_INK, 3, 0.7);

            QPointF lineStart(card.left() + 25, card.top() + 52);
            QPointF lineEnd(card.left() + 25 + 60, card.top() + 52);
            QPen decoPen(C_INK_LIGHT);
            decoPen.setWidthF(1.0);
            decoPen.setCapStyle(Qt::RoundCap);
            p.setPen(decoPen);
            QPainterPath wavy;
            wavy.moveTo(lineStart);
            wavy.cubicTo(lineStart + QPointF(20, -3), lineStart + QPointF(40, 3), lineEnd);
            p.drawPath(wavy);
            decoPen.setWidthF(0.5);
            p.setPen(decoPen);
            p.translate(0.3, 0.4);
            p.drawPath(wavy);
            p.resetTransform();

            QFont f = font();
            f.setPointSize(13);
            f.setBold(true);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
            p.setFont(f);
            p.setPen(C_INK);
            p.drawText(QRectF(card.left() + 25, card.top() + 18, card.width() - 50, 30),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QString::fromUtf8("提示"));

            f.setPointSize(10);
            f.setBold(false);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
            p.setFont(f);
            p.setPen(C_INK_LIGHT);
            p.drawText(QRectF(card.left() + 25, card.top() + 65, card.width() - 50, 70),
                       Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                       QString::fromUtf8("请先在地图上点击选择您所在的位置"));
        }

        void mousePressEvent(QMouseEvent *e) override {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        void mouseMoveEvent(QMouseEvent *e) override {
            if (e->buttons() & Qt::LeftButton)
                move(e->globalPosition().toPoint() - m_dragPos);
        }

    private:
        QPoint m_dragPos;
    };

    NoZoneDialog dlg(this);
    dlg.exec();
}

void MainWindow::onHistory() {
    CalendarWindow *cw = new CalendarWindow(this);
    cw->setAttribute(Qt::WA_DeleteOnClose);
    cw->setRecords(m_dailyRecords);
    cw->exec();
}

void MainWindow::onReview() {
    HistoryWindow *hw = new HistoryWindow(this);
    hw->setAttribute(Qt::WA_DeleteOnClose);
    hw->setWindowTitle(QString::fromUtf8("菜品评价"));

    struct EatenItem {
        Dish dish;
        QString time;// "yyyy-MM-dd HH:mm"
    };
    QVector<EatenItem> items;
    QSet<QString> seenDishKeys;

    // eatingTimes
    for (auto it = m_eatingTimes.begin(); it != m_eatingTimes.end(); ++it) {
        const QString &fullKey = it.key();
        int lastSep = fullKey.lastIndexOf('|');
        if (lastSep < 0) continue;
        int secondSep = fullKey.lastIndexOf('|', lastSep - 1);
        QString dishRestKey;
        if (secondSep >= 0)
            dishRestKey = fullKey.left(lastSep);
        else
            dishRestKey = fullKey;

        QString timeStr = it.value();
        if (timeStr.length() > 16) timeStr = timeStr.left(16);

        for (const auto &d : m_allDishes) {
            if (d.name + "|" + d.restaurant == dishRestKey) {
                items.append({d, timeStr});
                seenDishKeys.insert(dishRestKey);
                break;
            }
        }
    }

    // dailyRecords
    for (auto it = m_dailyRecords.begin(); it != m_dailyRecords.end(); ++it) {
        const QString &date = it.key();
        for (const auto &dishName : it->dishes) {
            bool alreadyInEatingTimes = false;
            for (auto et = m_eatingTimes.begin(); et != m_eatingTimes.end(); ++et) {
                if (et.key().startsWith(dishName + "|")) {
                    alreadyInEatingTimes = true;
                    break;
                }
            }
            if (alreadyInEatingTimes) continue;
            QVector<const Dish*> candidates;
            for (const auto &d : m_allDishes) {
                if (d.name + "|" + d.restaurant == dishName) candidates.append(&d);
            }
            if (candidates.size() == 1) {
                const Dish &d = *candidates.first();
                QString key = d.name + "|" + d.restaurant;
                if (!seenDishKeys.contains(key)) {
                    items.append({d, date + " 12:00"});
                    seenDishKeys.insert(key);
                }
            }
        }
    }

    std::sort(items.begin(), items.end(), [](const EatenItem &a, const EatenItem &b) {
        return a.time > b.time;
    });

    QVector<Dish> eatenDishes;
    QMap<QString, QString> displayDates;
    QMap<int, QString> idxToKey;
    for (int i = 0; i < items.size(); ++i) {
        eatenDishes.append(items[i].dish);
        displayDates[QString::number(i)] = items[i].time;
        idxToKey[i] = items[i].dish.name + "|" + items[i].dish.restaurant;
    }

    hw->loadDishes(eatenDishes, m_userProfile, displayDates);
    connect(hw, &HistoryWindow::dishRated, this, [this, idxToKey](int dishId, int rating) {
        if (idxToKey.contains(dishId)) {
            const QString &key = idxToKey[dishId];
            m_userProfile.ratings[key] = rating;
            m_ratingDates[key] = QDate::currentDate().toString("yyyy-MM-dd");
            saveRatingsToUserFile();
        }
    });
    hw->exec();
}

void MainWindow::onSettings() {
    SettingsDialog dlg(m_userData.settings, this);
    connect(&dlg, &SettingsDialog::bgmVolumeChanged, this, [this](int vol) {
        double v = qBound(0, vol, 100) / 100.0;
        m_bgmOutput->setVolume(v);
        if (vol > 0) m_bgmPlayer->play();
        else m_bgmPlayer->stop();
    });
    if(dlg.exec()==QDialog::Accepted){
        m_userData.settings=dlg.result();
        m_userData.save("user.json");
        applyUserSettings();
        int vol = qBound(0, m_userData.settings.bgmVolume, 100);
        m_bgmOutput->setVolume(vol / 100.0);
        if (vol > 0) m_bgmPlayer->play();
        else m_bgmPlayer->stop();
    }
}

void MainWindow::onManual() {
    ManualWindow dlg(this);
    dlg.exec();
}

void MainWindow::onMealReadyForReview(const QVector<Dish> &selected) {
    if(selected.isEmpty()) return;
    ReviewDialog dlg(selected, m_userZoneId, m_zoneManager, m_distanceDB, m_userData.settings, m_restaurantZoneMap, this);
    if(dlg.exec()==QDialog::Accepted && dlg.isConfirmed()){
        m_currentMealDishes=selected; m_mealActive=true; m_stack->setCurrentWidget(m_mapPage);
        
        if (!selected.isEmpty()) {
            QString restaurant = selected.first().restaurant;
            int zoneId = m_restaurantZoneMap.value(restaurant, -1);
            if (zoneId >= 0) {
                const ZoneInfo *zone = m_zoneManager->zoneInfo(zoneId);
                if (zone && zone->isValid()) {
                    m_targetPos = zone->polygon.boundingRect().center();
                    m_roaming = false;
                }
            }
        }

        double totalCal=0; for(const auto &d:selected) totalCal+=d.calories;
        m_userProfile.todayCalories+=totalCal;
        saveRatingsToUserFile();
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        DailyRecord &rec = m_dailyRecords[today];
        for (const auto &d : selected) rec.dishes.append(d.name + "|" + d.restaurant);
        rec.totalCalories += totalCal;
        rec.totalPrice += 0;
        for (const auto &d : selected) rec.totalPrice += d.price;
        saveDailyRecords();
        // 记录食用时间
        {
            QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            for (const auto &d : selected)
                m_eatingTimes[d.name + "|" + d.restaurant + "|" + now] = now;
            saveEatingTimes();
        }
        updateSidebarUserInfo(); updateLionSprite();

        struct MealDoneDialog : QDialog {
            MealDoneDialog(const QString &text, QWidget *parent) : QDialog(parent) {
                setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
                setAttribute(Qt::WA_TranslucentBackground);
                setFixedSize(420, 260);
                setModal(true);

                QFont titleFont; titleFont.setPointSize(15);
                titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.5);
                titleFont.setWeight(QFont::Bold);
                QFont bodyFont; bodyFont.setPointSize(11);
                bodyFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);

                auto *titleLbl = new QLabel(QString::fromUtf8("开饭啦！"), this);
                titleLbl->setFont(titleFont);
                titleLbl->setStyleSheet(QString("color: %1;").arg(C_INK.name()));
                titleLbl->setAlignment(Qt::AlignCenter);
                titleLbl->setGeometry(0, 50, width(), 34);

                auto *bodyLbl = new QLabel(text, this);
                bodyLbl->setFont(bodyFont);
                bodyLbl->setStyleSheet(QString("color: %1;").arg(C_INK_LIGHT.name()));
                bodyLbl->setAlignment(Qt::AlignCenter);
                bodyLbl->setWordWrap(true);
                bodyLbl->setGeometry(30, 92, width() - 60, 56);

                auto *btn = new SketchyButton(QString::fromUtf8("好的"),
                                              C_CARD_SAGE, C_SHADOW_DK, this);
                btn->setFixedSize(90, 40);
                btn->move((width() - 90) / 2, height() - btn->height() - 24);
                QFont btnFont; btnFont.setPointSize(11);
                btnFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
                btn->setFont(btnFont);
                connect(btn, &QPushButton::clicked, this, &QDialog::accept);
            }
        protected:
            void paintEvent(QPaintEvent *) override {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing);
                QRectF card(18, 18, width() - 36, height() - 36);
                int seed = 57;
                QRectF shadow = card.translated(2.5, 3.5);
                QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
                p.setBrush(C_SHADOW_DK);
                p.setPen(Qt::NoPen);
                p.drawPath(sp);
                QPainterPath cp = sketchyRect(card, seed, 2.8);
                drawInkWash(&p, cp, C_CREAM, 18);
                drawInkBorder(&p, cp, C_INK, 3, 0.7);
            }
        };
        MealDoneDialog doneDlg(QString::fromUtf8("共 %1 道菜，合计 %2 kcal\n祝你用餐愉快！")
            .arg(selected.size()).arg(totalCal,0,'f',0), this);
        doneDlg.exec();

        QTimer::singleShot(500, this, &MainWindow::onFinishEating);
    } else { m_mealPage->resetMeal(); }
}

void MainWindow::onFinishEating() {
    if(!m_mealActive) return;
    m_mealActive=false;
    HistoryWindow *hw = new HistoryWindow(this); hw->setAttribute(Qt::WA_DeleteOnClose);
    hw->setWindowTitle(QString::fromUtf8("为这顿饭打分"));
    QMap<QString, QString> eatingDates;
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
    for (int i = 0; i < m_currentMealDishes.size(); ++i)
        eatingDates[QString::number(i)] = now;
    UserProfile tempProfile = m_userProfile;
    for (const auto &d : m_currentMealDishes) {
        QString k = d.name + "|" + d.restaurant;
        tempProfile.ratings[k] = 0;
    }
    hw->loadDishes(m_currentMealDishes, tempProfile, eatingDates);
    connect(hw, &HistoryWindow::dishRated, this, [this](int dishId, int rating){
        if(dishId>=0 && dishId<m_currentMealDishes.size()) {
            const QString &name = m_currentMealDishes[dishId].name;
            const QString &restaurant = m_currentMealDishes[dishId].restaurant;
            QString key = name + "|" + restaurant;
            m_userProfile.ratings[key]=rating;
            m_ratingDates[key] = QDate::currentDate().toString("yyyy-MM-dd");
            saveRatingsToUserFile();
        }
    });

    if (m_achievementManager) {
        double bmr = 2000;
        const auto &s = m_userData.settings;
        if (s.height > 0 && s.weight > 0 && s.age > 0 && !s.gender.isEmpty()) {
            if (s.gender == QString::fromUtf8("女"))
                bmr = 10.0 * s.weight + 6.25 * s.height - 5.0 * s.age - 161;
            else
                bmr = 10.0 * s.weight + 6.25 * s.height - 5.0 * s.age + 5;
        }

        double mealPrice = 0;
        for (const auto &d : m_currentMealDishes) mealPrice += d.price;

        QMap<QString, double> dishPrices;
        for (const auto &d : m_allDishes)
            dishPrices[d.name + "|" + d.restaurant] = d.price;

        m_achievementManager->checkAll(
            QDateTime::currentDateTime(),
            bmr,
            m_dailyRecords,
            mealPrice,
            m_eatingTimes,
            dishPrices);

        m_sidebar->setAchievementDot(m_achievementManager->hasNewAchievements());
    }

    hw->exec();
    m_mealPage->resetMeal(); m_currentMealDishes.clear(); m_stack->setCurrentWidget(m_mapPage);
}

void MainWindow::backFromMeal() { m_mealPage->resetMeal(); m_stack->setCurrentWidget(m_mapPage); }

void MainWindow::onSceneLeftClicked(QPointF scenePos) {
    int zoneId = m_zoneManager->zoneAtPoint(scenePos);
    if(zoneId>=0){
        m_targetPos=scenePos; m_roaming=false;
        m_selectedZoneId=zoneId; m_userZoneId=zoneId;
    } else {
        m_selectedZoneId=-1;
        QPoint pos = QCursor::pos();
        QTimer::singleShot(0, this, [this, pos]() {
            QToolTip::showText(pos, QString::fromUtf8("未识别区域"), m_mapView, QRect(), 2000);
        });
    }
}

void MainWindow::applyUserSettings() {
    QPixmap avatar(m_userData.settings.avatarPath); m_sidebar->setAvatar(avatar);
    m_sidebar->setUserName(m_userData.settings.name);
    const auto &s = m_userData.settings;
    if(s.height>0 && s.weight>0 && s.age>0 && !s.gender.isEmpty()){
        int bmr;
        if(s.gender==QString::fromUtf8("女"))
            bmr=static_cast<int>((10.0*s.weight+6.25*s.height-5.0*s.age-161)*1.35);
        else bmr=static_cast<int>((10.0*s.weight+6.25*s.height-5.0*s.age+5)*1.35);
        m_sidebar->setBMR(bmr);
    }
    updateSidebarUserInfo();
}

void MainWindow::updateSidebarUserInfo() {
    m_sidebar->setTodayCalories(static_cast<int>(m_userProfile.todayCalories));
    updateLionSprite();
}

void MainWindow::updateLionSprite() {
    const auto &s = m_userData.settings;
    double bmr = 2000;
    if (s.height > 0 && s.weight > 0 && s.age > 0 && !s.gender.isEmpty()) {
        if (s.gender == QString::fromUtf8("女"))
            bmr = 10.0 * s.weight + 6.25 * s.height - 5.0 * s.age - 161;
        else
            bmr = 10.0 * s.weight + 6.25 * s.height - 5.0 * s.age + 5;
    }
    m_isObese = (m_userProfile.todayCalories > bmr * 1.35 + 300);
    QString skinKey = m_achievementManager ? m_achievementManager->activeSkin() : "first_record";
    QString path = "lion_" + skinKey + (m_isObese ? "_obese.png" : "_slim.png");
    m_character->setSprite(path);
}

void MainWindow::onToggleEditMode() { m_editMode=!m_editMode; m_zoneEditor->setEditMode(m_editMode); }

void MainWindow::saveRatingsToUserFile() {
    QFile f("user.json");
    QJsonObject obj;
    if (f.open(QIODevice::ReadOnly)) {
        obj = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    }
    QJsonObject rObj;
    for (auto it = m_userProfile.ratings.begin(); it != m_userProfile.ratings.end(); ++it)
        rObj[it.key()] = it.value();
    obj["ratings"] = rObj;
    obj["todayCalories"] = m_userProfile.todayCalories;
    obj["calorieDate"] = QDate::currentDate().toString("yyyy-MM-dd");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        f.close();
    }
    
    {
        QFile fd("rating_dates.json");
        if (fd.open(QIODevice::WriteOnly)) {
            QJsonObject dObj;
            for (auto it = m_ratingDates.begin(); it != m_ratingDates.end(); ++it)
                dObj[it.key()] = it.value();
            fd.write(QJsonDocument(dObj).toJson(QJsonDocument::Indented));
            fd.close();
        }
    }
}

void MainWindow::loadDailyRecords() {
    QFile file("daily_records.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QJsonObject robj = it.value().toObject();
        DailyRecord rec;
        for (const auto &v : robj["dishes"].toArray())
            rec.dishes.append(v.toString());
        rec.totalCalories = robj["calories"].toDouble();
        rec.totalPrice = robj["price"].toDouble();
        m_dailyRecords[it.key()] = rec;
    }
}

void MainWindow::saveDailyRecords() {
    QJsonObject obj;
    for (auto it = m_dailyRecords.begin(); it != m_dailyRecords.end(); ++it) {
        QJsonObject robj;
        QJsonArray arr;
        for (const auto &d : it.value().dishes) arr.append(d);
        robj["dishes"] = arr;
        robj["calories"] = it.value().totalCalories;
        robj["price"] = it.value().totalPrice;
        obj[it.key()] = robj;
    }
    QFile file("daily_records.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void MainWindow::loadEatingTimes() {
    QFile f("eating_times.json");
    if (f.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        for (auto it = obj.begin(); it != obj.end(); ++it)
            m_eatingTimes[it.key()] = it.value().toString();
        f.close();
    }
}

void MainWindow::saveEatingTimes() {
    QFile f("eating_times.json");
    if (f.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        for (auto it = m_eatingTimes.begin(); it != m_eatingTimes.end(); ++it)
            obj[it.key()] = it.value();
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        f.close();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if(event->key()==Qt::Key_E && event->modifiers()==Qt::ControlModifier){ onToggleEditMode(); return; }
    if(m_stack->currentWidget()==m_mapPage && (event->modifiers() & Qt::ControlModifier)) {
        if(event->key()==Qt::Key_Equal || event->key()==Qt::Key_Plus) { m_mapView->scale(1.15,1.15); return; }
        if(event->key()==Qt::Key_Minus) { m_mapView->scale(1.0/1.15,1.0/1.15); return; }
        if(event->key()==Qt::Key_0) {
            m_mapView->resetTransform();
            double s = m_mapView->viewport()->height() / m_mapScene->sceneRect().height();
            m_mapView->scale(s,s);
            m_mapView->centerOn(m_mapScene->sceneRect().center());
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}
