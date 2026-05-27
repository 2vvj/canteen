#ifndef RECOMMEND_H
#define RECOMMEND_H

#include "dishdata.h"
#include <QMap>

// 推荐算法（纯后台，不展示给用户）
namespace Recommend {

// 计算每个候选菜的最终抽卡权重
// candidates: 待抽的候选菜品
// 返回: 菜名 → 最终权重
QMap<QString, double> computeWeights(const QVector<Dish> &candidates,
                                      const UserProfile &user);

// UCB1 单项得分
double ucb1Score(const Dish &dish, int totalDraws);

} // namespace Recommend

#endif
