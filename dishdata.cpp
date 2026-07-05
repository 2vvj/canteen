#include "dishdata.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

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
