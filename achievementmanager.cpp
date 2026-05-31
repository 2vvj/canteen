#include "achievementmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QSet>
#include <algorithm>

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
                                   double todayCalories,
                                   double bmr,
                                   const QMap<QString, DailyRecord> &dailyRecords,
                                   double currentMealTotalPrice) {
    QDate today = currentTime.date();
    bool hasRecordToday = dailyRecords.contains(today.toString("yyyy-MM-dd"));

    // ── 一次性成就 ──

    if (hasRecordToday) {
        auto &fr = m_data.states["first_record"];
        if (!fr.unlocked) unlock("first_record", today);
    }

    if (currentTime.time().hour() >= 21) {
        auto &lm = m_data.states["late_meal"];
        if (!lm.unlocked) unlock("late_meal", today);
    }

    if (currentMealTotalPrice > 50.0) {
        auto &lx = m_data.states["luxury"];
        if (!lx.unlocked) unlock("luxury", today);
    }

    // ── 连续型成就：统计连续天数 ──

    // 从 today 往回数，满足条件则+1，遇不满足则断
    auto countReverseStreak = [&](auto predicate) -> int {
        int streak = 0;
        QDate cursor = today;
        // 如果今天没有记录，从昨天开始检查
        if (!dailyRecords.contains(cursor.toString("yyyy-MM-dd")))
            cursor = cursor.addDays(-1);
        while (true) {
            QString key = cursor.toString("yyyy-MM-dd");
            if (!dailyRecords.contains(key)) break;
            if (!predicate(dailyRecords[key])) break;
            streak++;
            cursor = cursor.addDays(-1);
        }
        return streak;
    };

    int underStreak = countReverseStreak([bmr](const DailyRecord &r) {
        return r.totalCalories <= bmr + 300;
    });
    int overStreak = countReverseStreak([bmr](const DailyRecord &r) {
        return r.totalCalories > bmr + 300;
    });
    int recordStreak = countReverseStreak([](const DailyRecord &) { return true; });

    auto updateStreak = [&](const QString &key, int target, int currentStreak) {
        auto &st = m_data.states[key];
        if (st.unlocked) return;
        st.progress = currentStreak;
        if (currentStreak >= target) unlock(key, today);
    };

    updateStreak("streak_calorie_3", 3, underStreak);
    updateStreak("streak_calorie_7", 7, underStreak);
    updateStreak("streak_over_3",    3, overStreak);
    updateStreak("streak_over_7",    7, overStreak);
    updateStreak("streak_record_7",  7, recordStreak);

    // ── 累计型成就：统计所有历史不重复菜品 ──

    QSet<QString> allDishes;
    for (auto it = dailyRecords.begin(); it != dailyRecords.end(); ++it) {
        for (const auto &dish : it->dishes)
            allDishes.insert(dish);
    }

    auto updateCumulative = [&](const QString &key, int target, int count) {
        auto &st = m_data.states[key];
        if (st.unlocked) return;
        st.progress = count;
        if (count >= target) unlock(key, today);
    };

    updateCumulative("taste_50",  50, allDishes.size());
    updateCumulative("taste_100", 100, allDishes.size());
}
