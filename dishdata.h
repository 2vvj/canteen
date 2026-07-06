#ifndef DISHDATA_H
#define DISHDATA_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QTime>
#include <QJsonObject>

enum DishRole {
    FULL_MEAL,  // 整餐
    MEAT,       // 荤菜
    VEGGIE,     // 素菜
    STAPLE,     // 主食
    BEVERAGE,   // 饮品
    SNACK       // 小吃
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
};

namespace DishData {
    QVector<Dish> loadDishes(const QString &filename);
    bool saveDishes(const QString &filename, const QVector<Dish> &dishes);

    QVector<Dish> filterByCurrentTime(const QVector<Dish> &dishes);
}

#endif
