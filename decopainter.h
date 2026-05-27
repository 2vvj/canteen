#ifndef DECOPAINTER_H
#define DECOPAINTER_H

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QPointF>
#include <QRectF>
#include <QFont>

// 手绘装饰工具类 — ddyc1230 的手绘风格绘制函数
namespace DecoPainter {

// 颜色
QColor titleBrown();
QColor textBrown();
QColor lightBrown();
QBrush paperBase();

// 字体
void setHandwritingFont(QPainter &painter, int size, bool bold = false);

// 纹理
void drawPaperTexture(QPainter &painter, const QRect &rect);

// 形状
QPainterPath makeOrganicRect(const QRectF &r, float radius, int seed = 42);
QPainterPath makeWavyRect(const QRectF &r, float amplitude);

// 边框
void drawSketchyBorder(QPainter *p, const QPainterPath &path,
                       const QColor &ink, int passes, double spread);

// 线条
void drawScratchyLine(QPainter &painter, QPointF start, QPointF end,
                      const QColor &color, float width, float wobble);

// 水彩
void drawWatercolorSplotch(QPainter &painter, QPointF center, float size,
                           const QColor &color);

// 装饰
void drawSakura(QPainter &painter, QPointF center, float size);
void drawPetal(QPainter &painter, QPointF center, float size, float angle);
void drawSesame(QPainter &painter, QPointF center, float size, float angle);
void drawScallion(QPainter &painter, QPointF center, float size, float angle);
void drawXiaolongbao(QPainter &painter, QPointF center, float size);
void drawTinyCat(QPainter &painter, QPointF center, float size);
void drawRoughCircle(QPainter &painter, QPointF center, float size,
                     const QColor &color);

} // namespace DecoPainter

#endif
