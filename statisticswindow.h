#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QMap>
#include <QColor>
#include <QPoint>
#include "calendarwindow.h"

class SketchyButton;
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
    void setDateRange(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QColor m_accentColor;
    QColor m_cardColor;
    QLabel *m_rangeLabel = nullptr;
};

class StatisticsWindow : public QDialog {
    Q_OBJECT
public:
    explicit StatisticsWindow(QWidget *parent = nullptr);

    void setRecords(const QMap<QString, DailyRecord> &records);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void loadData();
    void loadExpenseChart();
    void loadCalorieChart();
    void onExpenseSwiped(int direction);
    void onCalorieSwiped(int direction);

    ChartCardWidget *m_expenseCard;
    ChartCardWidget *m_calorieCard;
    ChartCardWidget *m_monthlyCard;
    LineChartWidget *m_expenseChart;
    LineChartWidget *m_calorieChart;
    BarChartWidget *m_monthlyBarChart;
    QLabel *m_emptyLabel;
    QMap<QString, DailyRecord> m_records;
    SketchyButton *m_closeBtn;
    QPoint m_dragPos;
    bool m_dragging = false;
    int m_expenseOffset = 0;
    int m_calorieOffset = 0;
};

#endif
