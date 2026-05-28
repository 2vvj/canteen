#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QGraphicsView>
#include <QPoint>

class MapView : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapView(QGraphicsScene *scene, QWidget *parent = nullptr);

signals:
    void sceneLeftClicked(QPointF scenePos);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_lbuttonHeld = false;
    QPoint m_lbuttonPressScreenPos;
    bool m_lbuttonIsClick = false;

    static constexpr int CLICK_TOLERANCE = 4;
};

#endif
