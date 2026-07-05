#include "achievementpage.h"
#include "achievementmanager.h"
#include "sketchyui.h"
#include "decopainter.h"
#include "calendarwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>

static const QColor C_CARD_UNLOCKED("#FDFBF7");
static const QColor C_CARD_ACTIVE("#FBF2DD");
static const QColor C_CARD_LOCKED("#E8E4DE");
static const QColor C_SHADOW("#3A3530");
static const QColor C_INK("#2B2B2B");

// 皮肤切换确弹窗
class SkinConfirmDialog : public QDialog {
public:
    SkinConfirmDialog(const QString &skinName, QWidget *parent = nullptr)
        : QDialog(parent), m_result(false)
    {
        setWindowTitle(QString::fromUtf8("切换皮肤"));
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(380, 200);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(36, 36, 36, 28);
        mainLayout->setSpacing(14);

        auto *titleLabel = new QLabel(QString::fromUtf8("切 换 皮 肤"), this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet(
            "font-size:18px; font-weight:bold; color:#2B2B2B;"
            "letter-spacing:6px; font-family:'Microsoft YaHei';"
            "border:none; background:transparent;");
        mainLayout->addWidget(titleLabel);

        mainLayout->addWidget(new ScratchyDivider(this));

        auto *msgLabel = new QLabel(
            QString::fromUtf8("确定要将小狮子切换为\n「%1」吗？").arg(skinName), this);
        msgLabel->setAlignment(Qt::AlignCenter);
        msgLabel->setStyleSheet(
            "font-size:15px; color:#5D4B3A; "
            "font-family:'Microsoft YaHei'; border:none; background:transparent;"
            "line-height: 1.5;");
        msgLabel->setWordWrap(true);
        mainLayout->addWidget(msgLabel);

        mainLayout->addSpacing(4);

        auto *btnRow = new QHBoxLayout;
        btnRow->addStretch();

        auto *yesBtn = new SketchyButton(QString::fromUtf8("确定"),
            QColor("#D3D9CA"), C_SHADOW, this);
        yesBtn->setMinimumSize(90, 38);
        yesBtn->setCursor(Qt::PointingHandCursor);
        yesBtn->setStyleSheet(
            "font-size:15px; font-weight:bold; color:#2B2B2B;"
            "font-family:'Microsoft YaHei';");
        connect(yesBtn, &QPushButton::clicked, this, [this]() {
            m_result = true;
            accept();
        });
        btnRow->addWidget(yesBtn);

        btnRow->addSpacing(20);

        auto *noBtn = new SketchyButton(QString::fromUtf8("取消"),
            QColor("#E0D7CC"), C_SHADOW, this);
        noBtn->setMinimumSize(90, 38);
        noBtn->setCursor(Qt::PointingHandCursor);
        noBtn->setStyleSheet(
            "font-size:15px; font-weight:bold; color:#2B2B2B;"
            "font-family:'Microsoft YaHei';");
        connect(noBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnRow->addWidget(noBtn);

        btnRow->addStretch();
        mainLayout->addLayout(btnRow);
    }

    bool confirmed() const { return m_result; }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRectF card(12, 12, width() - 24, height() - 24);
        int seed = 47;

        QRectF shadow = card.translated(2.5, 3.5);
        QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
        p.setBrush(C_SHADOW);
        p.setPen(Qt::NoPen);
        p.drawPath(sp);

        QPainterPath cp = sketchyRect(card, seed, 2.8);
        drawInkWash(&p, cp, QColor("#FDFBF7"), 18);
        drawInkBorder(&p, cp, C_INK, 3, 0.7);
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            QWidget *child = childAt(e->pos());
            if (!child || child == this)
                m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (e->buttons() & Qt::LeftButton) {
            QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
            if (delta.manhattanLength() > 4)
                move(e->globalPosition().toPoint() - m_dragPos);
        }
        QDialog::mouseMoveEvent(e);
    }

private:
    bool m_result;
    QPoint m_dragPos;
};

// 皮肤卡片
AchievementCard::AchievementCard(const AchievementDef &def, const AchievementState &state,
                                 bool isNew, bool isActive, bool showObese,
                                 QWidget *parent)
    : QWidget(parent), m_key(def.key), m_unlocked(state.unlocked), m_isActive(isActive),
      m_showObese(showObese)
{
    setFixedHeight(174);
    setCursor(m_unlocked ? Qt::PointingHandCursor : Qt::ArrowCursor);
    setStyleSheet("background: transparent; border: none;");

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(18, 18, 18, 14);
    lay->setSpacing(14);

    // 左侧文字
    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(3);

    auto *nameLbl = new QLabel(def.name);
    nameLbl->setStyleSheet(QString("color:%1; font-size:22px; font-weight:bold; "
                                   "font-family:'Microsoft YaHei'; border:none; background:transparent; "
                                   "letter-spacing:2px;")
                           .arg(m_unlocked ? "#3A3530" : "#A09890"));
    textCol->addWidget(nameLbl);

    auto *descLbl = new QLabel(def.description);
    descLbl->setStyleSheet("color:#8B7D6B; font-size:17px; "
                           "font-family:'Microsoft YaHei'; border:none; background:transparent;");
    descLbl->setWordWrap(true);
    textCol->addWidget(descLbl);

    if (!state.unlocked && def.progressMax > 0) {
        QString progText = QString::fromUtf8("进度 %1/%2").arg(state.progress).arg(def.progressMax);
        auto *progLbl = new QLabel(progText);
        progLbl->setStyleSheet("color:#C86A5A; font-size:16px; font-style:italic; "
                               "font-family:'Microsoft YaHei'; border:none; background:transparent;");
        textCol->addWidget(progLbl);
    }
    if (state.unlocked && !state.unlockDate.isEmpty()) {
        auto *dateLbl = new QLabel(state.unlockDate);
        dateLbl->setStyleSheet("color:#7A8B6A; font-size:16px; "
                               "font-family:'Microsoft YaHei'; border:none; background:transparent;");
        textCol->addWidget(dateLbl);
    }
    textCol->addStretch();
    lay->addLayout(textCol, 1);

    // 右侧缩略图
    const int kThumbW = 100, kThumbH = 100;
    auto *thumb = new QLabel;
    thumb->setFixedSize(kThumbW, kThumbH);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet("background: transparent; border: none;");
    QString suffix = m_showObese ? "_obese.png" : "_slim.png";
    QPixmap pm("lion_" + def.skinSuffix + suffix);
    if (!pm.isNull())
        thumb->setPixmap(pm.scaled(kThumbW, kThumbH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        thumb->setText(QString::fromUtf8("\xF0\x9F\xA6\x81"));
    lay->addWidget(thumb);

    auto *badgeCol = new QVBoxLayout;
    badgeCol->setAlignment(Qt::AlignTop);
    auto *badgePlaceholder = new QWidget;
    badgePlaceholder->setFixedSize(28, 28);
    badgePlaceholder->setStyleSheet("background:transparent;");
    auto *badgeLayout = new QVBoxLayout(badgePlaceholder);
    badgeLayout->setContentsMargins(0, 0, 0, 0);
    badgeLayout->setAlignment(Qt::AlignCenter);

    if (isNew) {
        auto *dot = new QLabel;
        dot->setFixedSize(10, 10);
        dot->setStyleSheet("background:#D45A3A; border-radius:5px;");
        badgeLayout->addWidget(dot);
        badgeLayout->setAlignment(dot, Qt::AlignCenter);
    } else if (!m_unlocked) {
        auto *lockLbl = new QLabel(QString::fromUtf8("\xF0\x9F\x94\x92"));
        lockLbl->setFixedSize(24, 24);
        lockLbl->setAlignment(Qt::AlignCenter);
        lockLbl->setStyleSheet("font-size:16px; border:none; background:transparent;");
        badgeLayout->addWidget(lockLbl);
    }
    badgeCol->addWidget(badgePlaceholder);
    badgeCol->addStretch();
    lay->addLayout(badgeCol);
}

void AchievementCard::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r = rect().adjusted(3, 3, -3, -3);
    int seed = qHash(m_key) & 0xFF;

    // 阴影层
    double shadowOffset = m_isActive ? 3.0 : 2.5;
    double shadowOpacity = m_isActive ? 0.45 : 0.35;
    QPainterPath sp = sketchyRect(r.translated(shadowOffset, shadowOffset + 1), seed + 100, 2.8);
    p.setBrush(C_SHADOW);
    p.setPen(Qt::NoPen);
    p.setOpacity(shadowOpacity);
    p.drawPath(sp);
    p.setOpacity(1.0);

    // 纸张底色
    QColor cardBg = C_CARD_LOCKED;
    if (m_isActive)
        cardBg = C_CARD_ACTIVE;
    else if (m_unlocked)
        cardBg = C_CARD_UNLOCKED;

    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, cardBg);
    bg.setColorAt(0.5, cardBg.darker(103));
    bg.setColorAt(1.0, cardBg.darker(107));
    QPainterPath cp = sketchyRect(r, seed, 2.8);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawPath(cp);

    drawInkWash(&p, cp, cardBg, m_isActive ? 18 : 14);

    QColor accent;
    if (m_isActive)
        accent = QColor("#D4A84C");
    else if (m_unlocked)
        accent = QColor("#C8A86A");
    else
        accent = QColor("#B8B0A8");

    QRectF accentR(r.x() + 16, r.y() + 6, r.width() - 32, 3.5);
    QPainterPath accentPath = DecoPainter::makeWavyRect(accentR, 0.7f);
    p.setBrush(accent);
    p.setPen(Qt::NoPen);
    p.setOpacity(m_isActive ? 0.70 : (m_unlocked ? 0.55 : 0.30));
    p.drawPath(accentPath);
    p.setOpacity(1.0);

    // 边框
    int passes = m_isActive ? 3 : (m_unlocked ? 3 : 2);
    double alpha = m_isActive ? 0.75 : (m_unlocked ? 0.65 : 0.4);
    drawInkBorder(&p, cp, C_INK, passes, alpha);
}

void AchievementCard::mousePressEvent(QMouseEvent *e) {
    if (m_unlocked) emit clicked(m_key);
    QWidget::mousePressEvent(e);
}

// 成就界面
AchievementPage::AchievementPage(AchievementManager *mgr, QWidget *parent)
    : QWidget(parent), m_mgr(mgr)
{
    buildUI();
}

void AchievementPage::buildUI() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(36, 32, 36, 24);
    outer->setSpacing(14);

    auto *title = new QLabel(QString::fromUtf8("干 饭 勋 章"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size:24px; font-weight:bold; color:#2B2B2B;"
        "letter-spacing:8px; font-family:'Microsoft YaHei';"
        "border:none; background:transparent;");
    outer->addWidget(title);

    outer->addSpacing(4);

    outer->addWidget(new ScratchyDivider(this));

    outer->addSpacing(6);
    auto *skinWrapper = new QHBoxLayout;
    skinWrapper->addStretch();

    m_currentSkinPreview = new QLabel;
    m_currentSkinPreview->setFixedSize(90, 90);
    m_currentSkinPreview->setAlignment(Qt::AlignCenter);
    m_currentSkinPreview->setStyleSheet("border:none; background:transparent;");
    skinWrapper->addWidget(m_currentSkinPreview);

    auto *skinInfo = new QVBoxLayout;
    skinInfo->setSpacing(2);
    m_currentSkinName = new QLabel;
    m_currentSkinName->setStyleSheet(
        "color:#3A3530; font-size:18px; font-weight:bold; "
        "font-family:'Microsoft YaHei'; letter-spacing:2px; "
        "border:none; background:transparent;");
    skinInfo->addWidget(m_currentSkinName);
    auto *hint1 = new QLabel(QString::fromUtf8("点击已解锁的成就，更换相应装扮"));
    hint1->setStyleSheet(
        "color:#9A9590; font-size:14px; "
        "font-family:'Microsoft YaHei'; border:none; background:transparent;");
    skinInfo->addWidget(hint1);
    auto *hint2 = new QLabel(QString::fromUtf8("期待你解锁更多成就！"));
    hint2->setStyleSheet(
        "color:#C8BEB4; font-size:13px; "
        "font-family:'Microsoft YaHei'; border:none; background:transparent;");
    skinInfo->addWidget(hint2);
    skinInfo->addStretch();
    skinWrapper->addLayout(skinInfo);

    skinWrapper->addStretch();
    outer->addLayout(skinWrapper);

    outer->addSpacing(6);
    outer->addWidget(new ScratchyDivider(this));
    outer->addSpacing(2);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px; }"
        "QScrollBar::handle:vertical { background: #C8BAB0; border-radius: 5px; min-height: 36px; }"
        "QScrollBar::handle:vertical:hover { background: #B0A090; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");

    m_gridContainer = new QWidget;
    m_gridContainer->setStyleSheet("background: transparent;");
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setSpacing(14);
    m_gridLayout->setContentsMargins(2, 6, 2, 6);
    scroll->setWidget(m_gridContainer);
    outer->addWidget(scroll, 1);

    auto *backBtn = new SketchyButton(QString::fromUtf8("← 返回地图"),
                                      QColor("#D3D9CA"), C_SHADOW, this);
    backBtn->setMinimumSize(140, 44);
    backBtn->setStyleSheet(
        "font-size:16px; font-weight:bold; color:#2B2B2B;"
        "font-family:'Microsoft YaHei';");
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_mgr->clearNewlyUnlocked();
        emit backToMap();
    });
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(backBtn);
    btnRow->addStretch();
    outer->addLayout(btnRow);
}

// 换肤后刷新
void AchievementPage::refresh() {
    const auto &defs = m_mgr->defs();
    const QString &activeSkin = m_mgr->activeSkin();
    const QStringList newly = m_mgr->newlyUnlocked();

    QString skinSuffix = m_isObese ? "_obese.png" : "_slim.png";
    QPixmap activePm("lion_" + activeSkin + skinSuffix);
    if (!activePm.isNull())
        m_currentSkinPreview->setPixmap(
            activePm.scaled(90, 90, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    QString displayName = activeSkin;
    for (const auto &d : defs) {
        if (d.key == activeSkin) { displayName = d.name; break; }
    }
    m_currentSkinName->setText(QString::fromUtf8("当前: %1").arg(displayName));

    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (int i = 0; i < defs.size(); ++i) {
        const auto &d = defs[i];
        const auto &st = m_mgr->state(d.key);
        bool isNew = newly.contains(d.key);
        bool isActive = (d.key == activeSkin);
        auto *card = new AchievementCard(d, st, isNew, isActive, m_isObese);
        connect(card, &AchievementCard::clicked,
                this, &AchievementPage::onCardClicked);
        m_gridLayout->addWidget(card, i / 2, i % 2);
    }
}

void AchievementPage::onCardClicked(const QString &key) {
    const auto st = m_mgr->state(key);
    if (!st.unlocked) return;

    const auto &defs = m_mgr->defs();
    QString name = key;
    for (const auto &d : defs) {
        if (d.key == key) { name = d.name; break; }
    }

    SkinConfirmDialog dlg(name, this);
    if (dlg.exec() == QDialog::Accepted && dlg.confirmed()) {
        m_mgr->setActiveSkin(key);
        m_mgr->clearNewlyUnlocked();
        refresh();
    }
}

void AchievementPage::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QRectF r = rect();
    int seed = 73;

    QLinearGradient bgGrad(r.topLeft(), r.bottomRight());
    bgGrad.setColorAt(0.0, QColor("#FDFBF7"));
    bgGrad.setColorAt(0.4, QColor("#FAF6F0"));
    bgGrad.setColorAt(0.7, QColor("#F8F3EB"));
    bgGrad.setColorAt(1.0, QColor("#F5EFE6"));
    p.setBrush(bgGrad);
    p.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeOrganicRect(r, 3.0f, 41);
    p.drawPath(bgPath);

    drawInkBorder(&p, bgPath, C_INK, 3, 0.65);

    DecoPainter::drawWatercolorSplotch(p, QPointF(r.width() * 0.85, r.height() * 0.12), 22,
                                       QColor(250, 235, 215, 16));
    DecoPainter::drawWatercolorSplotch(p, QPointF(r.width() * 0.12, r.height() * 0.88), 18,
                                       QColor(208, 227, 239, 14));
}
