#include "mapview.h"
#include <QMouseEvent>
#include <QPoint>

MapView::MapView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
}

void MapView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lbuttonHeld = true;
        m_lbuttonPressScreenPos = event->globalPosition().toPoint();
        m_lbuttonIsClick = true;
    }
    QGraphicsView::mousePressEvent(event);
}

void MapView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_lbuttonHeld) {
        QPoint delta = event->globalPosition().toPoint() - m_lbuttonPressScreenPos;
        if (delta.manhattanLength() > CLICK_TOLERANCE)
            m_lbuttonIsClick = false;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void MapView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_lbuttonIsClick) {
            QPointF scenePos = mapToScene(event->pos());
            emit sceneLeftClicked(scenePos);
        }
        m_lbuttonHeld = false;
        m_lbuttonIsClick = false;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
