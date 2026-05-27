#include "zonemanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ZoneManager::ZoneManager(QObject *parent) : QObject(parent) {}

bool ZoneManager::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return false;

    QJsonObject root = doc.object();
    m_nextId = root.value("next_id").toInt(0);

    m_zones.clear();
    QJsonArray zonesArr = root.value("zones").toArray();
    for (const auto &val : zonesArr) {
        QJsonObject obj = val.toObject();
        ZoneInfo zi;
        zi.id = obj.value("id").toInt(-1);
        zi.name = obj.value("name").toString();
        zi.category = obj.value("category").toString();
        QPolygonF poly;
        QJsonArray pts = obj.value("points").toArray();
        for (const auto &pv : pts) {
            QJsonObject pt = pv.toObject();
            poly << QPointF(pt.value("x").toDouble(), pt.value("y").toDouble());
        }
        zi.polygon = poly;
        if (zi.isValid())
            m_zones.append(zi);
    }
    return true;
}

bool ZoneManager::saveToFile(const QString &filePath) const
{
    QJsonObject root;
    root["version"] = 1;
    root["next_id"] = m_nextId;

    QJsonArray zonesArr;
    for (const auto &z : m_zones) {
        QJsonObject obj;
        obj["id"] = z.id;
        obj["name"] = z.name;
        obj["category"] = z.category;
        QJsonArray pts;
        for (const auto &p : z.polygon) {
            QJsonObject pt;
            pt["x"] = p.x();
            pt["y"] = p.y();
            pts.append(pt);
        }
        obj["points"] = pts;
        zonesArr.append(obj);
    }
    root["zones"] = zonesArr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

int ZoneManager::addZone(const QString &name, const QString &category, const QPolygonF &polygon)
{
    ZoneInfo zi;
    zi.id = m_nextId++;
    zi.name = name;
    zi.category = category;
    zi.polygon = polygon;
    m_zones.append(zi);
    emit zonesChanged();
    return zi.id;
}

bool ZoneManager::removeZone(int zoneId)
{
    for (int i = 0; i < m_zones.size(); ++i) {
        if (m_zones[i].id == zoneId) {
            m_zones.removeAt(i);
            emit zonesChanged();
            return true;
        }
    }
    return false;
}

bool ZoneManager::updateZone(int zoneId, const QString &name, const QPolygonF &polygon)
{
    for (auto &z : m_zones) {
        if (z.id == zoneId) {
            z.name = name;
            z.polygon = polygon;
            emit zonesChanged();
            return true;
        }
    }
    return false;
}

int ZoneManager::zoneAtPoint(const QPointF &scenePoint) const
{
    for (const auto &z : m_zones) {
        if (z.polygon.containsPoint(scenePoint, Qt::OddEvenFill))
            return z.id;
    }
    return -1;
}

const ZoneInfo *ZoneManager::zoneInfo(int zoneId) const
{
    for (const auto &z : m_zones) {
        if (z.id == zoneId)
            return &z;
    }
    return nullptr;
}

QVector<ZoneInfo> ZoneManager::allZones() const
{
    return m_zones;
}
