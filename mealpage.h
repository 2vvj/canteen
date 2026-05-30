#ifndef MEALPAGE_H
#define MEALPAGE_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QVector>
#include <QSet>
#include "dishdata.h"
#include "searchwidget.h"
#include "gachawidget.h"
#include "sketchyui.h"

// 用餐决策页面：搜索 → 抽卡 → 搭配
class MealPage : public QWidget {
    Q_OBJECT
public:
    MealPage(const QVector<Dish> &allDishes, UserProfile &user, QWidget *parent = nullptr);

    QStringList activeTags() const;
    void resetMeal();

signals:
    // 用户确认了菜品组合，要求展示雷达图确认
    void mealReadyForReview(const QVector<Dish> &selected, const UserProfile &user);
    // 用户想回到地图
    void backToMap();

private slots:
    void onSearchResults(const QVector<SearchResult> &results);
    void onTagsChanged(const QStringList &tags);
    void onDrawLimited();
    void onDrawWeighted();
    void onGachaDishSelected(const Dish &dish);
    void onEatDish();
    void onSwapDish();
    void onAddExtra();
    void onConfirmMeal();

protected:
    void paintEvent(QPaintEvent *) override;

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

    const QVector<Dish> &m_allDishes;
    UserProfile &m_user;
    Phase m_phase;

    // UI
    QLabel *m_timeLabel;
    QLabel *m_phaseLabel;
    SearchWidget *m_searchWidget;

    SketchyButton *m_backBtn;
    SketchyButton *m_drawLimitedBtn;
    SketchyButton *m_drawWeightedBtn;
    SketchyButton *m_confirmMealBtn;

    SketchyCard *m_resultPanel;
    QLabel *m_resultDishLabel;
    SketchyButton *m_eatBtn;
    SketchyButton *m_swapBtn;
    SketchyButton *m_giveUpBtn;
    SketchyButton *m_justEatBtn;

    SketchyCard *m_extraPanel;
    SketchyButton *m_extraDrinkBtn;
    SketchyButton *m_extraDoneBtn;

    QListWidget *m_menuList;

    // 状态
    QVector<SearchResult> m_lastResults;
    QSet<QString> m_mealExcluded;
    QVector<Dish> m_mealSelected;
    Dish m_latestDish;
    QString m_lockedRestaurant;
};

#endif
