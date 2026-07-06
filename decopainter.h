#ifndef DECOPAINTER_H
#define DECOPAINTER_H

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QPointF>
#include <QRectF>
#include <QFont>
namespace DecoPainter {

QColor titleBrown();
QColor textBrown();
QColor lightBrown();
QBrush paperBase();

void setHandwritingFont(QPainter &painter, int size, bool bold = false);

void drawPaperTexture(QPainter &painter, const QRect &rect);

QPainterPath makeOrganicRect(const QRectF &r, float radius, int seed = 42);
QPainterPath makeWavyRect(const QRectF &r, float amplitude);

void drawSketchyBorder(QPainter *p, const QPainterPath &path,
                       const QColor &ink, int passes, double spread);

void drawWatercolorSplotch(QPainter &painter, QPointF center, float size,
                           const QColor &color);

}

#endif
