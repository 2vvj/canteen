#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include <QDialog>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QColor>
#include <QString>
#include "dishdata.h"

class StarRatingWidget : public QWidget {
    Q_OBJECT
public:
    explicit StarRatingWidget(int maxStars = 5, QWidget *parent = nullptr);

    void setRating(int rating);
    int rating() const { return m_rating; }

    void confirmRating(int rating);
    void cancelPreview();
    void flashEffect();

signals:
    void ratingRequested(int proposedRating);
    void ratingChanged(int rating);

private slots:
    void onStarClicked();

private:
    void updateStarUI();
    void updateStarUIPreview(int previewRating);

    QVector<QPushButton*> starButtons;
    int m_maxStars;
    int m_rating;
    int m_pendingRating;
    bool m_previewing;
};

class HistoryItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit HistoryItemWidget(int dishId, const QString &dishName,
                               const QString &timestamp, const QColor &cardColor,
                               int initialRating, QWidget *parent = nullptr);

signals:
    void dishRated(int dishId, int rating);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onRatingRequested(int proposedRating);

private:
    int m_dishId;
    QString m_dishName;
    QColor m_cardColor;
    StarRatingWidget *m_starWidget;
};

class HistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit HistoryWindow(QWidget *parent = nullptr);

    void loadDishes(const QVector<Dish> &dishes, const UserProfile &user,
                    const QMap<QString, QString> &ratingDates = {});

signals:
    void dishRated(int dishId, int rating);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void handleDishRatingUpdate(int dishId, int newRating);

    QVBoxLayout *mainLayout;
    QVBoxLayout *listLayout;
};

#endif
