#include "dishdata.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

// ============ SupplyWindow ============

QJsonObject SupplyWindow::toJson() const {
    QJsonObject obj;
    obj["start"] = start.toString("HH:mm");
    obj["end"] = end.toString("HH:mm");
    return obj;
}

SupplyWindow SupplyWindow::fromJson(const QJsonObject &obj) {
    SupplyWindow sw;
    sw.start = QTime::fromString(obj["start"].toString(), "HH:mm");
    sw.end = QTime::fromString(obj["end"].toString(), "HH:mm");
    return sw;
}

// ============ Dish ============

QJsonObject Dish::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["restaurant"] = restaurant;
    obj["price"] = price;
    obj["calories"] = calories;
    QJsonArray swArr;
    for (const auto &sw : supplyWindows)
        swArr.append(sw.toJson());
    obj["supplyWindows"] = swArr;
    obj["role"] = dishRoleToString(role);
    obj["tags"] = QJsonArray::fromStringList(tags);
    obj["tastyScore"] = tastyScore;
    obj["experienceScore"] = experienceScore;
    obj["healthScore"] = healthScore;
    obj["drawCount"] = drawCount;
    obj["lastRating"] = lastRating;
    return obj;
}

Dish Dish::fromJson(const QJsonObject &obj) {
    Dish d;
    d.name = obj["name"].toString();
    d.restaurant = obj["restaurant"].toString();
    d.price = obj["price"].toDouble();
    d.calories = obj["calories"].toDouble();
    d.role = stringToDishRole(obj["role"].toString());
    for (const auto &v : obj["supplyWindows"].toArray())
        d.supplyWindows.append(SupplyWindow::fromJson(v.toObject()));
    for (const auto &v : obj["tags"].toArray())
        d.tags.append(v.toString());
    d.tastyScore = obj["tastyScore"].toDouble(-1);
    d.experienceScore = obj["experienceScore"].toDouble(5.0);
    d.healthScore = obj["healthScore"].toDouble(5.0);
    d.drawCount = obj["drawCount"].toInt(0);
    d.lastRating = obj["lastRating"].toDouble(3.0);
    return d;
}

// ============ UserProfile ============

QJsonObject UserProfile::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["dailyCalorieLimit"] = dailyCalorieLimit;
    obj["todayCalories"] = todayCalories;
    QJsonObject cc;
    for (auto it = chooseCount.begin(); it != chooseCount.end(); ++it)
        cc[it.key()] = it.value();
    obj["chooseCount"] = cc;
    QJsonObject rt;
    for (auto it = ratings.begin(); it != ratings.end(); ++it)
        rt[it.key()] = it.value();
    obj["ratings"] = rt;
    obj["recentChoices"] = QJsonArray::fromStringList(recentChoices);
    return obj;
}

UserProfile UserProfile::fromJson(const QJsonObject &obj) {
    UserProfile p;
    p.name = obj["name"].toString();
    p.dailyCalorieLimit = obj["dailyCalorieLimit"].toDouble(2500);
    p.todayCalories = obj["todayCalories"].toDouble(0);
    QJsonObject cc = obj["chooseCount"].toObject();
    for (auto it = cc.begin(); it != cc.end(); ++it)
        p.chooseCount[it.key()] = it.value().toInt();
    QJsonObject rt = obj["ratings"].toObject();
    for (auto it = rt.begin(); it != rt.end(); ++it)
        p.ratings[it.key()] = it.value().toDouble();
    for (const auto &v : obj["recentChoices"].toArray())
        p.recentChoices.append(v.toString());
    return p;
}

// ============ DishData ============

QVector<Dish> DishData::loadDishes(const QString &filename) {
    QVector<Dish> dishes;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return dishes;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    for (const auto &v : doc.array())
        dishes.append(Dish::fromJson(v.toObject()));
    return dishes;
}

bool DishData::saveDishes(const QString &filename, const QVector<Dish> &dishes) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QJsonArray arr;
    for (const auto &d : dishes) arr.append(d.toJson());
    file.write(QJsonDocument(arr).toJson());
    file.close();
    return true;
}

UserProfile DishData::loadUserProfile(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return UserProfile();
    UserProfile p = UserProfile::fromJson(QJsonDocument::fromJson(file.readAll()).object());
    file.close();
    return p;
}

bool DishData::saveUserProfile(const QString &filename, const UserProfile &profile) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(profile.toJson()).toJson());
    file.close();
    return true;
}

QVector<UserProfile> DishData::loadAllUsers(const QString &filename) {
    QVector<UserProfile> users;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return users;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    for (const auto &v : doc.array())
        users.append(UserProfile::fromJson(v.toObject()));
    return users;
}

bool DishData::saveAllUsers(const QString &filename, const QVector<UserProfile> &users) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QJsonArray arr;
    for (const auto &u : users) arr.append(u.toJson());
    file.write(QJsonDocument(arr).toJson());
    file.close();
    return true;
}

QVector<Dish> DishData::filterByCurrentTime(const QVector<Dish> &dishes) {
    QTime now = QTime::currentTime();
    QVector<Dish> result;
    for (const auto &d : dishes) {
        if (d.supplyWindows.isEmpty()) continue;
        for (const auto &sw : d.supplyWindows) {
            if (sw.contains(now)) {
                result.append(d);
                break;
            }
        }
    }
    return result;
}
