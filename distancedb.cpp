#include "distancedb.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>

DistanceDB::DistanceDB()
{
    static int connCounter = 0;
    m_connectionName = QString("distance_conn_%1").arg(connCounter++);
}

DistanceDB::~DistanceDB()
{
    close();
}

bool DistanceDB::open(const QString &dbPath)
{
    if (m_open) close();

    // Remove stale connection if it exists
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);
    if (!db.open())
        return false;

    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS distances ("
           "zone_a INTEGER NOT NULL,"
           "zone_b INTEGER NOT NULL,"
           "distance REAL NOT NULL,"
           "PRIMARY KEY (zone_a, zone_b))");
    m_open = true;
    return true;
}

void DistanceDB::close()
{
    if (!m_open) return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_open = false;
}

bool DistanceDB::isOpen() const
{
    return m_open;
}

std::pair<int, int> DistanceDB::orderedPair(int a, int b)
{
    return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

double DistanceDB::getDistance(int zoneA, int zoneB) const
{
    if (!m_open) return -1.0;
    auto [a, b] = orderedPair(zoneA, zoneB);

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    q.prepare("SELECT distance FROM distances WHERE zone_a = ? AND zone_b = ?");
    q.addBindValue(a);
    q.addBindValue(b);
    if (q.exec() && q.next())
        return q.value(0).toDouble();
    return -1.0;
}

bool DistanceDB::setDistance(int zoneA, int zoneB, double distance)
{
    if (!m_open) return false;
    auto [a, b] = orderedPair(zoneA, zoneB);

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO distances (zone_a, zone_b, distance) VALUES (?, ?, ?)");
    q.addBindValue(a);
    q.addBindValue(b);
    q.addBindValue(distance);
    return q.exec();
}

bool DistanceDB::removeDistance(int zoneA, int zoneB)
{
    if (!m_open) return false;
    auto [a, b] = orderedPair(zoneA, zoneB);

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    q.prepare("DELETE FROM distances WHERE zone_a = ? AND zone_b = ?");
    q.addBindValue(a);
    q.addBindValue(b);
    return q.exec();
}

QMap<int, double> DistanceDB::allDistancesForZone(int zoneId) const
{
    QMap<int, double> result;
    if (!m_open) return result;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    q.prepare("SELECT zone_a, zone_b, distance FROM distances WHERE zone_a = ? OR zone_b = ?");
    q.addBindValue(zoneId);
    q.addBindValue(zoneId);
    if (!q.exec()) return result;

    while (q.next()) {
        int a = q.value(0).toInt();
        int b = q.value(1).toInt();
        double d = q.value(2).toDouble();
        int other = (a == zoneId) ? b : a;
        result[other] = d;
    }
    return result;
}

bool DistanceDB::loadFromJson(const QString &filePath)
{
    if (!m_open) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray())
        return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        int a = obj.value("zone_a").toInt(-1);
        int b = obj.value("zone_b").toInt(-1);
        double d = obj.value("distance").toDouble(-1.0);
        if (a < 0 || b < 0 || d < 0) continue;

        auto [low, high] = orderedPair(a, b);
        QSqlQuery q(db);
        q.prepare("INSERT OR REPLACE INTO distances (zone_a, zone_b, distance) VALUES (?, ?, ?)");
        q.addBindValue(low);
        q.addBindValue(high);
        q.addBindValue(d);
        q.exec();
    }
    return true;
}
