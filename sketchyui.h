#ifndef SKETCHYUI_H
#define SKETCHYUI_H

#include <QWidget>
#include <QPushButton>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QColor>
#include <QRectF>

QPainterPath sketchyRect(const QRectF &r, int seed, double jitter = 3.0);

void drawInkBorder(QPainter *p, const QPainterPath &path,
                   const QColor &ink = QColor("#2B2B2B"),
                   int passes = 3, double spread = 0.6);

void drawInkWash(QPainter *p, const QPainterPath &path,
                 const QColor &fill, int alphaNoise = 18);

enum SketchyIcon {
    ICON_NONE = 0,
    ICON_CARD,
    ICON_SEARCH,
    ICON_STAR,
    ICON_HEART
};

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
    void setCardColor(const QColor &c) { m_cardColor = c; update(); }
    void setInkColor(const QColor &c) { m_inkColor = c; update(); }

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
    double m_pressOffset = 0;
    double m_hover = 0;
    int m_seed;
    SketchyIcon m_iconType = ICON_NONE;
    QPropertyAnimation *m_pressAnim;
    QPropertyAnimation *m_hoverAnim;
};

class SketchyCard : public QWidget {
    Q_OBJECT
public:
    explicit SketchyCard(QWidget *parent = nullptr);
    void setCardColor(const QColor &color);
    void setShadowColor(const QColor &color);
    void setInkColor(const QColor &color);
protected:
    void paintEvent(QPaintEvent *e) override;
private:
    QColor m_cardColor = QColor("#FAF7F0");
    QColor m_shadowColor = QColor("#3A3530");
    QColor m_inkColor = QColor("#2B2B2B");
};

#endif
