#include "mealpage.h"
#include "recommend.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTime>

MealPage::MealPage(const QVector<Dish> &allDishes, UserProfile &user, QWidget *parent)
    : QWidget(parent), m_allDishes(allDishes), m_user(user)
{
    m_phase = isBreakfast() ? BREAKFAST_MAIN : MAIN_PHASE;
    setupUI();
}

void MealPage::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 16, 24, 24);
    mainLayout->setSpacing(12);

    // === 返回按钮 ===
    auto *backBtn = new QPushButton(QString::fromUtf8("← 返回地图"));
    backBtn->setStyleSheet(
        "QPushButton{background:transparent; color:#8b6914; font-size:14px; border:1px solid #d4c4a0;"
        "border-radius:8px; padding:6px 14px;}"
        "QPushButton:hover{background:#f5e6c8;}");
    connect(backBtn, &QPushButton::clicked, this, &MealPage::backToMap);
    mainLayout->addWidget(backBtn);

    // === 时段状态 ===
    auto *timeRow = new QHBoxLayout;
    m_timeLabel = new QLabel;
    m_timeLabel->setStyleSheet("font-size: 15px; color: #8b6914; font-weight: bold;");
    timeRow->addWidget(m_timeLabel);
    m_phaseLabel = new QLabel;
    m_phaseLabel->setStyleSheet("font-size: 14px; color: #e65100; font-weight: bold;");
    timeRow->addWidget(m_phaseLabel);
    timeRow->addStretch();
    mainLayout->addLayout(timeRow);

    // === 搜索 ===
    m_searchWidget = new SearchWidget(m_allDishes, m_user);
    mainLayout->addWidget(m_searchWidget);
    connect(m_searchWidget, &SearchWidget::searchResultsChanged, this, &MealPage::onSearchResults);
    connect(m_searchWidget, &SearchWidget::tagsChanged, this, &MealPage::onTagsChanged);

    // === 抽卡按钮 ===
    auto *btnRow = new QHBoxLayout;
    m_drawLimitedBtn = new QPushButton(QString::fromUtf8("从搜索结果中抽卡"));
    m_drawLimitedBtn->setMinimumHeight(56);
    m_drawLimitedBtn->setEnabled(false);
    m_drawLimitedBtn->setStyleSheet(
        "QPushButton{background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #ecb860,stop:1 #d49430);"
        "color:white; font-size:17px; font-weight:bold; border-radius:14px; padding:10px;}"
        "QPushButton:hover{background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #f0c870,stop:1 #e0a440);}"
        "QPushButton:disabled{background:#ccc; color:#999;}");
    btnRow->addWidget(m_drawLimitedBtn);

    m_drawWeightedBtn = new QPushButton(QString::fromUtf8("加权抽卡（惊喜模式）"));
    m_drawWeightedBtn->setMinimumHeight(56);
    m_drawWeightedBtn->setStyleSheet(
        "QPushButton{background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #c4956a,stop:1 #9a6a40);"
        "color:white; font-size:17px; font-weight:bold; border-radius:14px; padding:10px;}"
        "QPushButton:hover{background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #d0a878,stop:1 #b08050);}");
    btnRow->addWidget(m_drawWeightedBtn);
    mainLayout->addLayout(btnRow);
    connect(m_drawLimitedBtn, &QPushButton::clicked, this, &MealPage::onDrawLimited);
    connect(m_drawWeightedBtn, &QPushButton::clicked, this, &MealPage::onDrawWeighted);

    // === 抽卡结果 ===
    m_resultPanel = new QWidget;
    m_resultPanel->setStyleSheet("background:rgba(255,255,255,220); border-radius:14px; padding:12px;");
    m_resultPanel->setVisible(false);
    auto *resultLayout = new QVBoxLayout(m_resultPanel);
    m_resultDishLabel = new QLabel;
    m_resultDishLabel->setAlignment(Qt::AlignCenter);
    m_resultDishLabel->setWordWrap(true);
    m_resultDishLabel->setStyleSheet("font-size: 18px; color: #5d3a1a; background:transparent;");
    resultLayout->addWidget(m_resultDishLabel);

    auto *choiceRow = new QHBoxLayout;
    m_eatBtn = new QPushButton(QString::fromUtf8("吃这个！"));
    m_eatBtn->setMinimumHeight(48);
    m_eatBtn->setStyleSheet("QPushButton{background:#4caf50; color:white; font-size:17px; font-weight:bold; border-radius:12px;}"
                            "QPushButton:hover{background:#43a047;}");
    choiceRow->addWidget(m_eatBtn);
    m_swapBtn = new QPushButton(QString::fromUtf8("换一个"));
    m_swapBtn->setMinimumHeight(48);
    m_swapBtn->setStyleSheet("QPushButton{background:#ff9800; color:white; font-size:17px; font-weight:bold; border-radius:12px;}"
                             "QPushButton:hover{background:#f57c00;}");
    choiceRow->addWidget(m_swapBtn);
    resultLayout->addLayout(choiceRow);

    // Bug1: "没菜了"时的两个选择按钮
    auto *noDishRow = new QHBoxLayout;
    m_giveUpBtn = new QPushButton(QString::fromUtf8("不吃了"));
    m_giveUpBtn->setMinimumHeight(44);
    m_giveUpBtn->setStyleSheet("QPushButton{background:#ff9800; color:white; font-size:15px; font-weight:bold; border-radius:10px;}"
                                "QPushButton:hover{background:#f57c00;}");
    m_giveUpBtn->setVisible(false);
    noDishRow->addWidget(m_giveUpBtn);
    m_justEatBtn = new QPushButton(QString::fromUtf8("就这样吧，直接开饭！"));
    m_justEatBtn->setMinimumHeight(44);
    m_justEatBtn->setStyleSheet("QPushButton{background:#4caf50; color:white; font-size:15px; font-weight:bold; border-radius:10px;}"
                                 "QPushButton:hover{background:#43a047;}");
    m_justEatBtn->setVisible(false);
    noDishRow->addWidget(m_justEatBtn);
    resultLayout->addLayout(noDishRow);
    connect(m_giveUpBtn, &QPushButton::clicked, this, [this]() { resetMeal(); emit backToMap(); });
    connect(m_justEatBtn, &QPushButton::clicked, this, &MealPage::onConfirmMeal);

    mainLayout->addWidget(m_resultPanel);
    connect(m_eatBtn, &QPushButton::clicked, this, &MealPage::onEatDish);
    connect(m_swapBtn, &QPushButton::clicked, this, &MealPage::onSwapDish);

    // === 附加阶段面板 ===
    m_extraPanel = new QWidget;
    m_extraPanel->setStyleSheet("background:rgba(255,255,240,220); border-radius:12px; padding:10px;");
    m_extraPanel->setVisible(false);
    auto *extraLayout = new QHBoxLayout(m_extraPanel);
    auto *extraLabel = new QLabel(QString::fromUtf8("核心齐了！"));
    extraLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #5d3a1a; background:transparent;");
    extraLayout->addWidget(extraLabel);
    m_extraDrinkBtn = new QPushButton(QString::fromUtf8("加个饮品/小吃"));
    m_extraDrinkBtn->setMinimumHeight(44);
    m_extraDrinkBtn->setStyleSheet("QPushButton{background:#e8c97a; color:white; font-size:15px; font-weight:bold; border-radius:10px;}"
                                   "QPushButton:hover{background:#d4a040;}");
    extraLayout->addWidget(m_extraDrinkBtn);
    m_extraDoneBtn = new QPushButton(QString::fromUtf8("够了，开吃！"));
    m_extraDoneBtn->setMinimumHeight(44);
    m_extraDoneBtn->setStyleSheet("QPushButton{background:#4caf50; color:white; font-size:15px; font-weight:bold; border-radius:10px;}"
                                  "QPushButton:hover{background:#43a047;}");
    extraLayout->addWidget(m_extraDoneBtn);
    mainLayout->addWidget(m_extraPanel);
    connect(m_extraDrinkBtn, &QPushButton::clicked, this, &MealPage::onAddExtra);
    connect(m_extraDoneBtn, &QPushButton::clicked, this, &MealPage::onConfirmMeal);

    // === 菜单列表 ===
    auto *menuLabel = new QLabel(QString::fromUtf8("今日菜单"));
    menuLabel->setStyleSheet("font-size: 17px; font-weight: bold; color: #8b6914; margin-top: 4px; background:transparent;");
    mainLayout->addWidget(menuLabel);
    m_menuList = new QListWidget;
    m_menuList->setMaximumHeight(180);
    m_menuList->setStyleSheet(
        "QListWidget{background:rgba(255,255,255,200); border: 2px dashed #e0c8a0; border-radius: 10px;"
        "font-size: 16px; color: #5d3a1a; selection-background-color: #ffe8cc; selection-color: #5d3a1a;}");
    mainLayout->addWidget(m_menuList);

    // === 确认菜单按钮 ===
    m_confirmMealBtn = new QPushButton(QString::fromUtf8("确认菜单，开始吃饭！"));
    m_confirmMealBtn->setMinimumHeight(50);
    m_confirmMealBtn->setEnabled(false);
    m_confirmMealBtn->setStyleSheet(
        "QPushButton{background: #e8c97a; color:white; font-size:19px; font-weight:bold; border-radius:14px;}"
        "QPushButton:hover{background: #d4a040;}"
        "QPushButton:disabled{background:#ccc;}");
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
    QString mode = isBreakfast() ? QString::fromUtf8("早餐模式") : QString::fromUtf8("中晚餐模式");
    m_timeLabel->setText(QString("%1  %2 | 供应时段内").arg(now.toString("HH:mm")).arg(mode));
}

// ============ 搭配系统 ============

bool MealPage::isBreakfast() const {
    return QTime::currentTime().hour() < 10;
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
            QStringList missing;
            missing << (r.contains(MEAT)   ? QString::fromUtf8("荤✅") : QString::fromUtf8("荤❌"));
            missing << (r.contains(VEGGIE) ? QString::fromUtf8("素✅") : QString::fromUtf8("素❌"));
            missing << (r.contains(STAPLE) ? QString::fromUtf8("主食✅") : QString::fromUtf8("主食❌"));
            m_phaseLabel->setText(missing.join(" "));
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
    m_searchWidget->resetMealSession();
    m_mealExcluded.clear();
    m_mealSelected.clear();
    m_latestDish = Dish();
    m_lockedRestaurant.clear();
    m_lastResults.clear();
    m_phase = isBreakfast() ? BREAKFAST_MAIN : MAIN_PHASE;
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
