#ifndef GACHAWIDGET_H
#define GACHAWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMap>
#include <QPixmap>
#include "dishdata.h"

struct Particle {
    double x, y;  // x=alpha, y=radius
};

struct Sparkle {
    double x, y, size, alpha;
};

class GachaWidget : public QWidget {
    Q_OBJECT

public:
    explicit GachaWidget(QWidget *parent = nullptr);

    void startDraw(const QVector<Dish> &candidates,
                   const QMap<QString, double> &weights);

signals:
    void dishSelected(const Dish &dish);
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onBurstTick();
    void onSteamTick();

private:
    void pickWinner();
    void startBurst();

    enum State { DROP, IDLE, DRAGGING, BURST, REVEAL };
    State m_state;

    QVector<Dish> m_candidates;
    QMap<QString, double> m_weights;
    Dish m_winner;

    double m_bagY;
    double m_ribbonPull;
    QPoint m_lastMousePos;
    bool m_isDragging;

    int m_burstFrame;
    int m_particleFrame;
    QTimer *m_burstTimer;
    QTimer *m_particleTimer;
    QTimer *m_idleTimer;
    QPixmap m_bento;
    QPixmap m_ribbon;
    QVector<Particle> m_particles;
    QVector<Sparkle> m_sparkles;

    QMediaPlayer *m_gachaSfx;
    QAudioOutput *m_gachaSfxOutput;

    double m_revealAlpha;
    double m_idleBob;
    bool m_showCloseBtn;

    Q_PROPERTY(double bagY READ bagY WRITE setBagY)
    Q_PROPERTY(double ribbonPull READ ribbonPull WRITE setRibbonPull)
    Q_PROPERTY(double dragProgress READ dragProgress WRITE setDragProgress)
    double bagY() const;
    void setBagY(double y);
    double ribbonPull() const;
    void setRibbonPull(double v);
    double dragProgress() const;
    void setDragProgress(double p);
};

#endif
