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

// ===== 有机矩形 =====
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
    // Top
    path.moveTo(x0, y0 + qSin(0.0) * amplitude);
    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        path.lineTo(x0 + (x1 - x0) * t, y0 + qSin(t * 6.28) * amplitude);
    }
    // Right
    for (int i = 1; i <= steps / 2; ++i) {
        double t = static_cast<double>(i) / (steps / 2);
        path.lineTo(x1 + qSin(t * 3.14) * amplitude * 0.5, y0 + (y1 - y0) * t);
    }
    // Bottom
    for (int i = steps; i >= 0; --i) {
        double t = static_cast<double>(i) / steps;
        path.lineTo(x0 + (x1 - x0) * t, y1 + qSin(t * 6.28 + 1.57) * amplitude);
    }
    // Left
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

void drawScratchyLine(QPainter &painter, QPointF start, QPointF end,
                      const QColor &color, float width, float wobble) {
    painter.save();
    QPen pen(color);
    pen.setWidthF(width);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    double len = QLineF(start, end).length();
    int steps = static_cast<int>(len / 3.0);
    QPointF prev = start;
    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double jx = qSin(t * 12.0 + QRandomGenerator::global()->bounded(1.0)) * wobble;
        double jy = qCos(t * 8.0 + QRandomGenerator::global()->bounded(1.0)) * wobble;
        QPointF pt(start.x() + (end.x() - start.x()) * t + jx,
                   start.y() + (end.y() - start.y()) * t + jy);
        painter.drawLine(prev, pt);
        prev = pt;
    }
    painter.restore();
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

// ===== 装饰图案 =====

void drawSakura(QPainter &painter, QPointF center, float size) {
    painter.save();
    painter.setBrush(QColor(255, 210, 210, 120));
    painter.setPen(QPen(QColor(220, 160, 160, 80), 0.5));
    for (int i = 0; i < 5; ++i) {
        double a = i * 2.0 * M_PI / 5.0;
        QPointF petal(center.x() + qCos(a) * size * 0.5,
                      center.y() + qSin(a) * size * 0.5);
        painter.drawEllipse(petal, size * 0.3, size * 0.2);
    }
    painter.setBrush(QColor(255, 240, 200, 150));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, size * 0.12, size * 0.12);
    painter.restore();
}

void drawPetal(QPainter &painter, QPointF center, float size, float angle) {
    painter.save();
    painter.translate(center);
    painter.rotate(angle);
    painter.setBrush(QColor(255, 220, 210, 80));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), size, size * 0.5);
    painter.restore();
}

void drawSesame(QPainter &painter, QPointF center, float size, float angle) {
    painter.save();
    painter.translate(center);
    painter.rotate(angle);
    painter.setBrush(QColor("#3A3530"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), size, size * 0.6);
    painter.restore();
}

void drawScallion(QPainter &painter, QPointF center, float size, float angle) {
    painter.save();
    painter.translate(center);
    painter.rotate(angle);
    QPen pen(QColor(120, 160, 80, 100));
    pen.setWidthF(1.2);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(QPointF(0, -size * 0.5), QPointF(0, size * 0.5));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(140, 180, 100, 80));
    painter.drawEllipse(QPointF(0, size * 0.5), size * 0.2, size * 0.3);
    painter.restore();
}

void drawXiaolongbao(QPainter &painter, QPointF center, float size) {
    painter.save();
    painter.setBrush(QColor(250, 240, 220, 180));
    painter.setPen(QPen(QColor(200, 180, 160, 100), 0.8));
    painter.drawEllipse(center, size, size * 0.8);
    // 褶子
    for (int i = 0; i < 8; ++i) {
        double a = i * 2.0 * M_PI / 8.0;
        QPointF p(center.x() + qCos(a) * size * 0.3,
                  center.y() + qSin(a) * size * 0.2 - size * 0.3);
        painter.drawLine(p, center + QPointF(0, -size * 0.05));
    }
    painter.restore();
}

void drawTinyCat(QPainter &painter, QPointF center, float size) {
    painter.save();
    // 身体
    painter.setBrush(QColor("#D4C5B8"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, size * 0.4, size * 0.3);
    // 头
    painter.drawEllipse(QPointF(center.x(), center.y() - size * 0.25), size * 0.25, size * 0.22);
    // 耳朵
    QPointF el(center.x() - size * 0.15, center.y() - size * 0.4);
    QPointF er(center.x() + size * 0.15, center.y() - size * 0.4);
    painter.setPen(QPen(QColor("#C0B0A0"), 0.5));
    painter.drawLine(el + QPointF(0, 6), el + QPointF(-4, -4));
    painter.drawLine(el + QPointF(0, 6), el + QPointF(4, -2));
    painter.drawLine(er + QPointF(0, 6), er + QPointF(4, -4));
    painter.drawLine(er + QPointF(0, 6), er + QPointF(-4, -2));
    painter.restore();
}

void drawRoughCircle(QPainter &painter, QPointF center, float size,
                     const QColor &color) {
    painter.save();
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    QPainterPath path;
    int pts = 12;
    for (int i = 0; i < pts; ++i) {
        double a = i * 2.0 * M_PI / pts;
        double r = size + qSin(i * 3.7) * size * 0.2;
        QPointF pt(center.x() + qCos(a) * r, center.y() + qSin(a) * r);
        if (i == 0) path.moveTo(pt);
        else path.lineTo(pt);
    }
    path.closeSubpath();
    painter.drawPath(path);
    painter.restore();
}

} // namespace DecoPainter
