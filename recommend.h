#ifndef RECOMMEND_H
#define RECOMMEND_H

#include "dishdata.h"
#include <QMap>
namespace Recommend {

QMap<QString, double> computeWeights(const QVector<Dish> &candidates,
                                      const UserProfile &user);

double ucb1Score(const Dish &dish, int totalDraws);

}

#endif
