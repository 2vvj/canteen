#ifndef CALENDARWINDOW_H
#define CALENDARWINDOW_H

#include <QDialog>
#include <QCalendarWidget>
#include <QPushButton>
#include <QLabel>
#include <QDate>
#include <QMap>
#include <QColor>
#include <QPoint>

struct DailyRecord {
    QStringList dishes;
    double totalCalories = 0;
    double totalPrice = 0;
};

class SketchyButton;

class ScratchyDivider : public QWidget {
    Q_OBJECT
public:
    explicit ScratchyDivider(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
};

class CalendarFrameWidget : public QWidget {
    Q_OBJECT
public:
    explicit CalendarFrameWidget(QCalendarWidget *calendar, QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QCalendarWidget *m_calendar;
};

class RecordCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit RecordCardWidget(QWidget *parent = nullptr);

    void setDateText(const QString &text);
    void setDishesText(const QString &text);
    void setCaloriesText(const QString &text);
    void setPriceText(const QString &text);
    void addButton(SketchyButton *btn);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_dateLabel;
    QLabel *m_dishesLabel;
    QLabel *m_caloriesLabel;
    QLabel *m_priceLabel;
    QColor m_cardColor;
};

class CalendarWindow : public QDialog {
    Q_OBJECT
public:
    explicit CalendarWindow(QWidget *parent = nullptr);

    void setRecords(const QMap<QString, DailyRecord> &records);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void onDateSelected(const QDate &date);
    void onStatisticsClicked();
    void onReportClicked();

private:
    void showRecordForDate(const QDate &date);

    QCalendarWidget *m_calendar;
    CalendarFrameWidget *m_calendarFrame;
    RecordCardWidget *m_recordCard;
    SketchyButton *m_closeBtn;
    QDate m_currentDate;
    QMap<QString, DailyRecord> m_records;
    QPoint m_dragPos;
    bool m_dragging = false;
};

#endif
