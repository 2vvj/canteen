#include "sketchyui.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include <QFont>
#include <QMouseEvent>

static quint32 quickHash(const void *data, size_t len)
{
    auto *p = static_cast<const quint8 *>(data);
    quint32 h = 5381;
    for (size_t i = 0; i < len; ++i) h = ((h << 5) + h) + p[i];
    return h;
}

static int seedFor(const QRectF &r, int base)
{
    int x = static_cast<int>(r.x() * 7919 + r.y() * 6271 + base * 131);
    return x & 0x7FFFFFFF;
}

static double jit(int seed, int idx, double amplitude)
{
    // Deterministic pseudo-random: fast, seed-dependent
    int s = seed + idx * 2654435761u;
    s ^= (s >> 16);
    s *= 0x45d9f3b;
    s ^= (s >> 16);
    double v = static_cast<double>(qAbs(s) % 10000) / 10000.0;
    return (v - 0.5) * 2.0 * amplitude;
}

QPainterPath sketchyRect(const QRectF &r, int seed, double jitterAmt)
{
    QPainterPath path;
    double x0 = r.left();
    double y0 = r.top();
    double x1 = r.right();
    double y1 = r.bottom();

    // 4 corners with jitter
    double cx0 = x0 + jit(seed, 0, jitterAmt);
    double cy0 = y0 + jit(seed, 1, jitterAmt);
    double cx1 = x1 + jit(seed, 2, jitterAmt);
    double cy1 = y0 + jit(seed, 3, jitterAmt);
    double cx2 = x1 + jit(seed, 4, jitterAmt);
    double cy2 = y1 + jit(seed, 5, jitterAmt);
    double cx3 = x0 + jit(seed, 6, jitterAmt);
    double cy3 = y1 + jit(seed, 7, jitterAmt);

    // 4 midpoints along edges with jitter
    double mxTop    = (x0 + x1) / 2.0 + jit(seed, 8,  jitterAmt * 0.8);
    double myTop    = y0 + jit(seed, 9,  jitterAmt * 0.5);
    double mxRight  = x1 + jit(seed, 10, jitterAmt * 0.5);
    double myRight  = (y0 + y1) / 2.0 + jit(seed, 11, jitterAmt * 0.8);
    double mxBottom = (x0 + x1) / 2.0 + jit(seed, 12, jitterAmt * 0.8);
    double myBottom = y1 + jit(seed, 13, jitterAmt * 0.5);
    double mxLeft   = x0 + jit(seed, 14, jitterAmt * 0.5);
    double myLeft   = (y0 + y1) / 2.0 + jit(seed, 15, jitterAmt * 0.8);

    path.moveTo(cx0, cy0);
    // Top edge: corner0 → midTop → corner1
    path.quadTo(mxTop, myTop, cx1, cy1);
    // Right edge
    path.quadTo(mxRight, myRight, cx2, cy2);
    // Bottom edge
    path.quadTo(mxBottom, myBottom, cx3, cy3);
    // Left edge
    path.quadTo(mxLeft, myLeft, cx0, cy0);

    return path;
}

void drawInkBorder(QPainter *p, const QPainterPath &path,
                   const QColor &ink, int passes, double spread)
{
    QPen pen(ink);
    pen.setWidthF(1.2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    p->setBrush(Qt::NoBrush);

    for (int i = 0; i < passes; ++i) {
        double ox = (i - (passes - 1) / 2.0) * spread;
        double oy = ox * 0.7;
        p->save();
        p->translate(ox, oy);
        pen.setWidthF(1.0 + (i % 2) * 0.5);
        p->setPen(pen);
        p->drawPath(path);
        p->restore();
    }
}

void drawInkWash(QPainter *p, const QPainterPath &path,
                 const QColor &fill, int alphaNoise)
{
    // Slightly textured fill
    QColor base = fill;
    if (base.alpha() == 255) {
        int a = 240 + static_cast<int>(quickHash(&base, sizeof(base)) % alphaNoise);
        base.setAlpha(qBound(200, a, 255));
    }
    p->setBrush(base);
    p->setPen(Qt::NoPen);
    p->drawPath(path);

    // Subtle cross-hatch suggestion
    QPen hatch(base.darker(105));
    hatch.setWidthF(0.3);
    p->setPen(hatch);
    QRectF bb = path.boundingRect();
    double step = 14.0;
    for (double y = bb.top(); y < bb.bottom(); y += step) {
        double off = static_cast<int>(quickHash(&y, sizeof(y)) % 5) - 2.5;
        p->drawLine(QPointF(bb.left(), y + off), QPointF(bb.right(), y + off + 1.0));
    }
}


// ── SketchyButton ───────────────────────────────────────────────

SketchyButton::SketchyButton(const QString &text,
                             const QColor &cardColor,
                             const QColor &shadowColor,
                             QWidget *parent)
    : QPushButton(text, parent),
      m_cardColor(cardColor),
      m_shadowColor(shadowColor)
{
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(44);
    setMinimumWidth(110);

    QFont f = font();
    f.setPointSize(12);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    setFont(f);

    m_seed = seedFor(QRectF(0, 0, width(), height()),
                     reinterpret_cast<quintptr>(this) & 0xFFFF);

    m_pressAnim = new QPropertyAnimation(this, "pressOffset", this);
    m_pressAnim->setDuration(180);
    m_pressAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_hoverAnim = new QPropertyAnimation(this, "hover", this);
    m_hoverAnim->setDuration(220);
    m_hoverAnim->setEasingCurve(QEasingCurve::OutCubic);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SketchyButton::setPressOffset(double v)
{
    m_pressOffset = v;
    update();
}

void SketchyButton::setHover(double v)
{
    m_hover = v;
    update();
}

void SketchyButton::drawIcon(QPainter *p, const QRectF &iconRect)
{
    p->save();
    QPen ipen(m_inkColor);
    ipen.setWidthF(1.3);
    ipen.setCapStyle(Qt::RoundCap);
    ipen.setJoinStyle(Qt::RoundJoin);
    p->setPen(ipen);
    p->setBrush(Qt::NoBrush);

    double cx = iconRect.center().x();
    double cy = iconRect.center().y();
    double s = qMin(iconRect.width(), iconRect.height()) * 0.42;

    switch (m_iconType) {
    case ICON_CARD: {
        // Diamond/card shape
        QPainterPath card;
        card.moveTo(cx, cy - s);
        card.lineTo(cx + s * 0.7, cy);
        card.lineTo(cx, cy + s);
        card.lineTo(cx - s * 0.7, cy);
        card.closeSubpath();
        // Double-line sketchy effect
        p->drawPath(card);
        p->translate(0.4, 0.3);
        ipen.setWidthF(0.8);
        p->setPen(ipen);
        p->drawPath(card);
        break;
    }
    case ICON_SEARCH: {
        // Magnifying glass: circle + handle
        double r = s * 0.55;
        p->drawEllipse(QPointF(cx - s * 0.15, cy - s * 0.15), r, r);
        p->translate(0.4, 0.2);
        ipen.setWidthF(0.7);
        p->setPen(ipen);
        p->drawEllipse(QPointF(cx - s * 0.15, cy - s * 0.15), r, r);
        // Handle
        QPointF handleStart(cx - s * 0.15 + r * 0.7, cy - s * 0.15 + r * 0.7);
        QPointF handleEnd(handleStart.x() + s * 0.7, handleStart.y() + s * 0.7);
        ipen.setWidthF(1.5);
        p->setPen(ipen);
        p->drawLine(handleStart, handleEnd);
        p->translate(0.3, -0.2);
        ipen.setWidthF(0.8);
        p->setPen(ipen);
        p->drawLine(handleStart, handleEnd);
        break;
    }
    case ICON_STAR: {
        // 5-pointed star
        QPainterPath star;
        for (int i = 0; i < 5; ++i) {
            double angle = -M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
            double x = cx + cos(angle) * s;
            double y = cy + sin(angle) * s;
            if (i == 0) star.moveTo(x, y);
            else star.lineTo(x, y);
            // Inner point
            angle += M_PI / 5.0;
            x = cx + cos(angle) * s * 0.38;
            y = cy + sin(angle) * s * 0.38;
            star.lineTo(x, y);
        }
        star.closeSubpath();
        p->drawPath(star);
        p->translate(0.3, 0.3);
        ipen.setWidthF(0.7);
        p->setPen(ipen);
        p->drawPath(star);
        break;
    }
    case ICON_HEART: {
        // Heart shape using two arcs
        QPainterPath heart;
        double hs = s * 0.65;
        heart.moveTo(cx, cy + hs);
        heart.cubicTo(cx - hs * 1.2, cy - hs * 0.1, cx - hs * 0.6, cy - hs * 0.8, cx, cy - hs * 0.2);
        heart.cubicTo(cx + hs * 0.6, cy - hs * 0.8, cx + hs * 1.2, cy - hs * 0.1, cx, cy + hs);
        p->drawPath(heart);
        p->translate(0.3, 0.3);
        ipen.setWidthF(0.7);
        p->setPen(ipen);
        p->drawPath(heart);
        break;
    }
    default:
        break;
    }
    p->restore();
}

void SketchyButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    QRectF area(0, 0, w, h);
    double margin = 4.0;

    // Shadow offset — shifts slightly on hover
    double sx = 2.5 + m_hover * 0.8;
    double sy = 3.0 + m_hover * 0.8;
    QRectF shadowRect(area.left() + margin + sx,
                      area.top() + margin + sy,
                      area.width() - margin * 2 - 4,
                      area.height() - margin * 2 - 4);

    // Card sits offset from shadow; press moves card toward shadow
    double baseOx = -1.5, baseOy = -1.5;
    double targetOx = sx, targetOy = sy;
    double ox = baseOx + (targetOx - baseOx) * m_pressOffset;
    double oy = baseOy + (targetOy - baseOy) * m_pressOffset;

    QRectF cardRect(shadowRect.left() + ox,
                    shadowRect.top() + oy,
                    shadowRect.width(),
                    shadowRect.height());

    int s = seedFor(cardRect, m_seed);
    // Very subtle organic edge — barely visible, just enough for paper feel
    double jitAmt = 0.6;

    QPainterPath shadowPath = sketchyRect(shadowRect, s + 100, jitAmt);
    QPainterPath cardPath   = sketchyRect(cardRect, s, jitAmt);

    // Shadow — deepens on hover
    QColor shadowCol = m_shadowColor;
    if (m_hover > 0.01) {
        shadowCol = shadowCol.darker(100 + static_cast<int>(m_hover * 30));
    }
    p.setBrush(shadowCol);
    p.setPen(Qt::NoPen);
    p.drawPath(shadowPath);

    // Card fill — warms slightly on hover
    QColor cardCol = m_cardColor;
    if (m_hover > 0.01) {
        int r = qMin(255, cardCol.red() + static_cast<int>(m_hover * 12));
        int g = qMin(255, cardCol.green() + static_cast<int>(m_hover * 8));
        int b = qMin(255, cardCol.blue() + static_cast<int>(m_hover * 4));
        cardCol = QColor(r, g, b, cardCol.alpha());
    }
    drawInkWash(&p, cardPath, cardCol, 15);

    // Ink border — gets bolder on hover
    QColor ink = m_inkColor;
    int borderPasses = 2 + static_cast<int>(m_hover * 1.5);
    double borderSpread = 0.4 + m_hover * 0.3;
    drawInkBorder(&p, cardPath, ink, borderPasses, borderSpread);

    // Icon + Text — ink color intensifies on hover
    QColor textInk = ink;
    if (m_hover > 0.5) {
        textInk = QColor(
            qMin(255, ink.red() + 20),
            qMin(255, ink.green() + 10),
            qMin(255, ink.blue() + 5));
    }
    p.setPen(textInk);
    QFont f = font();
    f.setWeight(m_hover > 0.5 ? QFont::DemiBold : QFont::Medium);
    p.setFont(f);

    if (m_iconType != ICON_NONE) {
        double iconW = cardRect.height() * 0.48;
        double padLeft = cardRect.height() * 0.22;
        QRectF iconArea(cardRect.left() + padLeft,
                        cardRect.center().y() - iconW / 2.0,
                        iconW, iconW);
        drawIcon(&p, iconArea);

        QRectF textRect(cardRect.left() + iconW + padLeft * 0.6,
                        cardRect.top(),
                        cardRect.width() - iconW - padLeft * 1.6,
                        cardRect.height());
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
    } else {
        p.drawText(cardRect, Qt::AlignCenter, text());
    }
}

void SketchyButton::enterEvent(QEnterEvent *)
{
    m_hoverAnim->stop();
    m_hoverAnim->setEndValue(1.0);
    m_hoverAnim->start();
}

void SketchyButton::leaveEvent(QEvent *)
{
    m_hoverAnim->stop();
    m_hoverAnim->setEndValue(0.0);
    m_hoverAnim->start();
}

void SketchyButton::mousePressEvent(QMouseEvent *e)
{
    m_pressAnim->stop();
    m_pressAnim->setEndValue(1.0);
    m_pressAnim->start();
    QPushButton::mousePressEvent(e);
}

void SketchyButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_pressAnim->stop();
    m_pressAnim->setEndValue(0.0);
    m_pressAnim->start();
    QPushButton::mouseReleaseEvent(e);
}
