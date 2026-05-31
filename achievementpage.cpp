#include "achievementpage.h"
#include "achievementmanager.h"
#include "sketchyui.h"
#include "decopainter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QPushButton>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>

static const QColor C_CARD_UNLOCKED("#F2E5C7");
static const QColor C_CARD_LOCKED("#D5CFC4");
static const QColor C_SHADOW("#3A3530");

// ── AchievementCard methods ──

AchievementCard::AchievementCard(const AchievementDef &def, const AchievementState &state,
                                 bool isNew, bool isActive, QWidget *parent)
    : QWidget(parent), m_key(def.key), m_unlocked(state.unlocked)
{
    setFixedHeight(160);
    setCursor(m_unlocked ? Qt::PointingHandCursor : Qt::ArrowCursor);

    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 8, 10, 8);
    lay->setSpacing(10);

    // 皮肤缩略图
    auto *thumb = new QLabel;
    thumb->setFixedSize(80, 80);
    thumb->setAlignment(Qt::AlignCenter);
    QPixmap pm("lion_" + def.skinSuffix + "_slim.png");
    if (!pm.isNull())
        thumb->setPixmap(pm.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lay->addWidget(thumb);

    // 文字信息
    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(2);

    auto *nameLbl = new QLabel(def.name);
    QFont nf;
    nf.setPointSize(22);
    nf.setBold(true);
    nf.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    nameLbl->setFont(nf);
    nameLbl->setStyleSheet(m_unlocked ? "color:#2B2B2B;" : "color:#A09890;");
    textCol->addWidget(nameLbl);

    auto *descLbl = new QLabel(def.description);
    descLbl->setStyleSheet("color:#9A9590; font-size:20px;");
    descLbl->setWordWrap(true);
    textCol->addWidget(descLbl);

    if (!state.unlocked && def.progressMax > 0) {
        auto *prog = new QLabel(
            QString::fromUtf8("进度: %1/%2").arg(state.progress).arg(def.progressMax));
        prog->setStyleSheet("color:#C86A5A; font-size:20px;");
        textCol->addWidget(prog);
    }
    if (state.unlocked && !state.unlockDate.isEmpty()) {
        auto *date = new QLabel(state.unlockDate);
        date->setStyleSheet("color:#7A8B6A; font-size:18px;");
        textCol->addWidget(date);
    }
    textCol->addStretch();
    lay->addLayout(textCol, 1);

    // 角标列
    auto *badgeCol = new QVBoxLayout;
    badgeCol->setAlignment(Qt::AlignTop);
    if (isNew) {
        auto *dot = new QLabel;
        dot->setFixedSize(10, 10);
        dot->setStyleSheet("background:#D45A3A; border-radius:5px;");
        badgeCol->addWidget(dot);
    } else if (!m_unlocked) {
        auto *lock = new QLabel(QString::fromUtf8("\xF0\x9F\x94\x92"));
        lock->setStyleSheet("font-size:28px;");
        badgeCol->addWidget(lock);
    }
    if (isActive) {
        auto *chk = new QLabel(QString::fromUtf8("\xE2\x9C\x93"));
        chk->setStyleSheet("font-size:28px; color:#5A8A5A; font-weight:bold;");
        badgeCol->addWidget(chk);
    }
    badgeCol->addStretch();
    lay->addLayout(badgeCol);
}

void AchievementCard::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r = rect().adjusted(2, 2, -2, -2);
    QPainterPath sp = sketchyRect(r.translated(2, 3), 42, 2.5);
    p.setBrush(C_SHADOW);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);
    QPainterPath cp = sketchyRect(r, 42, 2.5);
    drawInkWash(&p, cp, m_unlocked ? C_CARD_UNLOCKED : C_CARD_LOCKED, 12);
    drawInkBorder(&p, cp, QColor("#2B2B2B"), 2, 0.5);
}

void AchievementCard::mousePressEvent(QMouseEvent *e) {
    if (m_unlocked) emit clicked(m_key);
    QWidget::mousePressEvent(e);
}

// ── AchievementPage ──

AchievementPage::AchievementPage(AchievementManager *mgr, QWidget *parent)
    : QWidget(parent), m_mgr(mgr)
{
    buildUI();
}

void AchievementPage::buildUI() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(14);

    // 标题
    auto *title = new QLabel(QString::fromUtf8("干饭勋章"));
    QFont tf;
    tf.setPointSize(36);
    tf.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    title->setFont(tf);
    title->setStyleSheet("color:#2B2B2B;");
    title->setAlignment(Qt::AlignCenter);
    outer->addWidget(title);

    // 当前皮肤展示
    auto *skinRow = new QHBoxLayout;
    skinRow->setSpacing(14);
    m_currentSkinPreview = new QLabel;
    m_currentSkinPreview->setFixedSize(100, 100);
    m_currentSkinPreview->setAlignment(Qt::AlignCenter);
    skinRow->addWidget(m_currentSkinPreview);

    auto *skinInfo = new QVBoxLayout;
    skinInfo->setSpacing(4);
    m_currentSkinName = new QLabel;
    QFont snf;
    snf.setPointSize(26);
    snf.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    m_currentSkinName->setFont(snf);
    m_currentSkinName->setStyleSheet("color:#2B2B2B;");
    skinInfo->addWidget(m_currentSkinName);
    auto *hint = new QLabel(QString::fromUtf8("地图上的小狮子会穿上这套装扮"));
    hint->setStyleSheet("color:#9A9590; font-size:22px;");
    skinInfo->addWidget(hint);
    skinInfo->addStretch();
    skinRow->addLayout(skinInfo);
    skinRow->addStretch();
    outer->addLayout(skinRow);

    // 分隔线
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("QFrame { color: #C4BDB3; }");
    outer->addWidget(sep);

    // 可滚动的成就卡片网格
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; }");

    m_gridContainer = new QWidget;
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setSpacing(12);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    scroll->setWidget(m_gridContainer);
    outer->addWidget(scroll, 1);

    // 返回按钮（使用现有 SketchyButton 风格）
    auto *backBtn = new SketchyButton(QString::fromUtf8("返回地图"),
                                      QColor("#D3D9CA"), C_SHADOW);
    backBtn->setFixedHeight(48);
    QFont bf;
    bf.setPointSize(24);
    bf.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    backBtn->setFont(bf);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_mgr->clearNewlyUnlocked();
        emit backToMap();
    });
    outer->addWidget(backBtn);
}

void AchievementPage::refresh() {
    const auto &defs = m_mgr->defs();
    const QString &activeSkin = m_mgr->activeSkin();
    const QStringList newly = m_mgr->newlyUnlocked();

    // 刷新当前皮肤大图
    QPixmap activePm("lion_" + activeSkin + "_slim.png");
    if (!activePm.isNull())
        m_currentSkinPreview->setPixmap(
            activePm.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QString displayName = activeSkin;
    for (const auto &d : defs) {
        if (d.key == activeSkin) { displayName = d.name; break; }
    }
    m_currentSkinName->setText(QString::fromUtf8("当前: %1").arg(displayName));

    // 清除旧卡片
    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // 重建所有卡片
    for (int i = 0; i < defs.size(); ++i) {
        const auto &d = defs[i];
        const auto &st = m_mgr->state(d.key);
        bool isNew = newly.contains(d.key);
        bool isActive = (d.key == activeSkin);
        auto *card = new AchievementCard(d, st, isNew, isActive);
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

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("切换皮肤"));
    msgBox.setText(QString::fromUtf8("确定要将小狮子切换为\n「%1」吗？").arg(name));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(QString::fromUtf8("确定"));
    msgBox.button(QMessageBox::No)->setText(QString::fromUtf8("取消"));

    if (msgBox.exec() == QMessageBox::Yes) {
        m_mgr->setActiveSkin(key);
        refresh();
    }
}

void AchievementPage::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawInkWash(&p, sketchyRect(rect().adjusted(0, 0, 4, 0), 13, 4.0),
                QColor("#FDFBF7"), 12);
}
