#ifndef ACHIEVEMENTMANAGER_H
#define ACHIEVEMENTMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QMap>
#include "achievementdata.h"
#include "calendarwindow.h"

class AchievementManager : public QObject {
    Q_OBJECT
public:
    explicit AchievementManager(QObject *parent = nullptr);

    void load();
    void save();

    // 每餐后调用，检查所有未解锁成就
    void checkAll(const QDateTime &currentTime,
                  double bmr,
                  const QMap<QString, DailyRecord> &dailyRecords,
                  double currentMealTotalPrice,
                  const QMap<QString, QString> &eatingTimes = {},
                  const QMap<QString, double> &dishPrices = {});

    QString activeSkin() const;
    void setActiveSkin(const QString &key);

    const QVector<AchievementDef> &defs() const { return m_defs; }
    AchievementState state(const QString &key) const;
    bool hasNewAchievements() const { return !m_data.newlyUnlocked.isEmpty(); }
    QStringList newlyUnlocked() const { return m_data.newlyUnlocked; }
    void clearNewlyUnlocked();

signals:
    void activeSkinChanged(const QString &skinKey);

private:
    void unlock(const QString &key, const QDate &date);

    QVector<AchievementDef> m_defs;
    AchievementData m_data;
};

#endif
