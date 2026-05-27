#ifndef ZONEMANAGER_H
#define ZONEMANAGER_H

#include <QObject>
#include <QVector>
#include "zonecommon.h"

class ZoneManager : public QObject {
    Q_OBJECT
public:
    explicit ZoneManager(QObject *parent = nullptr);

    bool loadFromFile(const QString &filePath);
    bool saveToFile(const QString &filePath) const;

    int  addZone(const QString &name, const QString &category, const QPolygonF &polygon);
    bool removeZone(int zoneId);
    bool updateZone(int zoneId, const QString &name, const QPolygonF &polygon);

    int  zoneAtPoint(const QPointF &scenePoint) const;
    const ZoneInfo *zoneInfo(int zoneId) const;
    QVector<ZoneInfo> allZones() const;
    int  zoneCount() const { return m_zones.size(); }

signals:
    void zonesChanged();

private:
    QVector<ZoneInfo> m_zones;
    int m_nextId = 0;
};

#endif
