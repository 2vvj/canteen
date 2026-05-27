#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QMap>
#include "calendarwindow.h"

class LineChartWidget;
class BarChartWidget;

class StatisticsWindow : public QDialog {
    Q_OBJECT
public:
    explicit StatisticsWindow(QWidget *parent = nullptr);

    void setRecords(const QMap<QString, DailyRecord> &records);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void loadData();

    LineChartWidget *m_expenseChart;
    LineChartWidget *m_calorieChart;
    BarChartWidget *m_monthlyBarChart;
    QLabel *m_emptyLabel;
    QLabel *m_expenseAvgLabel;
    QLabel *m_calorieAvgLabel;
    QMap<QString, DailyRecord> m_records;
};

#endif
