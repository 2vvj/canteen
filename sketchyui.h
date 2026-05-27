#ifndef SKETCHYUI_H
#define SKETCHYUI_H

#include <QWidget>
#include <QPushButton>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QColor>
#include <QRectF>

// ── Utility: build an organic "jittered" rect path ──────────────
// Returns a QPainterPath with irregular edges — slight random
// offsets at each vertex + bezier curves instead of straight lines.
QPainterPath sketchyRect(const QRectF &r, int seed, double jitter = 3.0);

// ── Utility: draw an ink-style multiline border ─────────────────
// Draws multiple slightly offset, thin lines to simulate a
// hand-traced ink pen stroke.
void drawInkBorder(QPainter *p, const QPainterPath &path,
                   const QColor &ink = QColor("#2B2B2B"),
                   int passes = 3, double spread = 0.6);

// ── Ink-wash filled background ─────────────────────────────────
void drawInkWash(QPainter *p, const QPainterPath &path,
                 const QColor &fill, int alphaNoise = 18);

// ── Icon types for SketchyButton ──────────────────────────────
enum SketchyIcon {
    ICON_NONE = 0,
    ICON_CARD,
    ICON_SEARCH,
    ICON_STAR,
    ICON_HEART
};

// ── SketchyButton: shadow-offset paper-sticker button ──────────
class SketchyButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double pressOffset READ pressOffset WRITE setPressOffset)
    Q_PROPERTY(double hover READ hover WRITE setHover)
public:
    SketchyButton(const QString &text,
                  const QColor &cardColor,
                  const QColor &shadowColor,
                  QWidget *parent = nullptr);

    double pressOffset() const { return m_pressOffset; }
    void setPressOffset(double v);
    double hover() const { return m_hover; }
    void setHover(double v);
    void setIconType(SketchyIcon icon) { m_iconType = icon; update(); }

protected:
    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void drawIcon(QPainter *p, const QRectF &iconRect);

    QColor m_cardColor;
    QColor m_shadowColor;
    QColor m_inkColor = QColor("#2B2B2B");
    double m_pressOffset = 0;   // 0 = rest, 1.0 = fully pressed
    double m_hover = 0;          // 0 = idle, 1.0 = hovered
    int m_seed;
    SketchyIcon m_iconType = ICON_NONE;
    QPropertyAnimation *m_pressAnim;
    QPropertyAnimation *m_hoverAnim;
};

#endif
