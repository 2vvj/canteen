#include "userdata.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

bool UserData::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return false;

    QJsonObject obj = doc.object();
    settings.avatarPath = obj.value("avatar").toString();
    settings.name = obj.value("name").toString("用户");
    settings.gender = obj.value("gender").toString();
    settings.height = obj.value("height").toInt(0);
    settings.weight = obj.value("weight").toInt(0);
    settings.age = obj.value("age").toInt(0);
    return true;
}

bool UserData::save(const QString &filePath) const
{
    QJsonObject obj;
    // Read existing file to preserve ratings/calories/calorieDate etc.
    {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            obj = QJsonDocument::fromJson(f.readAll()).object();
            f.close();
        }
    }
    obj["avatar"] = settings.avatarPath;
    obj["name"] = settings.name;
    obj["gender"] = settings.gender;
    obj["height"] = settings.height;
    obj["weight"] = settings.weight;
    obj["age"] = settings.age;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
