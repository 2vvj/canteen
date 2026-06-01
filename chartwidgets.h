#ifndef CHARTWIDGETS_H
#define CHARTWIDGETS_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QColor>
#include <QRectF>

class LineChartWidget : public QWidget {
    Q_OBJECT
public:
    struct DataPoint {
        QString label;
        double value = 0;
    };

    explicit LineChartWidget(QWidget *parent = nullptr);

    void setData(const QVector<DataPoint> &points, const QString &title,
                 const QColor &lineColor);
    void setYAxisLabel(const QString &label);
    void setAverageLine(double value, const QColor &color);

signals:
    void swiped(int direction);  // +1 = 右滑看更新的, -1 = 左滑看更早的

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF calcChartArea() const;
    QVector<DataPoint> m_points;
    QString m_title;
    QString m_yAxisLabel;
    QColor m_lineColor;
    double m_avgValue = -1;
    QColor m_avgColor;
    int m_hoveredIndex = -1;
    QPointF m_pressPos;
    bool m_pressing = false;
};

class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    struct BarData {
        QString label;
        double value = 0;
    };

    explicit BarChartWidget(QWidget *parent = nullptr);

    void setData(const QVector<BarData> &bars, const QString &title);
    void setAverageLine(double value, const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF calcChartArea() const;
    QVector<BarData> m_bars;
    QString m_title;
    double m_avgValue = -1;
    QColor m_avgColor;
    int m_hoveredIndex = -1;
};

#endif
