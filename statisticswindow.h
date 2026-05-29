#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QMap>
#include <QColor>
#include "calendarwindow.h"

class LineChartWidget;
class BarChartWidget;

// 有机形状图表容器 — 手绘卡片包裹每个图表
class ChartCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartCardWidget(const QString &title, const QColor &accentColor,
                             QWidget *parent = nullptr);

    void setChartWidget(QWidget *chart);
    void setSummaryLabel(QLabel *label);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QColor m_accentColor;
    QColor m_cardColor;
};

class StatisticsWindow : public QDialog {
    Q_OBJECT
public:
    explicit StatisticsWindow(QWidget *parent = nullptr);

    void setRecords(const QMap<QString, DailyRecord> &records);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void loadData();

    ChartCardWidget *m_expenseCard;
    ChartCardWidget *m_calorieCard;
    ChartCardWidget *m_monthlyCard;
    LineChartWidget *m_expenseChart;
    LineChartWidget *m_calorieChart;
    BarChartWidget *m_monthlyBarChart;
    QLabel *m_emptyLabel;
    QLabel *m_expenseAvgLabel;
    QLabel *m_calorieAvgLabel;
    QMap<QString, DailyRecord> m_records;
};

#endif
