#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>

#include "fuzzysearch.h"
#include "sketchyui.h"

class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(const QVector<Dish> &allDishes,
                          UserProfile &user,
                          QWidget *parent = nullptr);

    QStringList activeTags() const;
    QVector<SearchResult> lastResults() const;
    void resetMealSession();

signals:
    void searchResultsChanged(const QVector<SearchResult> &results);
    void tagsChanged(const QStringList &tags);

private slots:
    void onSearch();
    void onRemoveTag();

private:
    void setupUI();
    void rebuildTagChips();

    QLineEdit *m_searchInput;
    SketchyButton *m_searchBtn;
    QListWidget *m_resultList;
    QWidget *m_tagChipArea;
    QHBoxLayout *m_tagChipLayout;
    QLabel *m_statusLabel;

    const QVector<Dish> &m_allDishes;
    UserProfile &m_user;
    FuzzySearch m_fuzzy;
    QVector<SearchResult> m_lastResults;
};

#endif
