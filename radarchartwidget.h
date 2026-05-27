#ifndef RADARCHARTWIDGET_H
#define RADARCHARTWIDGET_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QPointF>

struct RadarData {
    float taste = 80;
    float price = 70;
    float experience = 85;
    float calories = 60;
    float distance = 90;

    // 实际值（标注用）
    float actualTaste = 0;
    float actualPrice = 0;
    float actualExperience = 0;
    float actualCalories = 0;
    float actualDistance = 0;
    bool showActualValues = false;

    RadarData() = default;
    RadarData(float t, float p, float e, float c, float d)
        : taste(t), price(p), experience(e), calories(c), distance(d) {}
};

class RadarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadarChartWidget(QWidget *parent = nullptr);

    void setData(const RadarData &data, const QString &itemName);
    void setMainColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPointF calculatePoint(const QPointF &center, float radius, float angleDegrees);

    QStringList m_labels;
    QColor m_mainColor;
    RadarData m_data;
    QString m_itemName;
};

#endif
