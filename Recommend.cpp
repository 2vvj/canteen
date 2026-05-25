#include "Recommend.h"
#include <QtMath>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <algorithm>

namespace Recommend {

double ucb1Score(const Dish &dish, int totalDraws) {
    // 冷启动：新菜给默认评分 3.0
    double avgRating = (dish.drawCount > 0) ? dish.avgRating() : 3.0;
    int n = (dish.drawCount > 0) ? dish.drawCount : 1;
    int T = qMax(totalDraws, 1);

    // 利用项：平均评分
    double exploitation = avgRating;
    // 探索项：UCB1 置信上界，C = 2.0
    double exploration = 2.0 * qSqrt(qLn(static_cast<double>(T)) / n);
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

        // 反重复衰减：一道菜被选太多次则降权
        int chosen = user.chooseCount.value(d.name, 0);
        double repeatPenalty = (chosen > 3) ? qPow(0.8, chosen - 3) : 1.0;

        // 最终权重 = UCB1 分数 × 反重复衰减
        double weight = score * repeatPenalty;
        // 保证下界，防止权重归零导致永远抽不到
        weights[d.name] = qMax(weight, 0.01);
    }

    return weights;
}

bool recalculateAllWeights() {
    // 1. 从数据库加载全部菜品
    QVector<Dish> dishes = DishData::loadDishes();
    if (dishes.isEmpty()) {
        qDebug() << "[Recommend] 无菜品数据，跳过权重计算";
        return false;
    }

    // 2. 加载用户画像
    UserProfile user = DishData::loadUserProfile();

    // 3. 调用 UCB1 计算权重
    QMap<QString, double> weights = computeWeights(dishes, user);

    // 4. 逐个写回数据库
    QSqlQuery query;
    query.prepare("UPDATE Dishes SET current_weight = ? WHERE name = ?");

    for (auto it = weights.begin(); it != weights.end(); ++it) {
        query.addBindValue(it.value());
        query.addBindValue(it.key());
        if (!query.exec()) {
            qDebug() << "[Recommend] 权重更新失败:" << it.key()
                     << query.lastError().text();
            return false;
        }
    }

    qDebug() << "[Recommend] 全量权重已更新，共" << weights.size() << "道菜";
    return true;
}

} // namespace Recommend
