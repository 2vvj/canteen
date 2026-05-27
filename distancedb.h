#ifndef DISTANCEDB_H
#define DISTANCEDB_H

#include <QString>
#include <QMap>

class DistanceDB {
public:
    DistanceDB();
    ~DistanceDB();

    bool open(const QString &dbPath);
    void close();
    bool isOpen() const;

    bool loadFromJson(const QString &filePath);

    double getDistance(int zoneA, int zoneB) const;
    bool setDistance(int zoneA, int zoneB, double distance);
    bool removeDistance(int zoneA, int zoneB);
    QMap<int, double> allDistancesForZone(int zoneId) const;

private:
    static std::pair<int, int> orderedPair(int a, int b);

    QString m_connectionName;
    bool m_open = false;
};

#endif
