#ifndef DISHDATA_H
#define DISHDATA_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QTime>
#include <QJsonObject>

// 菜品角色（搭配系统用）
enum DishRole {
    FULL_MEAL,  // 整餐：面、套餐 → 抽到即结束
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

// 供应时间窗口
struct SupplyWindow {
    QTime start;  // e.g. 11:00
    QTime end;    // e.g. 13:00

    QJsonObject toJson() const;
    static SupplyWindow fromJson(const QJsonObject &obj);

    bool contains(const QTime &t) const {
        return t >= start && t <= end;
    }
};

struct Dish {
    QString name;            // 菜名
    QString restaurant;      // 食堂+楼层，如"一食堂一楼"
    double price = 0;        // 价格（元）
    double calories = 0;     // 热量（kcal）
    QVector<SupplyWindow> supplyWindows;  // 供应时间窗口（可多个时段）
    DishRole role = MEAT;    // 菜品角色（搭配系统用）
    QStringList tags;        // 标签：菜系、口味、套餐等

    // 以下由队友的数据层维护
    double tastyScore = -1;   // 美味度 0-10（必填，-1表示未设置）
    double experienceScore = 5.0;// 体验感 0-10
    double healthScore = 5.0;   // 健康度 0-10

    // 历史统计（推荐算法用）
    int drawCount = 0;
    double lastRating = 3.0;   // 最近一次评分（队友维护），默认3分

    QJsonObject toJson() const;
    static Dish fromJson(const QJsonObject &obj);
};

// 用户数据
struct UserProfile {
    QString name;
    double dailyCalorieLimit = 2500;
    double todayCalories = 0;
    QMap<QString, int> chooseCount;     // 菜名 -> 被选次数
    QMap<QString, double> ratings;      // "菜名|食堂" -> 用户评分 (0-10)
    QStringList recentChoices;

    QJsonObject toJson() const;
    static UserProfile fromJson(const QJsonObject &obj);
};

// 数据读写
namespace DishData {
    QVector<Dish> loadDishes(const QString &filename);
    bool saveDishes(const QString &filename, const QVector<Dish> &dishes);

    UserProfile loadUserProfile(const QString &filename);
    bool saveUserProfile(const QString &filename, const UserProfile &profile);

    QVector<UserProfile> loadAllUsers(const QString &filename);
    bool saveAllUsers(const QString &filename, const QVector<UserProfile> &users);

    // 按实时时间筛选（当前时间落在任一 supplyWindow 内则保留）
    QVector<Dish> filterByCurrentTime(const QVector<Dish> &dishes);
}

#endif
