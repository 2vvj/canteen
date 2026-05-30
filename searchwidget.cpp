#include "searchwidget.h"
#include <QScrollArea>

SearchWidget::SearchWidget(const QVector<Dish> &allDishes,
                           UserProfile &user,
                           QWidget *parent)
    : QWidget(parent)
    , m_allDishes(allDishes)
    , m_user(user)
{
    m_fuzzy.loadSynonyms("synonyms.json");
    m_fuzzy.buildTagIndex(m_allDishes);
    setupUI();
}

// 与 MealPage 一致的配色
static const QColor C_INK       = QColor("#2B2B2B");
static const QColor C_INK_LIGHT = QColor("#4A4540");
static const QColor C_SHADOW_DK = QColor("#3A3530");
static const QColor C_CARD_TAUPE = QColor("#E0D7CC");

void SearchWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // --- 搜索栏 ---
    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(10);
    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText(QString::fromUtf8("搜索菜名、食堂、标签...（如：面食、辣的、便宜）"));
    m_searchInput->setMinimumHeight(46);
    m_searchInput->setStyleSheet(QString(
        "QLineEdit{font-size:15px; padding:4px 14px;"
        "background:rgba(255,255,255,220); border:2px dashed #C8BFA8;"
        "border-radius:10px; color:%1;}").arg(C_INK.name()));

    m_searchBtn = new SketchyButton(QString::fromUtf8("搜索"), C_CARD_TAUPE, C_SHADOW_DK);
    m_searchBtn->setFixedHeight(46);
    m_searchBtn->setMinimumWidth(90);
    QFont btnFont;
    btnFont.setPointSize(13);
    btnFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    m_searchBtn->setFont(btnFont);
    searchRow->addWidget(m_searchInput, 1);
    searchRow->addWidget(m_searchBtn);
    mainLayout->addLayout(searchRow);

    // --- 标签条 ---
    m_tagChipArea = new QWidget;
    m_tagChipLayout = new QHBoxLayout(m_tagChipArea);
    m_tagChipLayout->setContentsMargins(0, 0, 0, 0);
    m_tagChipLayout->setSpacing(6);
    m_tagChipLayout->addStretch();
    mainLayout->addWidget(m_tagChipArea);

    // --- 状态 ---
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QString("color:%1; font-size:14px; padding:4px 0; background:transparent;").arg(C_INK_LIGHT.name()));
    mainLayout->addWidget(m_statusLabel);

    // --- 结果列表 ---
    m_resultList = new QListWidget;
    m_resultList->setAlternatingRowColors(true);
    m_resultList->setStyleSheet(QString(
        "QListWidget{background:rgba(255,255,255,200); border:2px dashed #C8BFA8;"
        "border-radius:10px; font-size:15px; color:%1; padding:6px;}"
        "QListWidget::item{padding:4px 8px;}"
        "QListWidget::item:hover{background:transparent;}"
        "QListWidget::item:selected{background:#F0E8D8; color:%1;}"
        "QListWidget::item:alternate{background:rgba(255,255,248,180);}").arg(C_INK.name()));
    mainLayout->addWidget(m_resultList, 1);

    connect(m_searchBtn, &QPushButton::clicked, this, &SearchWidget::onSearch);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &SearchWidget::onSearch);
}

void SearchWidget::rebuildTagChips() {
    // 清空旧标签
    QLayoutItem *child;
    while ((child = m_tagChipLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QStringList tags = m_fuzzy.tempTags();
    for (const auto &tag : tags) {
        auto *chip = new QPushButton(tag + QString::fromUtf8(" ×"));
        chip->setStyleSheet(
            "QPushButton{background:#F0E0C8; color:#8B5E3C; border:1px solid #D8C8A8;"
            "border-radius:14px; padding:6px 14px; font-size:14px;}"
            "QPushButton:hover{background:#E8D0B0;}");
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, &SearchWidget::onRemoveTag);
        m_tagChipLayout->addWidget(chip);
    }
    m_tagChipLayout->addStretch();
}

void SearchWidget::onRemoveTag() {
    auto *chip = qobject_cast<QPushButton *>(sender());
    if (!chip) return;

    // 从按钮文字提取标签名（去掉 " ×" 后缀）
    QString tag = chip->text();
    tag.chop(2); // 去掉 " ×"
    m_fuzzy.removeTempTag(tag);

    // 用剩余标签重新搜索
    if (m_fuzzy.tempTags().isEmpty()) {
        m_resultList->clear();
        m_statusLabel->setText(QString::fromUtf8("标签已清空"));
        m_lastResults.clear();
        emit searchResultsChanged({});
    } else {
        // 用剩余标签重新打分（不添加新标签）
        m_lastResults = m_fuzzy.rescoreWithCurrentTags(m_allDishes, m_user);
        m_resultList->clear();
        for (const auto &r : m_lastResults) {
            QString text = QString("%1 — %2  ¥%3  %4kcal")
                               .arg(r.dish.name)
                               .arg(r.dish.restaurant)
                               .arg(r.dish.price, 0, 'f', 1)
                               .arg(r.dish.calories, 0, 'f', 0);
            if (r.score > 0.5) text += QString::fromUtf8(" [匹配]");
            m_resultList->addItem(text);
        }
        m_statusLabel->setText(QString::fromUtf8("匹配 ") + QString::number(m_lastResults.size()) + QString::fromUtf8(" 道菜"));
        emit searchResultsChanged(m_lastResults);
    }

    rebuildTagChips();
    emit tagsChanged(m_fuzzy.tempTags());
}

void SearchWidget::onSearch() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) return;

    m_lastResults = m_fuzzy.search(query, m_allDishes, m_user);

    m_resultList->clear();
    for (const auto &r : m_lastResults) {
        QString text = QString("%1 — %2  ¥%3  %4kcal")
                           .arg(r.dish.name)
                           .arg(r.dish.restaurant)
                           .arg(r.dish.price, 0, 'f', 1)
                           .arg(r.dish.calories, 0, 'f', 0);
        if (r.score > 0.5) text += QString::fromUtf8(" [匹配]");
        m_resultList->addItem(text);
    }

    rebuildTagChips();

    QStringList tags = m_fuzzy.tempTags();
    m_statusLabel->setText(QString::fromUtf8("匹配 ") + QString::number(m_lastResults.size()) + QString::fromUtf8(" 道菜"));
    m_searchInput->clear();

    emit searchResultsChanged(m_lastResults);
    emit tagsChanged(tags);
}

QStringList SearchWidget::activeTags() const {
    return m_fuzzy.tempTags();
}

QVector<SearchResult> SearchWidget::lastResults() const {
    return m_lastResults;
}

void SearchWidget::resetMealSession() {
    m_fuzzy.clearTempTags();
    m_resultList->clear();
    m_lastResults.clear();
    m_statusLabel->setText(QString::fromUtf8("标签已重置"));
    rebuildTagChips();
}
