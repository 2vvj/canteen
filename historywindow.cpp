#include "historywindow.h"
#include "calendarwindow.h"
#include "decopainter.h"
#include "sketchyui.h"
#include "dishdata.h"
#include <QFrame>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QMouseEvent>
#include <QTimer>

// 艺术市集色板 — 参考主界面侧边栏按钮配色
static const QList<QColor> kCardColors = {
    QColor("#DCE4D3"), QColor("#D0DDE8"), QColor("#EAD7D2"),
    QColor("#E0D7CC"), QColor("#DDE8D0"), QColor("#D5E0E8"),
    QColor("#EDDCD8"), QColor("#E4DCD0"),
};

static const QColor C_CREAM  = QColor("#FDFBF7");
static const QColor C_INK    = QColor("#2B2B2B");
static const QColor C_SHADOW = QColor("#3A3530");

// ========== StarRatingWidget ==========
StarRatingWidget::StarRatingWidget(int maxStars, QWidget *parent)
    : QWidget(parent), m_maxStars(maxStars), m_rating(0),
      m_pendingRating(0), m_previewing(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    for (int i = 0; i < m_maxStars; ++i) {
        QPushButton *btn = new QPushButton(QString::fromUtf8("☆"), this);
        btn->setStyleSheet(
            "QPushButton { border: none; background: transparent;"
            "  color: #8B8580; font-size: 26px; padding: 2px; }"
            "QPushButton:hover { color: #4A4540; }");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("starIndex", i + 1);
        connect(btn, &QPushButton::clicked, this, &StarRatingWidget::onStarClicked);
        starButtons.append(btn);
        layout->addWidget(btn);
    }
    layout->addStretch();
}

void StarRatingWidget::onStarClicked() {
    if (m_locked) return;
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) return;
    int proposedRating = clickedBtn->property("starIndex").toInt();
    if (proposedRating == m_rating && !m_previewing) return;
    m_pendingRating = proposedRating;
    m_previewing = true;
    updateStarUIPreview(proposedRating);
    emit ratingRequested(proposedRating);
}

void StarRatingWidget::confirmRating(int rating) {
    m_rating = rating;
    m_previewing = false;
    m_pendingRating = 0;
    updateStarUI();
    emit ratingChanged(rating);
}

void StarRatingWidget::cancelPreview() {
    m_previewing = false;
    m_pendingRating = 0;
    updateStarUI();
}

void StarRatingWidget::flashEffect() {
    QString flashStyle =
        "StarRatingWidget { background-color: #EDE4D8;"
        "  border: 2px solid #4A4540; border-radius: 10px; }";
    QString normalStyle;
    setStyleSheet(flashStyle);
    QTimer::singleShot(150, this, [this, normalStyle, flashStyle]() {
        setStyleSheet(normalStyle);
        QTimer::singleShot(120, this, [this, flashStyle, normalStyle]() {
            setStyleSheet(flashStyle);
            QTimer::singleShot(150, this, [this, normalStyle, flashStyle]() {
                setStyleSheet(normalStyle);
                QTimer::singleShot(120, this, [this, flashStyle, normalStyle]() {
                    setStyleSheet(flashStyle);
                    QTimer::singleShot(180, this, [this, normalStyle]() {
                        setStyleSheet(normalStyle);
                    });
                });
            });
        });
    });
}

void StarRatingWidget::setLocked(bool locked) {
    m_locked = locked;
    for (auto *btn : starButtons) {
        btn->setEnabled(!locked);
        btn->setCursor(locked ? Qt::ArrowCursor : Qt::PointingHandCursor);
    }
    updateStarUI();
}

void StarRatingWidget::setRating(int rating) {
    if (m_locked && rating != m_rating) return;
    if (rating < 0) rating = 0;
    if (rating > m_maxStars) rating = m_maxStars;
    m_rating = rating;
    m_previewing = false;
    updateStarUI();
}

void StarRatingWidget::updateStarUI() {
    for (int i = 0; i < m_maxStars; ++i) {
        if (i < m_rating) {
            starButtons[i]->setText(QString::fromUtf8("★"));
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #4A4540; font-size: 26px; padding: 2px; }"
                "QPushButton:hover { color: #2B2B2B; }");
        } else {
            starButtons[i]->setText(QString::fromUtf8("☆"));
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #8B8580; font-size: 26px; padding: 2px; }"
                "QPushButton:hover { color: #4A4540; }");
        }
    }
}

void StarRatingWidget::updateStarUIPreview(int previewRating) {
    for (int i = 0; i < m_maxStars; ++i) {
        if (i < previewRating) {
            starButtons[i]->setText(QString::fromUtf8("★"));
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #4A4540; font-size: 26px; padding: 2px; }"
                "QPushButton:hover { color: #2B2B2B; }");
        } else {
            starButtons[i]->setText(QString::fromUtf8("☆"));
            starButtons[i]->setStyleSheet(
                "QPushButton { border: none; background: transparent;"
                "  color: #8B8580; font-size: 26px; padding: 2px; }"
                "QPushButton:hover { color: #4A4540; }");
        }
    }
}

// ========== HistoryItemWidget ==========
HistoryItemWidget::HistoryItemWidget(int dishId, const QString &dishName,
                                     const QString &dateStr, const QString &canteenName,
                                     const QColor &cardColor,
                                     int initialRating, bool locked, QWidget *parent)
    : QWidget(parent), m_dishId(dishId), m_dishName(dishName), m_cardColor(cardColor)
{
    setStyleSheet("HistoryItemWidget { background: transparent; border: none; }");
    setMinimumHeight(72);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 12, 20, 12);
    mainLayout->setSpacing(14);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    QLabel *nameLabel = new QLabel(dishName, this);
    nameLabel->setStyleSheet(
        "font-size: 17px; font-weight: bold; border: none;"
        "background: transparent; color: #2B2B2B; font-family: 'Microsoft YaHei';");

    QString infoText;
    if (!dateStr.isEmpty() && !canteenName.isEmpty())
        infoText = dateStr + QString::fromUtf8(" · ") + canteenName;
    else if (!dateStr.isEmpty())
        infoText = dateStr;
    else if (!canteenName.isEmpty())
        infoText = canteenName;
    else
        infoText = QString::fromUtf8("未知");

    QLabel *infoLabel = new QLabel(infoText, this);
    infoLabel->setStyleSheet(
        "font-size: 12px; border: none; background: transparent;"
        "color: #8B8B8B; font-family: 'Microsoft YaHei';");

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(infoLabel);
    mainLayout->addLayout(infoLayout, 1);

    m_starWidget = new StarRatingWidget(5, this);
    m_starWidget->setRating(initialRating);
    if (locked || initialRating > 0)
        m_starWidget->setLocked(true);
    connect(m_starWidget, &StarRatingWidget::ratingRequested,
            this, &HistoryItemWidget::onRatingRequested);
    mainLayout->addWidget(m_starWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
}

void HistoryItemWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QRectF r = rect().adjusted(2, 2, -2, -2);

    // 卡片底色
    p.setBrush(m_cardColor);
    p.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeWavyRect(r, 2.0f);
    p.drawPath(bgPath);

    // 手绘墨水边框
    QColor ink(43, 43, 43, 90);
    DecoPainter::drawSketchyBorder(&p, bgPath, ink, 2, 1.0f);

    // 左侧手绘色条
    float barW = 4.0f;
    QRectF barR(r.x() + 7, r.y() + r.height() * 0.15f, barW, r.height() * 0.7f);
    p.setBrush(m_cardColor.darker(125));
    p.setPen(Qt::NoPen);
    QPainterPath barPath = DecoPainter::makeWavyRect(barR, 0.9f);
    p.drawPath(barPath);

    // 淡色水彩晕染
    DecoPainter::drawWatercolorSplotch(p, QPointF(r.right() - 18, r.y() + 14), 8,
                                       QColor(250, 235, 215, 18));

    p.end();
    QWidget::paintEvent(event);
}

void HistoryItemWidget::onRatingRequested(int proposedRating) {
    QString starStr;
    for (int i = 0; i < proposedRating; ++i)
        starStr += QString::fromUtf8("★ ");
    for (int i = proposedRating; i < 5; ++i)
        starStr += QString::fromUtf8("☆ ");

    struct ConfirmDialog : QDialog {
        bool confirmed = false;
        QPoint dragPos;

        ConfirmDialog(const QString &dishName, int rating, const QString &stars,
                      QWidget *parent)
            : QDialog(parent)
        {
            setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
            setAttribute(Qt::WA_TranslucentBackground);
            setFixedSize(380, 230);
            setModal(true);

            QVBoxLayout *lay = new QVBoxLayout(this);
            lay->setContentsMargins(40, 36, 40, 24);
            lay->setSpacing(12);

            QLabel *titleLbl = new QLabel(QString::fromUtf8("确认评分"), this);
            titleLbl->setAlignment(Qt::AlignCenter);
            titleLbl->setStyleSheet(
                "font-size: 17px; font-weight: bold; color: #2B2B2B;"
                "border: none; background: transparent;"
                "font-family: 'Microsoft YaHei';");
            lay->addWidget(titleLbl);

            QLabel *textLbl = new QLabel(this);
            textLbl->setText(QString::fromUtf8(
                "确认给 <b>%1</b> 打 %2 颗星吗？")
                .arg(dishName).arg(rating));
            textLbl->setAlignment(Qt::AlignCenter);
            textLbl->setWordWrap(true);
            textLbl->setStyleSheet(
                "font-size: 14px; color: #4A4540;"
                "border: none; background: transparent;"
                "font-family: 'Microsoft YaHei';");
            lay->addWidget(textLbl);

            QLabel *starLbl = new QLabel(stars, this);
            starLbl->setAlignment(Qt::AlignCenter);
            starLbl->setStyleSheet(
                "font-size: 28px; color: #4A4540; letter-spacing: 5px;"
                "border: none; background: transparent;");
            lay->addWidget(starLbl);

            lay->addStretch();

            QHBoxLayout *btnRow = new QHBoxLayout;
            btnRow->addStretch();
            SketchyButton *confirmBtn = new SketchyButton(
                QString::fromUtf8("确认"), QColor("#E0D7CC"),
                QColor("#3A3530"), this);
            confirmBtn->setFixedSize(90, 38);
            SketchyButton *cancelBtn = new SketchyButton(
                QString::fromUtf8("取消"), QColor("#E0D7CC"),
                QColor("#3A3530"), this);
            cancelBtn->setFixedSize(90, 38);
            connect(confirmBtn, &QPushButton::clicked, this,
                    [this]() { confirmed = true; accept(); });
            connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
            btnRow->addWidget(confirmBtn);
            btnRow->addWidget(cancelBtn);
            btnRow->addStretch();
            lay->addLayout(btnRow);
        }

    protected:
        void paintEvent(QPaintEvent *) override {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            QRectF card(18, 18, width() - 36, height() - 36);
            int seed = 53;
            QRectF shadow = card.translated(2.5, 3.5);
            QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
            p.setBrush(QColor("#3A3530"));
            p.setPen(Qt::NoPen);
            p.drawPath(sp);
            QPainterPath cp = sketchyRect(card, seed, 2.8);
            drawInkWash(&p, cp, QColor("#FDFBF7"), 18);
            drawInkBorder(&p, cp, QColor("#2B2B2B"), 3, 0.7);
        }

        void mousePressEvent(QMouseEvent *e) override {
            dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        void mouseMoveEvent(QMouseEvent *e) override {
            if (e->buttons() & Qt::LeftButton)
                move(e->globalPosition().toPoint() - dragPos);
        }
    };

    ConfirmDialog dlg(m_dishName, proposedRating, starStr.trimmed(), this);
    if (dlg.exec() == QDialog::Accepted && dlg.confirmed) {
        m_starWidget->confirmRating(proposedRating);
        m_starWidget->flashEffect();
        emit dishRated(m_dishId, proposedRating);
    } else {
        m_starWidget->cancelPreview();
    }
}

// ========== HistoryWindow ==========
HistoryWindow::HistoryWindow(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(500, 640);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 36, 36, 28);
    mainLayout->setSpacing(10);

    // ── 标题 — 居中 ──
    QLabel *titleLabel = new QLabel(QString::fromUtf8("干 饭 档 案"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 20px; font-weight: bold; color: #2B2B2B;"
        "letter-spacing: 10px; font-family: 'Microsoft YaHei';"
        "border: none; background: transparent;");
    mainLayout->addWidget(titleLabel);

    // ── 关闭按钮 — SketchyButton，匹配设置界面风格 ──
    m_closeBtn = new SketchyButton(QString::fromUtf8("×"),
        QColor("#E0D7CC"), C_SHADOW, this);
    m_closeBtn->setFixedSize(36, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #2B2B2B;"
        "font-family: 'Microsoft YaHei';");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    m_closeBtn->move(width() - 36 - 36, 30);

    // ── 副标题 ──
    QLabel *subTitleLabel = new QLabel(QString::fromUtf8("点击星星为你吃过的菜打分"), this);
    subTitleLabel->setAlignment(Qt::AlignCenter);
    subTitleLabel->setStyleSheet(
        "font-size: 13px; color: #8B8B8B;"
        "font-family: 'Microsoft YaHei';"
        "border: none; background: transparent;");
    mainLayout->addWidget(subTitleLabel);

    mainLayout->addSpacing(4);

    // ── 手绘分隔线 ──
    mainLayout->addWidget(new ScratchyDivider(this));

    // ── 滚动区域 ──
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical {"
        "  background: #E8E0D8; width: 8px; border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #C8BAB0; border-radius: 4px; min-height: 40px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}");

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("background: transparent;");
    listLayout = new QVBoxLayout(scrollWidget);
    listLayout->setSpacing(14);
    listLayout->setContentsMargins(8, 10, 8, 10);
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);
}

void HistoryWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 73;

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

void HistoryWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QWidget *child = childAt(e->pos());
        if (!child || child == this) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
        }
    }
    QDialog::mousePressEvent(e);
}

void HistoryWindow::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
        if (delta.manhattanLength() > 4)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    QDialog::mouseMoveEvent(e);
}

void HistoryWindow::mouseReleaseEvent(QMouseEvent *e) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}

void HistoryWindow::loadDishes(const QVector<Dish> &dishes, const UserProfile &user,
                                const QMap<QString, QString> &eatingDates) {
    QLayoutItem *child;
    while ((child = listLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    int colorIndex = 0;
    for (int i = 0; i < dishes.size(); ++i) {
        const auto &d = dishes[i];
        QString key = d.name + "|" + d.restaurant;
        int rating = static_cast<int>(user.ratings.value(key, 0));
        QString dateStr = eatingDates.value(key);
        bool locked = (rating > 0);
        QColor cardColor = kCardColors[colorIndex % kCardColors.size()];
        colorIndex++;

        HistoryItemWidget *itemWidget = new HistoryItemWidget(
            i, d.name, dateStr, d.restaurant, cardColor, rating, locked, this);
        listLayout->addWidget(itemWidget);

        connect(itemWidget, &HistoryItemWidget::dishRated, this,
                [this](int dishId, int r) {
            emit dishRated(dishId, r);
        });
    }

    if (dishes.isEmpty()) {
        QLabel *emptyLabel = new QLabel(
            QString::fromUtf8("还没有菜品数据"), this);
        emptyLabel->setStyleSheet(
            "color: #8B8B8B; font-size: 14px; padding: 40px 0;"
            "font-family: 'Microsoft YaHei';");
        emptyLabel->setAlignment(Qt::AlignCenter);
        listLayout->addWidget(emptyLabel);
    }
    listLayout->addStretch();
}
