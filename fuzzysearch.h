#ifndef FUZZYSEARCH_H
#define FUZZYSEARCH_H

#include "dishdata.h"

struct SearchResult {
    Dish dish;
    double score = 0;
    double nameScore = 0;
    int matchedTags = 0;
};

class FuzzySearch {
public:
    FuzzySearch();

    bool loadSynonyms(const QString &filename);

    void buildTagIndex(const QVector<Dish> &dishes);

    QVector<SearchResult> search(const QString &query,
                                  const QVector<Dish> &allDishes,
                                  const UserProfile &user);

    QVector<SearchResult> rescoreWithCurrentTags(const QVector<Dish> &allDishes,
                                                  const UserProfile &user);

    void addTempTag(const QString &tag);

    void removeTempTag(const QString &tag);

    void clearTempTags();

    QStringList tempTags() const;

    static int levenshteinDistance(const QString &s1, const QString &s2);

private:
    static int maxAcceptableDist(const QString &query);

    static double nameSimilarity(const QString &s1, const QString &s2);

    static double restaurantSimilarity(const QString &query, const QString &restaurant);

    QStringList resolveSynonyms(const QString &word) const;

    double historyBonus(const Dish &dish, const UserProfile &user) const;

    QStringList m_tempTags;

    QMap<QString, QStringList> m_synonymToTags;

    QMap<QString, QVector<int>> m_tagIndex;
};

#endif
