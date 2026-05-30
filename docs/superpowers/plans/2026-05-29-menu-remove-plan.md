# 今日菜单删除功能 - 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 MealPage 的"今日菜单"列表中为每道菜添加删除按钮，支持删除、阶段回退、餐厅解锁。

**Architecture:** 仅修改 mealpage.h 和 mealpage.cpp。新增 `recomputePhase()` 和 `removeDishFromMenu()` 两个方法，修改 `refreshMenuList()` 从纯文本改为自定义 item widget（含 ✕ 按钮）。

**Tech Stack:** Qt 6.5 Widgets, C++17

---

### Task 1: 添加方法声明

**Files:**
- Modify: `mealpage.h`

- [ ] **Step 1: 添加 recomputePhase 和 removeDishFromMenu 声明**

在 `mealpage.h` 的 `private:` 区域，`refreshMenuList()` 声明下方添加两行：

```cpp
// mealpage.h — 在 "void refreshMenuList();" 之后添加：
    void refreshMenuList();
    void recomputePhase();
    void removeDishFromMenu(int index);
```

完整 private 区域变为：

```cpp
private:
    void setupUI();
    enum Phase { MAIN_PHASE, EXTRA_PHASE, BREAKFAST_MAIN, BREAKFAST_DRINK };
    bool isBreakfast() const;
    QSet<DishRole> filledRoles() const;
    bool isCoreComplete() const;
    void updatePhaseLabel();
    void showExtraPhase();
    QVector<Dish> filterByPhase(const QVector<Dish> &dishes);
    QVector<Dish> allSearchDishes() const;
    void doDraw(const QVector<Dish> &candidates, const QMap<QString, double> &weights);
    void refreshMenuList();
    void recomputePhase();
    void removeDishFromMenu(int index);
    void showNoDishes();
    bool m_mealSelectedContains(const QString &name) const;
```

- [ ] **Step 2: 编译验证**

```bash
cd "c:/Users/cfk20/canteen" && "D:/Qt/6.5.3/mingw_64/bin/qmake.exe" "what_to_eat.pro" -o Makefile && /d/Mingw/mingw64/bin/mingw32-make.exe -f Makefile
```

预期：编译通过（函数未实现会有链接错误，先确认声明语法正确即可）。

---

### Task 2: 实现 recomputePhase()

**Files:**
- Modify: `mealpage.cpp`

- [ ] **Step 1: 在 mealpage.cpp 末尾（resetMeal 之前）添加 recomputePhase 实现**

```cpp
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
```

插入位置：在 `onConfirmMeal()` 之后，`onSearchResults()` 之前（或 `resetMeal()` 之前任意位置）。

---

### Task 3: 实现 removeDishFromMenu()

**Files:**
- Modify: `mealpage.cpp`

- [ ] **Step 1: 在 recomputePhase() 之后添加 removeDishFromMenu 实现**

```cpp
void MealPage::removeDishFromMenu(int index) {
    if (index < 0 || index >= m_mealSelected.size()) return;

    m_mealSelected.removeAt(index);

    recomputePhase();
    updatePhaseLabel();
    refreshMenuList();

    // 重置结果面板和按钮状态
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
```

---

### Task 4: 修改 refreshMenuList() 使用自定义 item widget

**Files:**
- Modify: `mealpage.cpp` — 替换 `refreshMenuList()` 函数体（第385-399行）

- [ ] **Step 1: 替换 refreshMenuList 实现**

```cpp
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
```

---

### Task 5: 完整编译并验证

**Files:**
- Verify: `mealpage.h`, `mealpage.cpp`

- [ ] **Step 1: 编译**

```bash
cd "c:/Users/cfk20/canteen" && "D:/Qt/6.5.3/mingw_64/bin/qmake.exe" "what_to_eat.pro" -o Makefile && /d/Mingw/mingw64/bin/mingw32-make.exe -f Makefile 2>&1
```

预期：**编译通过，0 错误**。

- [ ] **Step 2: 功能自检清单（运行时手动验证）**

| 操作 | 预期行为 |
|------|----------|
| 选菜阶段接受一道菜 | 菜单列表出现该项，右侧有 ✕ 按钮 |
| 点击 ✕ 删除唯一菜品 | 菜单清空，餐厅解锁，阶段回退到初始 |
| 接受荤+素+主食后，删掉素菜 | 加餐面板消失，回到主阶段要求重选素菜 |
| 接受荤+素+主食后，删掉饮品（加餐阶段） | 仍在加餐阶段，饮品可选 |
| 早餐：接受主食→接受饮品→删掉主食（饮品保留） | 回退 BREAKFAST_MAIN，餐厅锁维持 |
| 早餐：接受主食→删掉主食（无饮品） | 回退 BREAKFAST_MAIN，餐厅解锁 |
| 删除后点击"换一个" | 被删的菜可能再次被抽到（未加入排除集） |
