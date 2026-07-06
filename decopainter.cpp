#include "decopainter.h"
#include <QtMath>
#include <QRandomGenerator>

namespace DecoPainter {

QColor titleBrown()  { return QColor("#5D4B3A"); }
QColor textBrown()   { return QColor("#8B7B6A"); }
QColor lightBrown()  { return QColor("#C4B8A8"); }
QBrush paperBase()   { return QBrush(QColor("#FAF8F5")); }

void setHandwritingFont(QPainter &painter, int size, bool bold) {
    QFont f("Microsoft YaHei", size);
    if (bold) f.setBold(true);
    painter.setFont(f);
}

void drawPaperTexture(QPainter &painter, const QRect &rect) {
    painter.save();
    QColor dots("#D8D0C8");
    for (int i = 0; i < 40; ++i) {
        int x = rect.left() + QRandomGenerator::global()->bounded(rect.width());
        int y = rect.top() + QRandomGenerator::global()->bounded(rect.height());
        int r = QRandomGenerator::global()->bounded(2);
        dots.setAlpha(20 + QRandomGenerator::global()->bounded(30));
        painter.setPen(Qt::NoPen);
        painter.setBrush(dots);
        painter.drawEllipse(QPointF(x, y), r, r);
    }
    painter.restore();
}

static double jitter(int seed, int idx, double amp) {
    int s = seed + idx * 2654435761u;
    s ^= (s >> 16);
    s *= 0x45d9f3b;
    s ^= (s >> 16);
    double v = static_cast<double>(qAbs(s) % 10000) / 10000.0;
    return (v - 0.5) * 2.0 * amp;
}

QPainterPath makeOrganicRect(const QRectF &r, float radius, int seed) {
    QPainterPath path;
    double x0 = r.left(), y0 = r.top(), x1 = r.right(), y1 = r.bottom();
    double j = radius * 0.4;
    Q_UNUSED(radius);

    double cx0 = x0 + jitter(seed, 0, j);
    double cy0 = y0 + jitter(seed, 1, j);
    double cx1 = x1 + jitter(seed, 2, j);
    double cy1 = y0 + jitter(seed, 3, j);
    double cx2 = x1 + jitter(seed, 4, j);
    double cy2 = y1 + jitter(seed, 5, j);
    double cx3 = x0 + jitter(seed, 6, j);
    double cy3 = y1 + jitter(seed, 7, j);

    path.moveTo(cx0, cy0);
    path.lineTo(cx1, cy1);
    path.lineTo(cx2, cy2);
    path.lineTo(cx3, cy3);
    path.closeSubpath();
    return path;
}

QPainterPath makeWavyRect(const QRectF &r, float amplitude) {
    QPainterPath path;
    double x0 = r.left(), y0 = r.top(), x1 = r.right(), y1 = r.bottom();
    int steps = 8;
    path.moveTo(x0, y0 + qSin(0.0) * amplitude);
    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        path.lineTo(x0 + (x1 - x0) * t, y0 + qSin(t * 6.28) * amplitude);
    }
    for (int i = 1; i <= steps / 2; ++i) {
        double t = static_cast<double>(i) / (steps / 2);
        path.lineTo(x1 + qSin(t * 3.14) * amplitude * 0.5, y0 + (y1 - y0) * t);
    }
    for (int i = steps; i >= 0; --i) {
        double t = static_cast<double>(i) / steps;
        path.lineTo(x0 + (x1 - x0) * t, y1 + qSin(t * 6.28 + 1.57) * amplitude);
    }
    for (int i = steps / 2; i >= 0; --i) {
        double t = static_cast<double>(i) / (steps / 2);
        path.lineTo(x0 + qSin(t * 3.14 + 3.14) * amplitude * 0.5, y0 + (y1 - y0) * t);
    }
    path.closeSubpath();
    return path;
}

void drawSketchyBorder(QPainter *p, const QPainterPath &path,
                       const QColor &ink, int passes, double spread) {
    QPen pen(ink);
    pen.setWidthF(1.0);
    pen.setCapStyle(Qt::RoundCap);
    p->setBrush(Qt::NoBrush);
    for (int i = 0; i < passes; ++i) {
        double ox = (i - (passes - 1) / 2.0) * spread;
        double oy = ox * 0.7;
        p->save();
        p->translate(ox, oy);
        pen.setWidthF(0.8 + (i % 2) * 0.6);
        p->setPen(pen);
        p->drawPath(path);
        p->restore();
    }
}

void drawWatercolorSplotch(QPainter &painter, QPointF center, float size,
                           const QColor &color) {
    painter.save();
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, size, size * 0.7);
    painter.drawEllipse(QPointF(center.x() + size * 0.3, center.y() - size * 0.1),
                        size * 0.6, size * 0.5);
    painter.drawEllipse(QPointF(center.x() - size * 0.2, center.y() + size * 0.2),
                        size * 0.5, size * 0.4);
    painter.restore();
}

}
