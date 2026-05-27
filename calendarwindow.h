#ifndef CALENDARWINDOW_H
#define CALENDARWINDOW_H

#include <QDialog>
#include <QCalendarWidget>
#include <QLabel>
#include <QDate>
#include <QMap>

struct DailyRecord {
    QStringList dishes;
    double totalCalories = 0;
    double totalPrice = 0;
};

class CalendarWindow : public QDialog {
    Q_OBJECT
public:
    explicit CalendarWindow(QWidget *parent = nullptr);

    void setRecords(const QMap<QString, DailyRecord> &records);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onDateSelected(const QDate &date);
    void onStatisticsClicked();

private:
    void showRecordForDate(const QDate &date);

    QCalendarWidget *m_calendar;
    QLabel *m_dateLabel;
    QLabel *m_dishesLabel;
    QLabel *m_caloriesLabel;
    QLabel *m_priceLabel;
    QDate m_currentDate;
    QMap<QString, DailyRecord> m_records;
};

#endif
