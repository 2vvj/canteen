#include "recommend.h"
#include <QtMath>

namespace Recommend {

double ucb1Score(const Dish &dish, int totalDraws) {
    // 利用最近一次评分（队友维护），冷启动默认3分
    double avgRating = dish.lastRating;
    int n = (dish.drawCount > 0) ? dish.drawCount : 1;
    int T = qMax(totalDraws, 1);

    double exploitation = avgRating;
    double exploration = 2.0 * qSqrt(qLn(T) / n);
    return exploitation + exploration;
}

QMap<QString, double> computeWeights(const QVector<Dish> &candidates,
                                      const UserProfile &user) {
    QMap<QString, double> weights;

    // 计算全局总抽取数 T
    int totalDraws = 0;
    for (const auto &d : candidates)
        totalDraws += d.drawCount;

    for (const auto &d : candidates) {
        double score = ucb1Score(d, totalDraws);

        // 反重复衰减（UCB1自带，但用户选的次数极多时额外降权）
        int chosen = user.chooseCount.value(d.name, 0);
        double repeatPenalty = (chosen > 3) ? qPow(0.8, chosen - 3) : 1.0;

        // 最终权重 = UCB1分数 × 重复衰减
        double weight = score * repeatPenalty;
        weights[d.name] = qMax(weight, 0.01);
    }

    // 米饭在主食中基础概率提升到50%
    double otherStapleWeight = 0;
    int riceStapleCount = 0;
    for (const auto &d : candidates) {
        if (d.role == STAPLE) {
            if (d.name == QString::fromUtf8("米饭"))
                riceStapleCount++;
            else
                otherStapleWeight += weights[d.name];
        }
    }
    if (riceStapleCount > 0 && otherStapleWeight > 0) {
        double perRiceWeight = otherStapleWeight / riceStapleCount;
        for (const auto &d : candidates) {
            if (d.role == STAPLE && d.name == QString::fromUtf8("米饭"))
                weights[d.name] = perRiceWeight;
        }
    }

    return weights;
}

} // namespace Recommend
