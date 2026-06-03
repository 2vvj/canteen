#include "fuzzysearch.h"
#include <QtMath>
#include <algorithm>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static QMap<QString, QString> restaurantAbbrevs() {
    return {
        {"家二", "家园二层"},
        {"家一", "家园一层"},
        {"家三", "家园三层"},
        {"勺二", "勺园二层"},
        {"勺一", "勺园一层"},
        {"农二", "农园二层"},
        {"农一", "农园一层"},
    };
}

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
    QString ratingKey = dish.name + "|" + dish.restaurant;
    if (user.ratings.contains(ratingKey)) {
        double rating = user.ratings[ratingKey];
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

    QVector<SearchResult> results;
    results.reserve(allDishes.size());

    for (int i = 0; i < allDishes.size(); ++i) {
        const auto &d = allDishes[i];
        SearchResult sr;
        sr.dish = d;

        // 交集匹配：每个临时标签都必须通过名字或标签匹配
        bool matchesAll = true;
        bool hasNameMatch = false;
        double minNameScore = 1.0;
        int matchedTagCount = 0;

        for (const auto &tag : m_tempTags) {
            double dishSim = nameSimilarity(tag, d.name);
            double restSim = restaurantSimilarity(tag, d.restaurant);
            if (restSim == 0.0) {
                QString expanded = restaurantAbbrevs().value(tag);
                if (!expanded.isEmpty())
                    restSim = restaurantSimilarity(expanded, d.restaurant);
            }
            double bestName = qMax(dishSim, restSim);

            bool tagOk = false;
            QStringList resolved = resolveSynonyms(tag);
            if (!resolved.isEmpty()) {
                for (const auto &dtag : d.tags) {
                    if (resolved.contains(dtag, Qt::CaseInsensitive)) {
                        tagOk = true;
                        break;
                    }
                }
            } else {
                for (const auto &dtag : d.tags) {
                    if (dtag.contains(tag, Qt::CaseInsensitive)
                        || tag.contains(dtag, Qt::CaseInsensitive)) {
                        tagOk = true;
                        break;
                    }
                }
            }

            if (bestName == 0.0 && !tagOk) {
                matchesAll = false;
                break;
            }

            if (bestName > 0.0) {
                hasNameMatch = true;
                minNameScore = qMin(minNameScore, bestName);
            }
            if (tagOk) matchedTagCount++;
        }

        if (!matchesAll) {
            sr.nameScore = 0.0;
            sr.matchedTags = 0;
        } else {
            sr.nameScore = hasNameMatch ? minNameScore : 0.0;
            sr.matchedTags = matchedTagCount;
        }

        int tagCount = m_tempTags.size();
        double tagScore = (tagCount > 0) ? static_cast<double>(sr.matchedTags) / tagCount : 0.0;

        sr.score = sr.nameScore * 0.6 + tagScore * 0.4;
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
    return out;
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

    QVector<SearchResult> results;
    results.reserve(allDishes.size());

    for (int i = 0; i < allDishes.size(); ++i) {
        const auto &d = allDishes[i];
        SearchResult sr;
        sr.dish = d;

        // 交集匹配：每个临时标签都必须通过名字或标签匹配
        bool matchesAll = true;
        bool hasNameMatch = false;
        double minNameScore = 1.0;
        int matchedTagCount = 0;

        for (const auto &tag : m_tempTags) {
            // 名字/食堂匹配
            double dishSim = nameSimilarity(tag, d.name);
            double restSim = restaurantSimilarity(tag, d.restaurant);
            if (restSim == 0.0) {
                QString expanded = restaurantAbbrevs().value(tag);
                if (!expanded.isEmpty())
                    restSim = restaurantSimilarity(expanded, d.restaurant);
            }
            double bestName = qMax(dishSim, restSim);

            // 标签匹配（同义词 → 子串兜底）
            bool tagOk = false;
            QStringList resolved = resolveSynonyms(tag);
            if (!resolved.isEmpty()) {
                for (const auto &dtag : d.tags) {
                    if (resolved.contains(dtag, Qt::CaseInsensitive)) {
                        tagOk = true;
                        break;
                    }
                }
            } else {
                for (const auto &dtag : d.tags) {
                    if (dtag.contains(tag, Qt::CaseInsensitive)
                        || tag.contains(dtag, Qt::CaseInsensitive)) {
                        tagOk = true;
                        break;
                    }
                }
            }

            if (bestName == 0.0 && !tagOk) {
                matchesAll = false;
                break;
            }

            if (bestName > 0.0) {
                hasNameMatch = true;
                minNameScore = qMin(minNameScore, bestName);
            }
            if (tagOk) matchedTagCount++;
        }

        if (!matchesAll) {
            sr.nameScore = 0.0;
            sr.matchedTags = 0;
        } else {
            sr.nameScore = hasNameMatch ? minNameScore : 0.0;
            sr.matchedTags = matchedTagCount;
        }

        int tagCount = m_tempTags.size();
        double tagScore = (tagCount > 0) ? static_cast<double>(sr.matchedTags) / tagCount : 0.0;

        sr.score = sr.nameScore * 0.6 + tagScore * 0.4;
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
    return out;
}
