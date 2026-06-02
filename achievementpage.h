#ifndef ACHIEVEMENTPAGE_H
#define ACHIEVEMENTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QMouseEvent>
#include "achievementdata.h"

class AchievementManager;

// 单张成就卡片（手绘风格）
class AchievementCard : public QWidget {
    Q_OBJECT
public:
    AchievementCard(const AchievementDef &def, const AchievementState &state,
                    bool isNew, bool isActive, bool showObese,
                    QWidget *parent = nullptr);
    QString key() const { return m_key; }

signals:
    void clicked(const QString &key);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QString m_key;
    bool m_unlocked;
    bool m_isActive;
    bool m_showObese;
};

class AchievementPage : public QWidget {
    Q_OBJECT
public:
    explicit AchievementPage(AchievementManager *mgr, QWidget *parent = nullptr);
    void refresh();
    void setIsObese(bool obese) { m_isObese = obese; }

signals:
    void backToMap();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void buildUI();
    void onCardClicked(const QString &key);

    AchievementManager *m_mgr;
    QWidget *m_gridContainer;
    QGridLayout *m_gridLayout;
    QLabel *m_currentSkinPreview;
    QLabel *m_currentSkinName;
    bool m_isObese = false;
};

#endif // ACHIEVEMENTPAGE_H
