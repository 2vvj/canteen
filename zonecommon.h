#ifndef ZONECOMMON_H
#define ZONECOMMON_H

#include <QString>
#include <QPolygonF>

struct ZoneInfo {
    int id = -1;
    QString name;
    QString category;
    QPolygonF polygon;

    bool isValid() const { return id >= 0 && !name.isEmpty() && polygon.size() >= 3; }
};

#endif
