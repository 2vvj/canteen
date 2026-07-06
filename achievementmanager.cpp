#include "achievementmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QSet>
#include <QTime>
#include <algorithm>
#include <QPair>

AchievementManager::AchievementManager(QObject *parent) : QObject(parent) {
    m_defs = achievementDefs();
    for (const auto &d : m_defs)
        m_data.states[d.key] = AchievementState{};
    m_data.activeSkin = "first_record";
}

void AchievementManager::load() {
    QFile f("achievements.json");
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    m_data = AchievementData::fromJson(obj);
    f.close();
}

void AchievementManager::save() {
    QFile f("achievements.json");
    f.open(QIODevice::WriteOnly);
    f.write(QJsonDocument(m_data.toJson()).toJson());
    f.close();
}

QString AchievementManager::activeSkin() const {
    return m_data.activeSkin;
}

void AchievementManager::setActiveSkin(const QString &key) {
    if (!m_data.states.contains(key)) return;
    if (!m_data.states[key].unlocked) return;
    if (m_data.activeSkin == key) return;
    m_data.activeSkin = key;
    save();
    emit activeSkinChanged(key);
}

AchievementState AchievementManager::state(const QString &key) const {
    return m_data.states.value(key);
}

void AchievementManager::clearNewlyUnlocked() {
    if (m_data.newlyUnlocked.isEmpty()) return;
    m_data.newlyUnlocked.clear();
    save();
}

void AchievementManager::unlock(const QString &key, const QDate &date) {
    auto &st = m_data.states[key];
    if (st.unlocked) return;
    st.unlocked = true;
    st.unlockDate = date.toString("yyyy-MM-dd");
    st.progress = 1;
    m_data.newlyUnlocked.append(key);
    save();
}

void AchievementManager::checkAll(const QDateTime &currentTime,
                                   double bmr,
                                   const QMap<QString, DailyRecord> &dailyRecords,
                                   double currentMealTotalPrice,
                                   const QMap<QString, QString> &eatingTimes,
                                   const QMap<QString, double> &dishPrices) {
    QDate today = currentTime.date();

    if (!dailyRecords.isEmpty()) {
        auto &fr = m_data.states["first_record"];
        if (!fr.unlocked) {
            QStringList keys = dailyRecords.keys();
            keys.sort();
            QDate firstDate = QDate::fromString(keys.first(), "yyyy-MM-dd");
            unlock("first_record", firstDate.isValid() ? firstDate : today);
        }
    }

    // 深夜食堂
    {
        auto &lm = m_data.states["late_meal"];
        if (!lm.unlocked) {
            QDate lateDate;
            bool found = false;
            if (currentTime.time().hour() >= 21) {
                lateDate = today;
                found = true;
            }
            if (!found) {
                for (auto it = eatingTimes.begin(); it != eatingTimes.end(); ++it) {
                    QString dtStr = it.value();
                    QDateTime dt = QDateTime::fromString(dtStr, "yyyy-MM-dd HH:mm");
                    if (dt.isValid() && dt.time().hour() >= 21) {
                        lateDate = dt.date();
                        found = true;
                        break;
                    }
                }
            }
            if (found) unlock("late_meal", lateDate);
        }
    }

    // 豪华大餐王
    {
        auto &lx = m_data.states["luxury"];
        if (!lx.unlocked) {
            QDate luxDate;
            bool found = false;
            if (currentMealTotalPrice > 50.0) {
                luxDate = today;
                found = true;
            }
            if (!found) {
                QMap<QPair<QString, int>, double> mealPrices;
                for (auto it = eatingTimes.begin(); it != eatingTimes.end(); ++it) {
                    QString ts = it.value();
                    QDateTime dt = QDateTime::fromString(ts, "yyyy-MM-dd HH:mm");
                    if (!dt.isValid()) continue;
                    int h = dt.time().hour();
                    int period = (h < 10) ? 0 : (h < 15) ? 1 : 2;
                    QString dateStr = dt.date().toString("yyyy-MM-dd");

                    QString fullKey = it.key();
                    QString dishKey = fullKey.left(fullKey.length() - ts.length() - 1);

                    auto priceIt = dishPrices.find(dishKey);
                    if (priceIt != dishPrices.end())
                        mealPrices[{dateStr, period}] += priceIt.value();
                }
                
                QStringList mealKeys;
                for (auto it = mealPrices.begin(); it != mealPrices.end(); ++it)
                    mealKeys.append(it.key().first + "|" + QString::number(it.key().second));
                mealKeys.sort();
                for (const auto &mk : mealKeys) {
                    QString dateStr = mk.section('|', 0, 0);
                    int period = mk.section('|', 1, 1).toInt();
                    if (mealPrices[{dateStr, period}] > 50.0) {
                        luxDate = QDate::fromString(dateStr, "yyyy-MM-dd");
                        if (luxDate.isValid()) { found = true; break; }
                    }
                }
            }
            if (found) unlock("luxury", luxDate);
        }
    }

    QStringList keys = dailyRecords.keys();
    keys.sort();

    auto findStreakInfo = [&](auto predicate) -> QPair<int, QPair<QDate, QDate>> {
        int best = 0, current = 0;
        QDate prev, d3, d7;

        for (const auto &key : keys) {
            QDate d = QDate::fromString(key, "yyyy-MM-dd");
            if (!d.isValid() || d >= today) continue;

            if (!predicate(dailyRecords[key])) {
                current = 0; prev = d; continue;
            }

            if (current == 0 || d != prev.addDays(1))
                current = 1;
            else
                current++;

            if (current > best) best = current;
            if (!d3.isValid() && current >= 3) d3 = d;
            if (!d7.isValid() && current >= 7) d7 = d;
            prev = d;
        }
        return {best, {d3, d7}};
    };

    auto underInfo = findStreakInfo([bmr](const DailyRecord &r) { return r.totalCalories <= bmr + 300; });
    auto overInfo  = findStreakInfo([bmr](const DailyRecord &r) { return r.totalCalories > bmr + 300; });
    auto recInfo   = findStreakInfo([](const DailyRecord &) { return true; });

    auto updateStreak = [&](const QString &key, int target, int currentStreak, const QDate &achieveDate) {
        auto &st = m_data.states[key];
        if (st.unlocked) return;
        st.progress = currentStreak;
        if (currentStreak >= target)
            unlock(key, achieveDate.isValid() ? achieveDate : today);
    };

    updateStreak("streak_calorie_3", 3, underInfo.first, underInfo.second.first);
    updateStreak("streak_calorie_7", 7, underInfo.first, underInfo.second.second);
    updateStreak("streak_over_3",    3, overInfo.first,  overInfo.second.first);
    updateStreak("streak_over_7",    7, overInfo.first,  overInfo.second.second);
    updateStreak("streak_record_7",  7, recInfo.first,   recInfo.second.second);

    // 味蕾收藏家 + 百味大师
    QSet<QString> seen;
    QDate taste50Date, taste100Date;
    for (const auto &key : keys) {
        QDate d = QDate::fromString(key, "yyyy-MM-dd");
        if (!d.isValid()) continue;
        if (d >= today) continue;
        for (const auto &dish : dailyRecords[key].dishes)
            seen.insert(dish);
        if (!taste50Date.isValid() && seen.size() >= 50)
            taste50Date = d;
        if (!taste100Date.isValid() && seen.size() >= 100)
            taste100Date = d;
    }

    auto updateCumulative = [&](const QString &key, int target, int count, const QDate &achieveDate) {
        auto &st = m_data.states[key];
        if (st.unlocked) return;
        st.progress = count;
        if (count >= target) {
            QDate useDate = achieveDate.isValid() ? achieveDate : today;
            unlock(key, useDate);
        }
    };

    updateCumulative("taste_50",  50, seen.size(), taste50Date);
    updateCumulative("taste_100", 100, seen.size(), taste100Date);
}
