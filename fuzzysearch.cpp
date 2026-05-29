#include "fuzzysearch.h"
#include <QtMath>
#include <algorithm>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

FuzzySearch::FuzzySearch() {}

// ============ 同义词 ============

bool FuzzySearch::loadSynonyms(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    m_synonymToTags.clear();
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString tag = it.key();           // 标签名，如 "麻辣"
        QJsonArray arr = it.value().toArray();
        for (const auto &v : arr) {
            QString syn = v.toString();   // 同义词，如 "辣"
            m_synonymToTags[syn].append(tag);
        }
    }
    return true;
}

QStringList FuzzySearch::resolveSynonyms(const QString &word) const {
    return m_synonymToTags.value(word);  // 空列表 = 未命中
}

void FuzzySearch::buildTagIndex(const QVector<Dish> &dishes) {
    m_tagIndex.clear();
    for (int i = 0; i < dishes.size(); ++i) {
        for (const auto &tag : dishes[i].tags) {
            m_tagIndex[tag].append(i);
        }
    }
}

// ============ 编辑距离 ============

int FuzzySearch::levenshteinDistance(const QString &s1, const QString &s2) {
    int m = s1.length(), n = s2.length();
    QVector<int> prev(n + 1), curr(n + 1);
    for (int j = 0; j <= n; ++j) prev[j] = j;

    for (int i = 1; i <= m; ++i) {
        curr[0] = i;
        for (int j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            curr[j] = qMin(qMin(prev[j] + 1, curr[j - 1] + 1),
                           prev[j - 1] + cost);
        }
        qSwap(prev, curr);
    }
    return prev[n];
}

int FuzzySearch::maxAcceptableDist(const QString &query) {
    return (query.length() <= 3) ? 1 : 2;
}

double FuzzySearch::nameSimilarity(const QString &s1, const QString &s2) {
    if (s1.isEmpty() && s2.isEmpty()) return 1.0;
    if (s1.isEmpty() || s2.isEmpty()) return 0.0;

    QString a = s1.toLower(), b = s2.toLower();

    // 完全匹配
    if (a == b) return 1.0;

    // 包含关系（短边至少2字，避免"面"误匹配"面包"）
    int minLen = qMin(a.length(), b.length());
    if (minLen >= 2 && (a.contains(b) || b.contains(a))) return 0.9;

    int dist = levenshteinDistance(a, b);
    int maxLen = qMax(a.length(), b.length());
    double sim = 1.0 - static_cast<double>(dist) / maxLen;

    // 超过自适应阈值则归零
    int threshold = maxAcceptableDist(s1);
    if (dist > threshold) return 0.0;

    return sim;
}

double FuzzySearch::restaurantSimilarity(const QString &query, const QString &restaurant) {
    if (query.isEmpty() || restaurant.isEmpty()) return 0.0;

    QString q = query.toLower(), r = restaurant.toLower();

    if (q == r) return 1.0;

    // 单向包含：查询被餐厅名完整包含（查询≥2字）
    if (q.length() >= 2 && r.contains(q)) return 0.9;

    return 0.0;
}

// ============ 历史偏好加权 ============

double FuzzySearch::historyBonus(const Dish &dish, const UserProfile &user) const {
    double bonus = 0.0;

    // 用户评分高的菜加分（最多+0.15）
    if (user.ratings.contains(dish.name)) {
        double rating = user.ratings[dish.name];
        bonus += (rating / 10.0) * 0.15;
    }

    // 常选的菜微加分（最多+0.1）
    if (user.chooseCount.contains(dish.name)) {
        int count = user.chooseCount[dish.name];
        bonus += qMin(static_cast<double>(count) / 20.0, 0.1);
    }

    return bonus;
}

// ============ 临时标签管理 ============

void FuzzySearch::addTempTag(const QString &tag) {
    if (!m_tempTags.contains(tag, Qt::CaseInsensitive))
        m_tempTags.append(tag);
}

void FuzzySearch::removeTempTag(const QString &tag) {
    m_tempTags.removeAll(tag);
}

void FuzzySearch::clearTempTags() {
    m_tempTags.clear();
}

QVector<SearchResult> FuzzySearch::rescoreWithCurrentTags(
    const QVector<Dish> &allDishes, const UserProfile &user) {

    if (m_tempTags.isEmpty()) {
        QVector<SearchResult> all;
        for (const auto &d : allDishes)
            all.append({d, 0.0, 0.0, 0});
        return all;
    }

    // 预计算标签命中数（使用反向索引）
    QMap<int, int> tagHits;
    for (const auto &t : m_tempTags) {
        QStringList resolved = resolveSynonyms(t);
        if (!resolved.isEmpty()) {
            for (const auto &rtag : resolved) {
                for (int idx : m_tagIndex.value(rtag))
                    tagHits[idx]++;
            }
        }
    }

    QVector<SearchResult> results;
    results.reserve(allDishes.size());

    for (int i = 0; i < allDishes.size(); ++i) {
        const auto &d = allDishes[i];
        SearchResult sr;
        sr.dish = d;

        // 名字匹配：用所有标签分别匹配菜名和食堂名，取最高分
        sr.nameScore = 0;
        for (const auto &tag : m_tempTags) {
            double dishSim = nameSimilarity(tag, d.name);
            double restSim = restaurantSimilarity(tag, d.restaurant);
            sr.nameScore = qMax(sr.nameScore, qMax(dishSim, restSim));
        }

        // 标签匹配
        sr.matchedTags = tagHits.value(i, 0);

        // 兜底：对未命中同义词的临时标签做子串匹配
        for (const auto &t : m_tempTags) {
            if (resolveSynonyms(t).isEmpty()) {
                for (const auto &dtag : d.tags) {
                    if (dtag.contains(t, Qt::CaseInsensitive)
                        || t.contains(dtag, Qt::CaseInsensitive))
                        sr.matchedTags++;
                }
            }
        }
        int totalRelevant = qMax(d.tags.size() + m_tempTags.size(), 1);
        double tagScore = static_cast<double>(sr.matchedTags) / totalRelevant;

        sr.score = sr.nameScore * 0.6 + tagScore * 0.4;
        // 只有搜索真正匹配到的菜才加上历史偏好加分
        if (sr.nameScore > 0.0 || sr.matchedTags > 0)
            sr.score += historyBonus(d, user);
        results.append(sr);
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  return a.score > b.score;
              });

    QVector<SearchResult> out;
    for (const auto &r : results) {
        if (r.nameScore > 0.0 || r.matchedTags > 0) out.append(r);
    }
    return out.isEmpty() ? results : out;
}

QStringList FuzzySearch::tempTags() const {
    return m_tempTags;
}

// ============ 主搜索 ============

QVector<SearchResult> FuzzySearch::search(const QString &query,
                                           const QVector<Dish> &allDishes,
                                           const UserProfile &user) {
    QString q = query.trimmed();
    if (q.isEmpty()) {
        // 空搜索返回所有菜
        QVector<SearchResult> all;
        for (const auto &d : allDishes) {
            all.append({d, 0.0, 0.0, 0});
        }
        return all;
    }

    // 先把关键词本身加入临时标签
    addTempTag(q);

    // 预计算标签命中数（使用反向索引加速同义词路径）
    QMap<int, int> tagHits;  // dish索引 → 命中次数

    QStringList resolved = resolveSynonyms(q);
    if (!resolved.isEmpty()) {
        for (const auto &rtag : resolved) {
            for (int idx : m_tagIndex.value(rtag))
                tagHits[idx]++;
        }
    }
    // 处理临时标签（同样逻辑）
    for (const auto &t : m_tempTags) {
        QStringList tresolved = resolveSynonyms(t);
        if (!tresolved.isEmpty()) {
            for (const auto &rtag : tresolved) {
                for (int idx : m_tagIndex.value(rtag))
                    tagHits[idx]++;
            }
        }
    }

    QVector<SearchResult> results;
    results.reserve(allDishes.size());

    for (int i = 0; i < allDishes.size(); ++i) {
        const auto &d = allDishes[i];
        SearchResult sr;
        sr.dish = d;

        // 名字匹配：菜名和食堂名都算，取较高分
        double dishNameSim = nameSimilarity(q, d.name);
        double restSim = restaurantSimilarity(q, d.restaurant);
        sr.nameScore = qMax(dishNameSim, restSim);

        // 标签匹配
        sr.matchedTags = tagHits.value(i, 0);

        // 如果同义词没命中，走兜底子串匹配
        if (resolved.isEmpty()) {
            for (const auto &dtag : d.tags) {
                if (q.contains(dtag, Qt::CaseInsensitive) || dtag.contains(q, Qt::CaseInsensitive))
                    sr.matchedTags++;
            }
        }
        for (const auto &t : m_tempTags) {
            if (resolveSynonyms(t).isEmpty()) {
                for (const auto &dtag : d.tags) {
                    if (dtag.contains(t, Qt::CaseInsensitive) || t.contains(dtag, Qt::CaseInsensitive))
                        sr.matchedTags++;
                }
            }
        }

        // 归一化标签分
        int totalRelevant = qMax(d.tags.size() + m_tempTags.size(), 1);
        double tagScore = static_cast<double>(sr.matchedTags) / totalRelevant;

        // 综合打分：名字60% + 标签40%
        sr.score = sr.nameScore * 0.6 + tagScore * 0.4;

        // 只有真正匹配到的菜才加历史偏好
        if (sr.nameScore > 0.0 || sr.matchedTags > 0)
            sr.score += historyBonus(d, user);

        results.append(sr);
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  return a.score > b.score;
              });

    QVector<SearchResult> out;
    for (const auto &r : results) {
        if (r.nameScore > 0.0 || r.matchedTags > 0)
            out.append(r);
    }
    return out.isEmpty() ? results : out;
}
