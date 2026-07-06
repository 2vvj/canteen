#include "zoneeditor.h"
#include "zonemanager.h"
#include <QMouseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QPen>
#include <QBrush>

ZoneEditor::ZoneEditor(QGraphicsView *view, QGraphicsScene *scene,
                       ZoneManager *manager, QObject *parent)
    : QObject(parent), m_view(view), m_scene(scene), m_zoneManager(manager)
{
}

void ZoneEditor::setEditMode(bool enabled)
{
    if (m_editMode == enabled) return;
    m_editMode = enabled;

    if (enabled) {
        m_view->viewport()->installEventFilter(this);
        m_view->viewport()->setCursor(Qt::CrossCursor);
        refreshZoneOverlays();
    } else {
        m_view->viewport()->removeEventFilter(this);
        m_view->viewport()->unsetCursor();
        clearZoneOverlays();
    }
    emit editModeChanged(enabled);
}

bool ZoneEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_editMode)
        return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            QPointF sp = m_view->mapToScene(me->pos());
            startDrawing(sp);
            return true;
        } else if (me->button() == Qt::RightButton) {
            QPointF sp = m_view->mapToScene(me->pos());
            int zoneId = m_zoneManager->zoneAtPoint(sp);
            if (zoneId >= 0) {
                const ZoneInfo *zi = m_zoneManager->zoneInfo(zoneId);
                int ret = QMessageBox::question(m_view, "删除区域",
                    QString("删除区域 \"%1\" (ID: %2)?").arg(zi->name).arg(zoneId),
                    QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    m_zoneManager->removeZone(zoneId);
                    m_zoneManager->saveToFile("../zones.json");
                    refreshZoneOverlays();
                }
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseMove && m_drawing) {
        auto *me = static_cast<QMouseEvent *>(event);
        QPointF sp = m_view->mapToScene(me->pos());
        updateDrawing(sp);
        return true;
    } else if (event->type() == QEvent::MouseButtonRelease && m_drawing) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            QPointF sp = m_view->mapToScene(me->pos());
            finishDrawing(sp);
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}

void ZoneEditor::refreshZoneOverlays()
{
    clearZoneOverlays();
    const auto zones = m_zoneManager->allZones();
    for (const auto &z : zones) {
        auto *item = m_scene->addPolygon(z.polygon,
            QPen(QColor("#2B2B2B"), 1.0, Qt::DashLine),
            QBrush(QColor(200, 120, 90, 60)));
        item->setZValue(5);
        m_overlays.append(item);
    }
}

void ZoneEditor::clearZoneOverlays()
{
    for (auto *item : m_overlays)
        m_scene->removeItem(item);
    m_overlays.clear();
}

void ZoneEditor::startDrawing(QPointF scenePos)
{
    m_drawing = true;
    m_rubberBand = m_scene->addRect(QRectF(scenePos, QSizeF(0, 0)),
        QPen(QColor("#2B2B2B"), 1.5, Qt::DashLine),
        QBrush(QColor(200, 120, 90, 50)));
    m_rubberBand->setZValue(20);
}

void ZoneEditor::updateDrawing(QPointF scenePos)
{
    if (!m_rubberBand) return;
    QRectF r = m_rubberBand->rect();
    m_rubberBand->setRect(QRectF(r.topLeft(), scenePos).normalized());
}

void ZoneEditor::finishDrawing(QPointF scenePos)
{
    if (!m_rubberBand) return;
    QRectF r = m_rubberBand->rect();
    m_scene->removeItem(m_rubberBand);
    delete m_rubberBand;
    m_rubberBand = nullptr;
    m_drawing = false;

    QPolygonF poly = rectToPolygon(r.normalized());
    if (poly.size() < 3) return;

    bool ok = false;
    QString name = QInputDialog::getText(m_view, "新建区域",
        "区域名称:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString category = QInputDialog::getItem(m_view, "区域类别",
        "类别 (A 或 B):", {"A", "B"}, 0, false, &ok);
    if (!ok) return;

    m_zoneManager->addZone(name, category, poly);
    m_zoneManager->saveToFile("../zones.json");
    refreshZoneOverlays();
}

QPolygonF ZoneEditor::rectToPolygon(const QRectF &rect) const
{
    QPolygonF poly;
    poly << rect.topLeft() << rect.topRight()
         << rect.bottomRight() << rect.bottomLeft();
    return poly;
}
