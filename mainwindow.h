#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QStackedWidget>
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QTimer>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QPushButton>

#include "dishdata.h"
#include "userdata.h"
#include "sketchyui.h"
#include "mapview.h"
#include "radarchartwidget.h"
#include "calendarwindow.h"

class ZoneManager;
class ZoneEditor;
class DistanceDB;
class MealPage;
class HistoryWindow;
struct ZoneInfo;

// 漫游小狮子
class CharacterItem : public QGraphicsObject {
    Q_OBJECT
public:
    CharacterItem(const QString &spritePath, int size, QGraphicsItem *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void advanceWalkPhase() { m_walkPhase += 0.18; update(); }
    void resetWalkPhase() { m_walkPhase = 0; update(); }
    void setSprite(const QString &path);
private:
    QPixmap m_sprite;
    int m_size;
    double m_walkPhase = 0;
};

// 欢迎页
class WelcomePage : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double fadeIn READ fadeIn WRITE setFadeIn)
public:
    explicit WelcomePage(QWidget *parent = nullptr);
    double fadeIn() const { return m_fadeIn; }
    void setFadeIn(double v);
protected:
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *e) override;
private:
    double m_fadeIn = 0;
    QPropertyAnimation *m_fadeAnim;
    QPixmap m_bgPixmap;
};

// 侧边栏 — 4个按钮
class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);
    void setAvatar(const QPixmap &pixmap);
    void setUserName(const QString &name);
    void setTodayCalories(int kcal);
    void setBMR(int bmr);
signals:
    void goEatClicked();
    void historyClicked();
    void reviewClicked();
    void settingsClicked();
protected:
    void paintEvent(QPaintEvent *e) override;
private:
    SketchyButton *m_cardBtn;
    SketchyButton *m_historyBtn;
    SketchyButton *m_reviewBtn;
    SketchyButton *m_settingsBtn;
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_calorieLabel;
    QLabel *m_bmrLabel;
    int m_todayCalories = 0;
};

// 雷达确认对话框
class ReviewDialog : public QDialog {
    Q_OBJECT
public:
    ReviewDialog(const QVector<Dish> &dishes, int userZoneId,
                 ZoneManager *zoneMgr, DistanceDB *distDB,
                 const UserSettings &settings,
                 QWidget *parent = nullptr);
    bool isConfirmed() const { return m_confirmed; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    int findZoneForRestaurant(const QString &restaurant) const;
    double getDistance(int zoneA, int zoneB) const;

    RadarChartWidget *m_radar;
    QLabel *m_summaryLabel;
    bool m_confirmed = false;
    ZoneManager *m_zoneManager;
    DistanceDB *m_distanceDB;
    int m_userZoneId;
    UserSettings m_userData;
    QPoint m_dragPos;
    bool m_dragging = false;
};

// 主窗口
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void enterMap();
    void tickCharacter();
    void onGoEat();
    void onHistory();
    void onReview();
    void onSettings();
    void onSceneLeftClicked(QPointF scenePos);
    void onToggleEditMode();
    void onMealReadyForReview(const QVector<Dish> &selected);
    void onFinishEating();
    void backFromMeal();

private:
    void setupMapPage();
    void showNoZoneDialog();
    void applyUserSettings();
    void updateSidebarUserInfo();
    void updateLionSprite();
    void loadDailyRecords();
    void saveDailyRecords();
    void saveRatingsToUserFile();
    void loadEatingTimes();
    void saveEatingTimes();

    QStackedWidget *m_stack;
    WelcomePage *m_welcomePage;
    QWidget *m_mapPage;
    MapView *m_mapView;
    QGraphicsScene *m_mapScene;
    CharacterItem *m_character;
    Sidebar *m_sidebar;
    QTimer *m_roamTimer;
    QTimer *m_welcomeTimer;
    QPointF m_targetPos;
    bool m_roaming = true;

    ZoneManager *m_zoneManager;
    ZoneEditor *m_zoneEditor;
    DistanceDB *m_distanceDB;
    UserData m_userData;
    bool m_editMode = false;
    int m_selectedZoneId = -1;
    int m_userZoneId = -1;  // 用户点击地图选择的区域

    // 用餐系统
    MealPage *m_mealPage;
    QVector<Dish> m_allDishes;
    UserProfile m_userProfile;
    QPushButton *m_finishEatingBtn;
    bool m_mealActive = false;
    QVector<Dish> m_currentMealDishes;
    QMap<QString, DailyRecord> m_dailyRecords;
    QMap<QString, QString> m_ratingDates;  // 菜名 -> 评分日期
    QMap<QString, QString> m_eatingTimes;  // 菜名 -> 食用时间 "yyyy-MM-dd HH:mm"
};

#endif
