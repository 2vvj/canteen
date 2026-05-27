#ifndef FUZZYSEARCH_H
#define FUZZYSEARCH_H

#include "dishdata.h"

struct SearchResult {
    Dish dish;
    double score = 0;       // 综合匹配分（名字60% + 标签40%）
    double nameScore = 0;   // 名字相似度
    int matchedTags = 0;    // 匹配到的标签数
};

class FuzzySearch {
public:
    FuzzySearch();

    // 加载标签同义词库（synonyms.json）
    bool loadSynonyms(const QString &filename);

    // 构建标签→菜品反向索引（加速匹配）
    void buildTagIndex(const QVector<Dish> &dishes);

    // 主搜索入口：多次调用会叠加临时标签权重
    QVector<SearchResult> search(const QString &query,
                                  const QVector<Dish> &allDishes,
                                  const UserProfile &user);

    // 仅用当前已有标签打分（不加新标签），用于删除标签后刷新结果
    QVector<SearchResult> rescoreWithCurrentTags(const QVector<Dish> &allDishes,
                                                  const UserProfile &user);

    // 添加一个临时标签（多次搜索叠加）
    void addTempTag(const QString &tag);

    // 删除单个临时标签
    void removeTempTag(const QString &tag);

    // 清空本次用餐的所有临时标签（用餐结束时调用）
    void clearTempTags();

    // 获取当前所有临时标签
    QStringList tempTags() const;

    // 编辑距离（公开，供队友复用）
    static int levenshteinDistance(const QString &s1, const QString &s2);

private:
    // 自适应阈值：≤3字容1，>3字容2
    static int maxAcceptableDist(const QString &query);

    // 编辑距离 → 相似度 (0.0 ~ 1.0)
    static double nameSimilarity(const QString &s1, const QString &s2);

    // 查同义词表：返回搜索词映射到的所有标签；空列表=未命中
    QStringList resolveSynonyms(const QString &word) const;

    // 历史偏好加权（常选/高分菜的加分）
    double historyBonus(const Dish &dish, const UserProfile &user) const;

    QStringList m_tempTags;  // 本次用餐叠加的临时标签

    // 同义词 → 标签 反向索引（从synonyms.json加载后构建）
    // 如："辣" → ["麻辣", "香辣", "酸辣"]
    QMap<QString, QStringList> m_synonymToTags;

    // 标签 → 菜品索引（加速同义词命中后的查找）
    // 如："麻辣" → [3, 7, 15, 22, ...]
    QMap<QString, QVector<int>> m_tagIndex;
};

#endif
