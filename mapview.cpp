#include "mapview.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPoint>

MapView::MapView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setFocusPolicy(Qt::StrongFocus);
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

void MapView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        scale(factor, factor);
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void MapView::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Equal || event->key() == Qt::Key_Plus) {
            scale(1.15, 1.15);
            return;
        }
        if (event->key() == Qt::Key_Minus) {
            scale(1.0 / 1.15, 1.0 / 1.15);
            return;
        }
        if (event->key() == Qt::Key_0) {
            resetTransform();
            double s = viewport()->height() / sceneRect().height();
            scale(s, s);
            centerOn(sceneRect().center());
            return;
        }
    }
    QGraphicsView::keyPressEvent(event);
}
