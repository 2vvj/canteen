#include "DishData.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>

// ==========================================
// MealTime JSON 序列化
// ==========================================
QJsonObject MealTime::toJson() const {
    QJsonObject obj;
    obj["morning"] = morning;
    obj["noon"]    = noon;
    obj["evening"] = evening;
    return obj;
}

MealTime MealTime::fromJson(const QJsonObject &obj) {
    MealTime mt;
    mt.morning = obj["morning"].toBool();
    mt.noon    = obj["noon"].toBool();
    mt.evening = obj["evening"].toBool();
    return mt;
}

// ==========================================
// Dish JSON 序列化
// ==========================================
QJsonObject Dish::toJson() const {
    QJsonObject obj;
    obj["name"]      = name;
    obj["restaurant"] = restaurant;
    obj["price"]     = price;
    obj["calories"]  = calories;
    obj["location"]  = QJsonObject{{"x", location.x()}, {"y", location.y()}};
    obj["mealTime"]  = mealTime.toJson();
    obj["role"]      = dishRoleToString(role);

    QJsonArray tagsArr;
    for (const auto &t : tags) tagsArr.append(t);
    obj["tags"] = tagsArr;

    obj["tastyScore"]       = tastyScore;
    obj["experienceScore"]  = experienceScore;
    obj["healthScore"]      = healthScore;
    obj["drawCount"]        = drawCount;
    obj["totalRating"]      = totalRating;
    return obj;
}

Dish Dish::fromJson(const QJsonObject &obj) {
    Dish d;
    d.name           = obj["name"].toString();
    d.restaurant     = obj["restaurant"].toString();
    d.price          = obj["price"].toDouble();
    d.calories       = obj["calories"].toDouble();
    d.location       = QPointF(obj["location"].toObject()["x"].toDouble(),
                               obj["location"].toObject()["y"].toDouble());
    d.mealTime       = MealTime::fromJson(obj["mealTime"].toObject());
    d.role           = stringToDishRole(obj["role"].toString());

    const auto tagsArr = obj["tags"].toArray();
    for (const auto &t : tagsArr) d.tags.append(t.toString());

    d.tastyScore      = obj["tastyScore"].toDouble();
    d.experienceScore = obj["experienceScore"].toDouble();
    d.healthScore     = obj["healthScore"].toDouble();
    d.drawCount       = obj["drawCount"].toInt();
    d.totalRating     = obj["totalRating"].toDouble();
    return d;
}

// ==========================================
// UserProfile JSON 序列化
// ==========================================
QJsonObject UserProfile::toJson() const {
    QJsonObject obj;
    obj["name"]             = name;
    obj["dailyCalorieLimit"]= dailyCalorieLimit;
    obj["todayCalories"]    = todayCalories;

    QJsonObject ccObj;
    for (auto it = chooseCount.begin(); it != chooseCount.end(); ++it)
        ccObj[it.key()] = it.value();
    obj["chooseCount"] = ccObj;

    QJsonObject rtObj;
    for (auto it = ratings.begin(); it != ratings.end(); ++it)
        rtObj[it.key()] = it.value();
    obj["ratings"] = rtObj;

    QJsonArray rcArr;
    for (const auto &c : recentChoices) rcArr.append(c);
    obj["recentChoices"] = rcArr;
    return obj;
}

UserProfile UserProfile::fromJson(const QJsonObject &obj) {
    UserProfile u;
    u.name              = obj["name"].toString();
    u.dailyCalorieLimit = obj["dailyCalorieLimit"].toDouble(2500);
    u.todayCalories     = obj["todayCalories"].toDouble();

    const auto ccObj = obj["chooseCount"].toObject();
    for (auto it = ccObj.begin(); it != ccObj.end(); ++it)
        u.chooseCount[it.key()] = it.value().toInt();

    const auto rtObj = obj["ratings"].toObject();
    for (auto it = rtObj.begin(); it != rtObj.end(); ++it)
        u.ratings[it.key()] = it.value().toDouble();

    const auto rcArr = obj["recentChoices"].toArray();
    for (const auto &c : rcArr) u.recentChoices.append(c.toString());
    return u;
}

// ==========================================
// DishData namespace — SQLite-backed
// ==========================================
namespace DishData {

QVector<Dish> loadDishes(const QString & /* filename - unused, reads from SQLite */) {
    QVector<Dish> dishes;
    QSqlQuery query;

    // 从 Dishes 表 JOIN Restaurants 和 History 统计
    query.exec(
        "SELECT d.id, d.name, r.name, d.price, d.calories, "
        "       r.dist_dorm, r.dist_library, r.dist_building, "
        "       d.taste, d.experience, d.health, "
        "       d.role, d.morning, d.noon, d.evening, d.tags, "
        "       COALESCE(h.cnt, 0), COALESCE(h.total, 0), "
        "       d.current_weight "
        "FROM Dishes d "
        "JOIN Restaurants r ON d.restaurant_id = r.id "
        "LEFT JOIN ("
        "  SELECT dish_id, COUNT(*) AS cnt, SUM(rating) AS total "
        "  FROM History GROUP BY dish_id"
        ") h ON d.id = h.dish_id "
        "ORDER BY d.id");

    while (query.next()) {
        Dish dish;
        dish.name            = query.value(1).toString();
        dish.restaurant      = query.value(2).toString();
        dish.price           = query.value(3).toDouble();
        dish.calories        = query.value(4).toDouble();
        // 位置：取宿舍距离作为 location.x 简化处理
        dish.location        = QPointF(query.value(5).toDouble(), query.value(6).toDouble());
        dish.tastyScore      = query.value(8).toDouble() / 10.0;   // DB存0-100, Dish用0-10
        dish.experienceScore = query.value(9).toDouble() / 10.0;
        dish.healthScore     = query.value(10).toDouble() / 10.0;
        dish.role            = stringToDishRole(query.value(11).toString());
        dish.mealTime.morning = query.value(12).toInt() != 0;
        dish.mealTime.noon    = query.value(13).toInt() != 0;
        dish.mealTime.evening = query.value(14).toInt() != 0;
        // tags: 逗号分隔
        QString tagsStr = query.value(15).toString();
        if (!tagsStr.isEmpty())
            dish.tags = tagsStr.split(",");
        // 历史统计
        dish.drawCount   = query.value(16).toInt();
        dish.totalRating = query.value(17).toDouble();

        dishes.append(dish);
    }
    return dishes;
}

bool saveDishes(const QString & /* filename - unused, writes to SQLite */,
                const QVector<Dish> &dishes) {
    QSqlQuery query;
    query.prepare("UPDATE Dishes SET current_weight = ?, "
                  "taste = ?, experience = ?, health = ? "
                  "WHERE name = ?");

    for (const auto &dish : dishes) {
        query.addBindValue(dish.drawCount > 0
            ? qBound(0.01, dish.totalRating / dish.drawCount, 10.0)
            : 1.0);  // fallback weight = avg rating or 1.0
        query.addBindValue(static_cast<int>(dish.tastyScore * 10));
        query.addBindValue(static_cast<int>(dish.experienceScore * 10));
        query.addBindValue(static_cast<int>(dish.healthScore * 10));
        query.addBindValue(dish.name);

        if (!query.exec()) {
            qDebug() << "saveDishes 失败:" << dish.name << query.lastError().text();
            return false;
        }
    }
    return true;
}

UserProfile loadUserProfile(const QString & /* username */) {
    UserProfile profile;
    profile.name = "默认用户";

    QSqlQuery query;

    // 获取所有评分记录
    query.exec("SELECT d.name, h.rating FROM History h "
               "JOIN Dishes d ON h.dish_id = d.id");
    while (query.next()) {
        QString dishName = query.value(0).toString();
        int rating       = query.value(1).toInt();
        profile.chooseCount[dishName] = profile.chooseCount.value(dishName, 0) + 1;
        profile.ratings[dishName]     = rating;
    }

    // 获取最近选择（按时间倒序取前20）
    query.exec("SELECT d.name FROM History h "
               "JOIN Dishes d ON h.dish_id = d.id "
               "ORDER BY h.timestamp DESC LIMIT 20");
    while (query.next())
        profile.recentChoices.append(query.value(0).toString());

    // 今日已摄入热量
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    query.prepare("SELECT calories FROM DailyRecords WHERE date = ?");
    query.addBindValue(today);
    if (query.exec() && query.next())
        profile.todayCalories = query.value(0).toDouble();

    return profile;
}

bool saveUserProfile(const QString & /* username */,
                     const UserProfile & /* profile */) {
    // UserProfile 的持久化由 History/DailyRecords 表自动维护，
    // 此处无需额外写入操作。
    return true;
}

QVector<UserProfile> loadAllUsers(const QString & /* filename */) {
    QVector<UserProfile> users;
    users.append(loadUserProfile());
    return users;
}

bool saveAllUsers(const QString & /* filename */,
                  const QVector<UserProfile> & /* users */) {
    // 同上，由各业务表自动持久化
    return true;
}

QVector<Dish> filterByMealTime(const QVector<Dish> &dishes, const QString &period) {
    QVector<Dish> result;
    for (const auto &d : dishes) {
        if (d.mealTime.availableAt(period))
            result.append(d);
    }
    return result;
}

} // namespace DishData
