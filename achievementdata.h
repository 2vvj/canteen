#ifndef ACHIEVEMENTDATA_H
#define ACHIEVEMENTDATA_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

// 一条成就的静态元数据（硬编码，不存 JSON）
struct AchievementDef {
    QString key;
    QString name;
    QString description;
    QString skinSuffix;   // "streak_calorie_3" → lion_streak_calorie_3_slim.png
    int progressMax = 0;  // 0=一次性，>0=连续/累计目标值
};

// 一条成就的运行时状态（存入 achievements.json）
struct AchievementState {
    bool unlocked = false;
    QString unlockDate;
    int progress = 0;
};

// 全部成就状态 + 活跃皮肤
struct AchievementData {
    QMap<QString, AchievementState> states;
    QString activeSkin = "first_record";
    QStringList newlyUnlocked;

    QJsonObject toJson() const;
    static AchievementData fromJson(const QJsonObject &root);
};

// 10条成就的元数据定义
inline QVector<AchievementDef> achievementDefs() {
    return {
        {"first_record",     "初来乍到",   "首次完成饮食记录",           "first_record",     0},
        {"streak_calorie_3", "自律干饭人", "连续3天卡路里不超标",         "streak_calorie_3", 3},
        {"streak_calorie_7", "铁律干饭人", "连续7天卡路里不超标",         "streak_calorie_7", 7},
        {"streak_record_7",  "全勤吃货",   "连续7天有饮食记录",           "streak_record_7",  7},
        {"streak_over_3",    "暴食魔王",   "连续3天卡路里超标",           "streak_over_3",    3},
        {"streak_over_7",    "热量暴君",   "连续7天卡路里超标",           "streak_over_7",    7},
        {"taste_50",         "味蕾收藏家", "累计品尝50种不同菜品",         "taste_50",        50},
        {"taste_100",        "百味大师",   "累计品尝100种不同菜品",        "taste_100",      100},
        {"late_meal",        "深夜食堂",   "晚上21:00后完成饮食记录",      "late_meal",        0},
        {"luxury",           "豪华大餐王", "单餐消费超过50元",             "luxury",           0},
    };
}

// ── JSON 序列化实现 ──

inline QJsonObject AchievementData::toJson() const {
    QJsonObject root;
    QJsonObject achObj;
    for (auto it = states.begin(); it != states.end(); ++it) {
        QJsonObject s;
        s["unlocked"] = it->unlocked;
        if (!it->unlockDate.isEmpty()) s["unlockDate"] = it->unlockDate;
        if (!it->unlocked && it->progress > 0) s["progress"] = it->progress;
        achObj[it.key()] = s;
    }
    root["achievements"] = achObj;
    root["activeSkin"] = activeSkin;
    QJsonArray arr;
    for (const auto &n : newlyUnlocked) arr.append(n);
    root["newlyUnlocked"] = arr;
    return root;
}

inline AchievementData AchievementData::fromJson(const QJsonObject &root) {
    AchievementData data;
    QJsonObject achObj = root["achievements"].toObject();
    const auto defs = achievementDefs();
    for (const auto &d : defs) {
        AchievementState st;
        if (achObj.contains(d.key)) {
            QJsonObject s = achObj[d.key].toObject();
            st.unlocked = s["unlocked"].toBool();
            st.unlockDate = s["unlockDate"].toString();
            st.progress = s["progress"].toInt();
        }
        data.states[d.key] = st;
    }
    data.activeSkin = root["activeSkin"].toString("first_record");
    for (const auto &v : root["newlyUnlocked"].toArray())
        data.newlyUnlocked.append(v.toString());
    return data;
}

#endif // ACHIEVEMENTDATA_H
