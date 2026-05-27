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

void SearchWidget::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // --- 搜索栏 ---
    auto *searchRow = new QHBoxLayout;
    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText("搜索菜名、食堂、标签...（如：面食、辣的、便宜）");
    m_searchInput->setMinimumHeight(48);
    m_searchInput->setStyleSheet("font-size: 16px; padding: 4px 12px;");
    m_searchBtn = new QPushButton("搜索");
    m_searchBtn->setMinimumHeight(48);
    m_searchBtn->setStyleSheet(
        "QPushButton{font-size: 16px; background:#e8c97a; color:white; font-weight:bold;"
        "border-radius: 10px; padding: 4px 20px;}"
        "QPushButton:hover{background:#d4a040;}");
    searchRow->addWidget(m_searchInput, 1);
    searchRow->addWidget(m_searchBtn);
    mainLayout->addLayout(searchRow);

    // --- 标签条 ---
    m_tagChipArea = new QWidget;
    m_tagChipLayout = new QHBoxLayout(m_tagChipArea);
    m_tagChipLayout->setContentsMargins(0, 0, 0, 0);
    m_tagChipLayout->setSpacing(4);
    m_tagChipLayout->addStretch();
    mainLayout->addWidget(m_tagChipArea);

    // --- 状态 ---
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #888; font-size: 14px; padding: 4px 0;");
    mainLayout->addWidget(m_statusLabel);

    // --- 结果列表 ---
    m_resultList = new QListWidget;
    m_resultList->setAlternatingRowColors(true);
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
        auto *chip = new QPushButton(tag + " ×");
        chip->setStyleSheet(
            "QPushButton{background:#ffe0b2;color:#e65100;border:none;"
            "border-radius:14px;padding:6px 14px;font-size:15px;}"
            "QPushButton:hover{background:#ffcc80;}");
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
        m_statusLabel->setText("标签已清空");
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
            if (r.score > 0.5) text += " [匹配]";
            m_resultList->addItem(text);
        }
        m_statusLabel->setText("匹配 " + QString::number(m_lastResults.size()) + " 道菜");
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
        if (r.score > 0.5) text += " [匹配]";
        m_resultList->addItem(text);
    }

    rebuildTagChips();

    QStringList tags = m_fuzzy.tempTags();
    m_statusLabel->setText("匹配 " + QString::number(m_lastResults.size()) + " 道菜");
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
    m_statusLabel->setText("标签已重置");
    rebuildTagChips();
}
