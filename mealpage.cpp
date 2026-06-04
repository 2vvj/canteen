#include "mealpage.h"
#include "recommend.h"
#include "decopainter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTime>
#include <QPainter>

// ── 配色（与 MainWindow 一致的手绘风色板）─────────────────────────
static const QColor C_CREAM      = QColor("#FDFBF7");
static const QColor C_INK        = QColor("#2B2B2B");
static const QColor C_INK_LIGHT  = QColor("#4A4540");
static const QColor C_SHADOW_DK  = QColor("#3A3530");
static const QColor C_CARD_SAGE  = QColor("#DCE4D3");
static const QColor C_CARD_BLUE  = QColor("#D0DDE8");
static const QColor C_CARD_ROSE  = QColor("#EAD7D2");
static const QColor C_CARD_TAUPE = QColor("#E0D7CC");
static const QColor C_CARD_WHEAT = QColor("#F2E9D4");
static const QColor C_ACCENT     = QColor("#C86A5A");

MealPage::MealPage(const QVector<Dish> &allDishes, UserProfile &user, QWidget *parent)
    : QWidget(parent), m_allDishes(allDishes), m_user(user)
{
    m_phase = isBreakfast() ? BREAKFAST_MAIN : MAIN_PHASE;
    setupUI();
}

void MealPage::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 奶油色纸张背景
    p.fillRect(rect(), C_CREAM);

    // 纸张纹理点
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 55; ++i) {
        int x = QRandomGenerator::global()->bounded(width());
        int y = QRandomGenerator::global()->bounded(height());
        int r = QRandomGenerator::global()->bounded(3);
        QColor dot("#D8D0C8");
        dot.setAlpha(15 + QRandomGenerator::global()->bounded(25));
        p.setBrush(dot);
        p.drawEllipse(QPointF(x, y), r, r);
    }
}

void MealPage::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 20, 28, 28);
    mainLayout->setSpacing(14);

    // 字体
    QFont bodyFont;
    bodyFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);

    // === 顶栏：时段信息（左） + 返回按钮（右） ===
    auto *headerRow = new QHBoxLayout;

    auto *timeBlock = new QVBoxLayout;
    timeBlock->setSpacing(2);
    m_timeLabel = new QLabel;
    m_timeLabel->setStyleSheet(QString("font-size:14px; color:%1; font-weight:bold; background:transparent;").arg(C_INK_LIGHT.name()));
    timeBlock->addWidget(m_timeLabel);

    m_phaseLabel = new QLabel;
    m_phaseLabel->setStyleSheet(QString("font-size:14px; color:%1; font-weight:bold; background:transparent;").arg(C_ACCENT.name()));
    timeBlock->addWidget(m_phaseLabel);
    headerRow->addLayout(timeBlock);
    headerRow->addStretch();

    m_backBtn = new SketchyButton(QString::fromUtf8("← 返回地图"), C_CARD_TAUPE, C_SHADOW_DK);
    m_backBtn->setFixedHeight(38);
    m_backBtn->setMinimumWidth(130);
    QFont backFont;
    backFont.setPointSize(11);
    backFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    m_backBtn->setFont(backFont);
    connect(m_backBtn, &QPushButton::clicked, this, &MealPage::backToMap);
    headerRow->addWidget(m_backBtn);
    mainLayout->addLayout(headerRow);

    // === 搜索 ===
    m_searchWidget = new SearchWidget(m_allDishes, m_user);
    m_searchWidget->setStyleSheet(QString("background:transparent;"));
    mainLayout->addWidget(m_searchWidget);
    connect(m_searchWidget, &SearchWidget::searchResultsChanged, this, &MealPage::onSearchResults);
    connect(m_searchWidget, &SearchWidget::tagsChanged, this, &MealPage::onTagsChanged);

    // === 抽卡按钮（双按钮并排） ===
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(14);

    m_drawLimitedBtn = new SketchyButton(QString::fromUtf8("从搜索结果中抽卡"), C_CARD_ROSE, C_SHADOW_DK);
    m_drawLimitedBtn->setFixedHeight(54);
    m_drawLimitedBtn->setEnabled(false);
    QFont drawFont;
    drawFont.setPointSize(13);
    drawFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    m_drawLimitedBtn->setFont(drawFont);
    btnRow->addWidget(m_drawLimitedBtn);

    m_drawWeightedBtn = new SketchyButton(QString::fromUtf8("加权抽卡（惊喜模式）"), C_CARD_BLUE, C_SHADOW_DK);
    m_drawWeightedBtn->setFixedHeight(54);
    m_drawWeightedBtn->setFont(drawFont);
    btnRow->addWidget(m_drawWeightedBtn);
    mainLayout->addLayout(btnRow);
    connect(m_drawLimitedBtn, &QPushButton::clicked, this, &MealPage::onDrawLimited);
    connect(m_drawWeightedBtn, &QPushButton::clicked, this, &MealPage::onDrawWeighted);

    // === 抽卡结果卡片 ===
    m_resultPanel = new SketchyCard;
    m_resultPanel->setCardColor(QColor("#FFFBF5"));
    m_resultPanel->setVisible(false);
    auto *resultLayout = new QVBoxLayout(m_resultPanel);
    resultLayout->setContentsMargins(20, 18, 20, 18);
    resultLayout->setSpacing(12);

    m_resultDishLabel = new QLabel;
    m_resultDishLabel->setAlignment(Qt::AlignCenter);
    m_resultDishLabel->setWordWrap(true);
    m_resultDishLabel->setStyleSheet(QString("font-size:18px; color:%1; background:transparent;").arg(C_INK.name()));
    resultLayout->addWidget(m_resultDishLabel);

    auto *choiceRow = new QHBoxLayout;
    choiceRow->setSpacing(12);
    m_eatBtn = new SketchyButton(QString::fromUtf8("吃这个！"), C_CARD_SAGE, C_SHADOW_DK);
    m_eatBtn->setFixedHeight(46);
    QFont choiceFont;
    choiceFont.setPointSize(13);
    choiceFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    m_eatBtn->setFont(choiceFont);
    choiceRow->addWidget(m_eatBtn);

    m_swapBtn = new SketchyButton(QString::fromUtf8("换一个"), C_CARD_WHEAT, C_SHADOW_DK);
    m_swapBtn->setFixedHeight(46);
    m_swapBtn->setFont(choiceFont);
    choiceRow->addWidget(m_swapBtn);
    resultLayout->addLayout(choiceRow);

    auto *noDishRow = new QHBoxLayout;
    noDishRow->setSpacing(12);
    m_giveUpBtn = new SketchyButton(QString::fromUtf8("不吃了"), C_CARD_ROSE, C_SHADOW_DK);
    m_giveUpBtn->setFixedHeight(42);
    m_giveUpBtn->setVisible(false);
    QFont noDishFont;
    noDishFont.setPointSize(12);
    noDishFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    m_giveUpBtn->setFont(noDishFont);
    noDishRow->addWidget(m_giveUpBtn);

    m_justEatBtn = new SketchyButton(QString::fromUtf8("就这样吧，直接开饭！"), C_CARD_SAGE, C_SHADOW_DK);
    m_justEatBtn->setFixedHeight(42);
    m_justEatBtn->setVisible(false);
    m_justEatBtn->setFont(noDishFont);
    noDishRow->addWidget(m_justEatBtn);
    resultLayout->addLayout(noDishRow);
    connect(m_giveUpBtn, &QPushButton::clicked, this, [this]() { resetMeal(); emit backToMap(); });
    connect(m_justEatBtn, &QPushButton::clicked, this, &MealPage::onConfirmMeal);

    mainLayout->addWidget(m_resultPanel);
    connect(m_eatBtn, &QPushButton::clicked, this, &MealPage::onEatDish);
    connect(m_swapBtn, &QPushButton::clicked, this, &MealPage::onSwapDish);

    // === 附加阶段面板 ===
    m_extraPanel = new SketchyCard;
    m_extraPanel->setCardColor(QColor("#F7F2E8"));
    m_extraPanel->setVisible(false);
    auto *extraLayout = new QHBoxLayout(m_extraPanel);
    extraLayout->setContentsMargins(18, 14, 18, 14);
    extraLayout->setSpacing(12);

    auto *extraLabel = new QLabel(QString::fromUtf8(""));
    extraLabel->setStyleSheet(QString("font-size:16px; font-weight:bold; color:%1; background:transparent;").arg(C_INK.name()));
    extraLayout->addWidget(extraLabel);

    m_extraDrinkBtn = new SketchyButton(QString::fromUtf8("加个饮品/小吃"), C_CARD_WHEAT, C_SHADOW_DK);
    m_extraDrinkBtn->setFixedHeight(42);
    QFont extraFont;
    extraFont.setPointSize(12);
    extraFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    m_extraDrinkBtn->setFont(extraFont);
    extraLayout->addWidget(m_extraDrinkBtn);

    m_extraDoneBtn = new SketchyButton(QString::fromUtf8("够了，开吃！"), C_CARD_SAGE, C_SHADOW_DK);
    m_extraDoneBtn->setFixedHeight(42);
    m_extraDoneBtn->setFont(extraFont);
    extraLayout->addWidget(m_extraDoneBtn);
    mainLayout->addWidget(m_extraPanel);
    connect(m_extraDrinkBtn, &QPushButton::clicked, this, &MealPage::onAddExtra);
    connect(m_extraDoneBtn, &QPushButton::clicked, this, &MealPage::onConfirmMeal);

    // === 菜单区域 ===
    auto *menuLabel = new QLabel(QString::fromUtf8("今日菜单"));
    menuLabel->setStyleSheet(QString("font-size:17px; font-weight:bold; color:%1; margin-top:6px; background:transparent;").arg(C_INK.name()));
    mainLayout->addWidget(menuLabel);

    m_menuList = new QListWidget;
    m_menuList->setMaximumHeight(180);
    m_menuList->setStyleSheet(
        QString("QListWidget{background:rgba(255,255,255,200); border:2px dashed #C8BFA8;"
        "border-radius:10px; font-size:15px; color:%1;"
        "selection-background-color:#F0E8D8; selection-color:%1;"
        "padding:6px;}").arg(C_INK.name()));
    mainLayout->addWidget(m_menuList);

    // === 确认菜单按钮 ===
    m_confirmMealBtn = new SketchyButton(QString::fromUtf8("确认菜单，开始吃饭！"), C_CARD_BLUE, C_SHADOW_DK);
    m_confirmMealBtn->setFixedHeight(52);
    m_confirmMealBtn->setEnabled(false);
    QFont confirmFont;
    confirmFont.setPointSize(15);
    confirmFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.5);
    m_confirmMealBtn->setFont(confirmFont);
    mainLayout->addWidget(m_confirmMealBtn);
    connect(m_confirmMealBtn, &QPushButton::clicked, this, &MealPage::onConfirmMeal);

    mainLayout->addStretch();
    updatePhaseLabel();

    // 实时时钟：每秒更新左上角时间
    m_timeTimer = new QTimer(this);
    connect(m_timeTimer, &QTimer::timeout, this, &MealPage::updateTimeDisplay);
    m_timeTimer->start(1000);
    updateTimeDisplay();
}

void MealPage::updateTimeDisplay() {
    QTime now = QTime::currentTime();
    int h = now.hour();
    QString mode;
    QString serving;
    if (h < 4) {
        mode = QString::fromUtf8("深夜");
        serving = QString::fromUtf8("非供应时段");
    } else if (h < 10) {
        mode = QString::fromUtf8("早餐模式");
        serving = QString::fromUtf8("供应时段内");
    } else if (h < 14) {
        mode = QString::fromUtf8("中餐模式");
        serving = QString::fromUtf8("供应时段内");
    } else if (h < 17) {
        mode = QString::fromUtf8("下午茶");
        serving = QString::fromUtf8("非供应时段");
    } else if (h < 21) {
        mode = QString::fromUtf8("晚餐模式");
        serving = QString::fromUtf8("供应时段内");
    } else {
        mode = QString::fromUtf8("宵夜");
        serving = QString::fromUtf8("非供应时段");
    }
    m_timeLabel->setText(QString("%1  %2 | %3").arg(now.toString("HH:mm")).arg(mode).arg(serving));
}

// ============ 搭配系统 ============

bool MealPage::isBreakfast() const {
    int h = QTime::currentTime().hour();
    return h >= 4 && h < 10;
}

QSet<DishRole> MealPage::filledRoles() const {
    QSet<DishRole> roles;
    for (const auto &d : m_mealSelected) roles.insert(d.role);
    return roles;
}

bool MealPage::isCoreComplete() const {
    auto r = filledRoles();
    return r.contains(MEAT) && r.contains(VEGGIE) && r.contains(STAPLE);
}

bool MealPage::m_mealSelectedContains(const QString &name) const {
    for (const auto &d : m_mealSelected)
        if (d.name == name) return true;
    return false;
}

void MealPage::updatePhaseLabel() {
    if (m_phase == BREAKFAST_MAIN) {
        m_phaseLabel->setText(filledRoles().contains(FULL_MEAL)
            ? QString::fromUtf8("已点套餐") : QString::fromUtf8("干的❌ 喝的❌"));
    } else if (m_phase == BREAKFAST_DRINK) {
        m_phaseLabel->setText(filledRoles().contains(BEVERAGE)
            ? QString::fromUtf8("干的✅ 喝的✅") : QString::fromUtf8("干的✅ 喝的❌"));
    } else if (m_phase == MAIN_PHASE) {
        auto r = filledRoles();
        if (r.contains(FULL_MEAL)) {
            m_phaseLabel->setText(QString::fromUtf8("已点整餐"));
        } else {
            m_phaseLabel->setText(QString());
        }
    } else {
        m_phaseLabel->setText(filledRoles().contains(BEVERAGE)
            ? QString::fromUtf8("饮品✅ 小吃可选") : QString::fromUtf8("饮品/小吃可选"));
    }
}

// ============ 筛菜 ============

QVector<Dish> MealPage::filterByPhase(const QVector<Dish> &dishes) {
    QVector<Dish> out;
    auto r = filledRoles();
    for (const auto &d : dishes) {
        if (r.contains(d.role)) continue;
        if (m_mealExcluded.contains(d.name)) continue;
        if (m_mealSelectedContains(d.name)) continue;
        if (!m_lockedRestaurant.isEmpty() && d.restaurant != m_lockedRestaurant) continue;
        if (m_phase == BREAKFAST_MAIN) {
            if (d.role == BEVERAGE) continue;
        } else if (m_phase == BREAKFAST_DRINK) {
            if (d.role != BEVERAGE) continue;
        } else if (m_phase == MAIN_PHASE) {
            if (d.role == BEVERAGE || d.role == SNACK) continue;
            if (d.role == FULL_MEAL && !r.isEmpty()) continue;
        } else {
            if (d.role != BEVERAGE && d.role != SNACK) continue;
        }
        out.append(d);
    }
    return out;
}

QVector<Dish> MealPage::allSearchDishes() const {
    QVector<Dish> out;
    for (const auto &r : m_lastResults) out.append(r.dish);
    return out;
}

// ============ 抽卡 ============

void MealPage::doDraw(const QVector<Dish> &candidates, const QMap<QString, double> &weights) {
    if (candidates.isEmpty()) return;
    auto *gacha = new GachaWidget(this);
    m_currentGacha = gacha;
    gacha->resize(this->window()->size());
    connect(gacha, &GachaWidget::dishSelected, this, &MealPage::onGachaDishSelected);
    gacha->startDraw(candidates, weights);
}

void MealPage::onDrawLimited() {
    QVector<Dish> candidates = filterByPhase(DishData::filterByCurrentTime(allSearchDishes()));
    if (candidates.isEmpty()) { showNoDishes(); return; }
    QMap<QString, double> weights;
    for (const auto &d : candidates) {
        double s = 0.05;
        for (const auto &r : m_lastResults)
            if (r.dish.name == d.name) { s = qMax(r.score, 0.05); break; }
        weights[d.name] = s;
    }
    doDraw(candidates, Recommend::computeWeights(candidates, m_user));
}

void MealPage::onDrawWeighted() {
    QVector<Dish> full = DishData::filterByCurrentTime(m_allDishes);
    QVector<Dish> candidates = filterByPhase(full);
    if (candidates.isEmpty()) { showNoDishes(); return; }
    doDraw(candidates, Recommend::computeWeights(candidates, m_user));
}

void MealPage::showNoDishes() {
    m_resultDishLabel->setText(QString::fromUtf8("该阶段没有可抽的菜了！"));
    m_resultPanel->setVisible(true);
    m_eatBtn->setVisible(false);
    m_swapBtn->setVisible(false);
    m_giveUpBtn->setVisible(true);
    m_justEatBtn->setVisible(true);
    m_drawLimitedBtn->setEnabled(false);
    m_drawWeightedBtn->setEnabled(false);
}

// ============ 抽卡结果 ============

void MealPage::onGachaDishSelected(const Dish &dish) {
    m_currentGacha = nullptr;
    m_latestDish = dish;
    QString roleName;
    switch (dish.role) {
        case FULL_MEAL: roleName = QString::fromUtf8("整餐"); break;
        case MEAT:      roleName = QString::fromUtf8("荤菜"); break;
        case VEGGIE:    roleName = QString::fromUtf8("素菜"); break;
        case STAPLE:    roleName = QString::fromUtf8("主食"); break;
        case BEVERAGE:  roleName = QString::fromUtf8("饮品"); break;
        case SNACK:     roleName = QString::fromUtf8("小吃"); break;
    }
    m_resultDishLabel->setText(QString("%1 [%2]\n%3 | ¥%4 | %5kcal")
        .arg(dish.name).arg(roleName).arg(dish.restaurant)
        .arg(dish.price, 0, 'f', 1).arg(dish.calories, 0, 'f', 0));
    m_resultPanel->setVisible(true);
    m_eatBtn->setVisible(true);
    m_swapBtn->setVisible(true);
    m_giveUpBtn->setVisible(false);
    m_justEatBtn->setVisible(false);
    m_drawLimitedBtn->setEnabled(false);
    m_drawWeightedBtn->setEnabled(false);
}

void MealPage::onEatDish() {
    if (m_latestDish.name.isEmpty()) return;
    if (m_lockedRestaurant.isEmpty())
        m_lockedRestaurant = m_latestDish.restaurant;
    m_mealSelected.append(m_latestDish);
    m_latestDish.drawCount++;
    m_user.chooseCount[m_latestDish.name]++;
    refreshMenuList();
    m_resultPanel->setVisible(false);
    updatePhaseLabel();
    m_drawLimitedBtn->setEnabled(!m_lastResults.isEmpty());
    m_drawWeightedBtn->setEnabled(true);

    if (m_phase == BREAKFAST_MAIN) {
        if (m_latestDish.role == FULL_MEAL) {
            m_confirmMealBtn->setEnabled(true);
            updatePhaseLabel();
            return;
        }
        m_phase = BREAKFAST_DRINK;
        updatePhaseLabel();
        return;
    }
    if (m_phase == BREAKFAST_DRINK) {
        m_confirmMealBtn->setEnabled(true);
        updatePhaseLabel();
        return;
    }

    if (m_phase == MAIN_PHASE && m_latestDish.role == FULL_MEAL) {
        showExtraPhase(); return;
    }
    if (m_phase == MAIN_PHASE && isCoreComplete()) {
        showExtraPhase(); return;
    }
    if (m_phase == EXTRA_PHASE) {
        auto r = filledRoles();
        if (r.contains(BEVERAGE) && r.contains(SNACK)) {
            m_extraPanel->setVisible(false);
            m_confirmMealBtn->setEnabled(true);
        }
    }
    m_confirmMealBtn->setEnabled(m_phase == EXTRA_PHASE || isCoreComplete()
                                  || filledRoles().contains(FULL_MEAL));
}

void MealPage::onSwapDish() {
    if (m_latestDish.name.isEmpty()) return;
    m_mealExcluded.insert(m_latestDish.name);
    m_resultPanel->setVisible(false);
    m_drawLimitedBtn->setEnabled(!m_lastResults.isEmpty());
    m_drawWeightedBtn->setEnabled(true);
}

void MealPage::showExtraPhase() {
    if (m_phase == BREAKFAST_MAIN || m_phase == BREAKFAST_DRINK) {
        m_confirmMealBtn->setEnabled(true);
        return;
    }
    m_phase = EXTRA_PHASE;
    m_extraPanel->setVisible(true);
    m_resultPanel->setVisible(false);
    updatePhaseLabel();
    if (filledRoles().contains(BEVERAGE)) m_extraDrinkBtn->setEnabled(false);
}

void MealPage::onAddExtra() {
    m_phase = EXTRA_PHASE;
    m_extraPanel->setVisible(false);
    m_resultPanel->setVisible(false);
    updatePhaseLabel();
}

void MealPage::refreshMenuList() {
    m_menuList->clear();
    for (int i = 0; i < m_mealSelected.size(); ++i) {
        const auto &d = m_mealSelected[i];
        QString rt;
        switch (d.role) {
            case FULL_MEAL: rt = QString::fromUtf8("[整餐]"); break;
            case MEAT:      rt = QString::fromUtf8("[荤]"); break;
            case VEGGIE:    rt = QString::fromUtf8("[素]"); break;
            case STAPLE:    rt = QString::fromUtf8("[主食]"); break;
            case BEVERAGE:  rt = QString::fromUtf8("[饮]"); break;
            case SNACK:     rt = QString::fromUtf8("[小吃]"); break;
        }

        auto *item = new QListWidgetItem;
        m_menuList->addItem(item);

        auto *row = new QWidget;
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(8, 4, 8, 4);
        layout->setSpacing(8);

        auto *label = new QLabel(QString("%1 %2 — ¥%3 %4kcal")
            .arg(rt).arg(d.name).arg(d.price, 0, 'f', 1).arg(d.calories, 0, 'f', 0));
        label->setStyleSheet("font-size: 16px; color: #5d3a1a; background:transparent;");
        layout->addWidget(label);
        layout->addStretch();

        auto *removeBtn = new QPushButton(QString::fromUtf8("✕"));
        removeBtn->setFixedSize(24, 24);
        removeBtn->setStyleSheet(
            "QPushButton{background:transparent; color:#c0392b; font-size:14px; font-weight:bold; border:1px solid #e0c0b0; border-radius:12px;}"
            "QPushButton:hover{background:#fce4e4; color:#e74c3c; border-color:#e74c3c;}");
        layout->addWidget(removeBtn);

        int idx = i;
        connect(removeBtn, &QPushButton::clicked, this, [this, idx]() {
            removeDishFromMenu(idx);
        });

        item->setSizeHint(row->sizeHint());
        m_menuList->setItemWidget(item, row);
    }
}

void MealPage::onConfirmMeal() {
    // 不再直接结束，而是发射信号让 MainWindow 展示雷达确认页
    emit mealReadyForReview(m_mealSelected, m_user);
}

void MealPage::recomputePhase() {
    if (m_mealSelected.isEmpty()) {
        m_lockedRestaurant.clear();
        m_phase = isBreakfast() ? BREAKFAST_MAIN : MAIN_PHASE;
        return;
    }

    if (isBreakfast()) {
        auto r = filledRoles();
        bool hasStaple = r.contains(STAPLE) || r.contains(FULL_MEAL);
        bool hasDrink = r.contains(BEVERAGE);

        if (hasStaple && hasDrink)
            m_phase = BREAKFAST_DRINK;
        else
            m_phase = BREAKFAST_MAIN;
    } else {
        auto r = filledRoles();
        if (r.contains(FULL_MEAL) || isCoreComplete())
            m_phase = EXTRA_PHASE;
        else
            m_phase = MAIN_PHASE;
    }
}

void MealPage::removeDishFromMenu(int index) {
    if (index < 0 || index >= m_mealSelected.size()) return;

    m_mealSelected.removeAt(index);

    if (m_mealSelected.isEmpty()) {
        resetMeal();
        return;
    }

    recomputePhase();
    updatePhaseLabel();
    refreshMenuList();

    // Reset result panel and button states
    m_resultPanel->setVisible(false);
    m_eatBtn->setVisible(true);
    m_swapBtn->setVisible(true);
    m_giveUpBtn->setVisible(false);
    m_justEatBtn->setVisible(false);
    m_drawLimitedBtn->setEnabled(!m_lastResults.isEmpty());
    m_drawWeightedBtn->setEnabled(true);

    if (m_phase == EXTRA_PHASE) {
        m_extraPanel->setVisible(true);
        m_extraDrinkBtn->setEnabled(!filledRoles().contains(BEVERAGE));
    } else {
        m_extraPanel->setVisible(false);
    }

    m_confirmMealBtn->setEnabled(
        m_phase == EXTRA_PHASE || isCoreComplete() || filledRoles().contains(FULL_MEAL)
    );
}

void MealPage::onSearchResults(const QVector<SearchResult> &results) {
    m_lastResults = results;
    m_drawLimitedBtn->setEnabled(!results.isEmpty());
}

void MealPage::onTagsChanged(const QStringList &tags) { Q_UNUSED(tags); }

QStringList MealPage::activeTags() const {
    return m_searchWidget->activeTags();
}

void MealPage::resetMeal() {
    if (m_currentGacha) {
        m_currentGacha->hide();
        m_currentGacha->deleteLater();
        m_currentGacha = nullptr;
    }
    m_searchWidget->resetMealSession();
    m_mealExcluded.clear();
    m_mealSelected.clear();
    m_latestDish = Dish();
    m_lockedRestaurant.clear();
    m_lastResults.clear();
    m_phase = isBreakfast() ? BREAKFAST_MAIN : MAIN_PHASE;
    m_drawWeightedBtn->setEnabled(true);
    m_drawLimitedBtn->setEnabled(false);
    m_confirmMealBtn->setEnabled(false);
    m_resultPanel->setVisible(false);
    m_extraPanel->setVisible(false);
    m_eatBtn->setVisible(true);
    m_swapBtn->setVisible(true);
    m_extraDrinkBtn->setEnabled(true);
    m_menuList->clear();
    updatePhaseLabel();
}
