#ifndef ZONEEDITOR_H
#define ZONEEDITOR_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QVector>

class ZoneManager;

class ZoneEditor : public QObject {
    Q_OBJECT
public:
    explicit ZoneEditor(QGraphicsView *view, QGraphicsScene *scene,
                        ZoneManager *manager, QObject *parent = nullptr);

    void setEditMode(bool enabled);
    bool isEditMode() const { return m_editMode; }

signals:
    void editModeChanged(bool enabled);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refreshZoneOverlays();
    void clearZoneOverlays();
    void startDrawing(QPointF scenePos);
    void updateDrawing(QPointF scenePos);
    void finishDrawing(QPointF scenePos);
    QPolygonF rectToPolygon(const QRectF &rect) const;

    QGraphicsView  *m_view;
    QGraphicsScene *m_scene;
    ZoneManager    *m_zoneManager;
    bool m_editMode = false;

    bool m_drawing = false;
    QGraphicsRectItem *m_rubberBand = nullptr;

    QVector<QGraphicsPolygonItem*> m_overlays;
};

#endif
