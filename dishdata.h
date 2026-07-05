#ifndef DISHDATA_H
#define DISHDATA_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QTime>
#include <QJsonObject>

enum DishRole {
    FULL_MEAL,  // 整餐：面、套餐
    MEAT,       // 荤菜
    VEGGIE,     // 素菜
    STAPLE,     // 主食：米饭、馒头
    BEVERAGE,   // 饮品：汤、豆浆、奶茶
    SNACK       // 小吃：煎饼、烤冷面
};

inline QString dishRoleToString(DishRole r) {
    switch (r) {
    case FULL_MEAL: return "FULL_MEAL";
    case MEAT:      return "MEAT";
    case VEGGIE:    return "VEGGIE";
    case STAPLE:    return "STAPLE";
    case BEVERAGE:  return "BEVERAGE";
    case SNACK:     return "SNACK";
    }
    return "MEAT";
}

inline DishRole stringToDishRole(const QString &s) {
    if (s == "FULL_MEAL") return FULL_MEAL;
    if (s == "MEAT")      return MEAT;
    if (s == "VEGGIE")    return VEGGIE;
    if (s == "STAPLE")    return STAPLE;
    if (s == "BEVERAGE")  return BEVERAGE;
    if (s == "SNACK")     return SNACK;
    return MEAT;
}

struct SupplyWindow {
    QTime start;
    QTime end;

    QJsonObject toJson() const;
    static SupplyWindow fromJson(const QJsonObject &obj);

    bool contains(const QTime &t) const {
        return t >= start && t <= end;
    }
};

struct Dish {
    QString name;
    QString restaurant;
    double price = 0;
    double calories = 0;
    QVector<SupplyWindow> supplyWindows;
    DishRole role = MEAT;
    QStringList tags;

    // 以下由队友的数据层维护
    //队友维护了吗？
    double tastyScore = -1;
    double experienceScore = 5.0;
    double healthScore = 5.0;

    int drawCount = 0;
    double lastRating = 3.0;

    QJsonObject toJson() const;
    static Dish fromJson(const QJsonObject &obj);
};

struct UserProfile {
    QString name;
    double dailyCalorieLimit = 2500;
    double todayCalories = 0;
    QMap<QString, int> chooseCount;
    QMap<QString, double> ratings;
    QStringList recentChoices;

    QJsonObject toJson() const;
    static UserProfile fromJson(const QJsonObject &obj);
};

namespace DishData {
    QVector<Dish> loadDishes(const QString &filename);
    bool saveDishes(const QString &filename, const QVector<Dish> &dishes);

    UserProfile loadUserProfile(const QString &filename);
    bool saveUserProfile(const QString &filename, const UserProfile &profile);

    QVector<UserProfile> loadAllUsers(const QString &filename);
    bool saveAllUsers(const QString &filename, const QVector<UserProfile> &users);

    QVector<Dish> filterByCurrentTime(const QVector<Dish> &dishes);
}

#endif
