#include "RadarChartWidget.h"
#include "DecoPainter.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QRadialGradient>

RadarChartWidget::RadarChartWidget(QWidget *parent) : QWidget(parent) {
    m_labels << QString::fromUtf8("美味") << QString::fromUtf8("价格")
             << QString::fromUtf8("体验") << QString::fromUtf8("热量")
             << QString::fromUtf8("距离");
    m_mainColor = QColor(200, 160, 130);
    m_data = RadarData(80, 70, 85, 60, 90);
}

void RadarChartWidget::setData(const RadarData &data, const QString &itemName) {
    m_data = data;
    m_itemName = itemName;
    update();
}

void RadarChartWidget::setMainColor(const QColor &color) {
    m_mainColor = color;
    update();
}

QPointF RadarChartWidget::calculatePoint(const QPointF &center, float radius, float angleDegrees) {
    float radians = qDegreesToRadians(angleDegrees - 90.0f);
    return QPointF(center.x() + radius * qCos(radians), center.y() + radius * qSin(radians));
}

static QPainterPath makePolygonPath(const QPolygonF &pts) {
    QPainterPath path;
    if (pts.isEmpty()) return path;
    path.moveTo(pts[0]);
    for (int i = 1; i < pts.size(); ++i)
        path.lineTo(pts[i]);
    path.closeSubpath();
    return path;
}

void RadarChartWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int w = width();
    int h = height();
    QPointF center(w / 2.0, h / 2.0 + 5);
    float side = qMin(w, h);
    float maxRadius = (side / 2.0) * 0.52f;  // 缩小留更多空间给标签

    // ==========================================
    // 1. 暖米色手账纸基底 + 复古纹理
    // ==========================================
    painter.setBrush(DecoPainter::paperBase());
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 18, 18);
    DecoPainter::drawPaperTexture(painter, rect());

    // ==========================================
    // 2. 边缘水彩晕染装饰
    // ==========================================
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.12f, h * 0.10f), 35,
                                       QColor(250, 235, 215, 25));
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.88f, h * 0.85f), 30,
                                       QColor(245, 225, 210, 20));
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.85f, h * 0.12f), 25,
                                       QColor(240, 230, 210, 20));

    // 手绘水彩蒸汽线条
    painter.setPen(QPen(QColor(200, 185, 170, 40), 1.2, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    for (int k = 0; k < 3; ++k) {
        float angle = 30.0f + k * 40.0f;
        float rad = qDegreesToRadians(angle);
        float r1 = maxRadius + 30 + k * 8;
        float r2 = maxRadius + 55 + k * 5;
        QPointF start = calculatePoint(center, r1, angle - 10 + k * 5);
        QPointF end = calculatePoint(center, r2, angle + 10 - k * 3);
        QPainterPath steam;
        steam.moveTo(start);
        float mx = (start.x() + end.x()) / 2 + 5 * qSin(k * 2.0f);
        float my = (start.y() + end.y()) / 2 - 12 + 3 * k;
        steam.quadTo(mx, my, end.x(), end.y());
        painter.drawPath(steam);
    }

    // ==========================================
    // 3. 底层蜘蛛网 — 淡棕色手绘线
    // ==========================================
    QColor gridColor(200, 185, 172, 100);
    for (int i = 1; i <= 5; ++i) {
        float r = maxRadius * i / 5.0f;
        QPolygonF ring;
        for (int j = 0; j < 5; ++j) {
            ring << calculatePoint(center, r, j * 72.0f);
        }
        QPainterPath ringPath = makePolygonPath(ring);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(gridColor, 0.8, Qt::SolidLine, Qt::RoundCap));
        painter.drawPath(ringPath);
    }

    // ==========================================
    // 4. 发散轴线
    // ==========================================
    QColor axisColor(185, 170, 155, 90);
    for (int i = 0; i < 5; ++i) {
        QPointF axisEnd = calculatePoint(center, maxRadius, i * 72.0f);
        float wobbleX = 1.2f * qSin(i * 1.3f);
        float wobbleY = 1.2f * qCos(i * 2.1f);
        QPointF endWobble(axisEnd.x() + wobbleX, axisEnd.y() + wobbleY);
        painter.setPen(QPen(axisColor, 1.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(center + QPointF(wobbleX*0.5, wobbleY*0.5), endWobble);
    }

    // ==========================================
    // 4.5 轴标签 + 实际数值 —— 固定在雷达外围不重叠
    // ==========================================
    // 根据标签点相对于中心的方向，自动选择向外对齐的方向
    struct TextBox { QRectF rect; int align; };
    auto outwardPlacement = [&](const QPointF &p, float textW, float textH) -> TextBox {
        QPointF dir = p - center;
        int hAlign = Qt::AlignHCenter;
        int vAlign = Qt::AlignVCenter;
        float rx = p.x(), ry = p.y();
        // 水平方向：在右侧则左对齐（文字向右延伸），左侧则右对齐（文字向左延伸）
        if      (dir.x() >  8) { hAlign = Qt::AlignLeft; }
        else if (dir.x() < -8) { hAlign = Qt::AlignRight; rx -= textW; }
        else                   { rx -= textW / 2; }
        // 垂直方向：在上方则底对齐（文字向上延伸），下方则顶对齐（文字向下延伸）
        if      (dir.y() < -8) { vAlign = Qt::AlignBottom; ry -= textH; }
        else if (dir.y() >  8) { vAlign = Qt::AlignTop; }
        else                   { ry -= textH / 2; }
        return { QRectF(rx, ry, textW, textH), hAlign | vAlign };
    };

    for (int i = 0; i < 5; ++i) {
        float angle = i * 72.0f;
        // 轴标签（在外圈稍远处）
        QPointF lp = calculatePoint(center, maxRadius * 1.28f, angle);
        TextBox labelBox = outwardPlacement(lp, 90, 22);
        DecoPainter::setHandwritingFont(painter, 12, true);
        painter.setPen(DecoPainter::titleBrown());
        painter.drawText(labelBox.rect, labelBox.align, m_labels[i]);

        // 实际数值（在标签更外侧，错开避免遮挡）
        if (m_data.showActualValues) {
            QStringList actualTexts;
            actualTexts << QString::number(m_data.actualTaste, 'f', 0) + QString::fromUtf8("分");
            actualTexts << QString::fromUtf8("¥") + QString::number(m_data.actualPrice, 'f', 1);
            actualTexts << QString::number(m_data.actualExperience, 'f', 0) + QString::fromUtf8("分");
            actualTexts << QString::number(m_data.actualCalories, 'f', 0) + QString::fromUtf8("kcal");
            actualTexts << QString::number(m_data.actualDistance, 'f', 0) + QString::fromUtf8("m");

            // 数值固定在标签下方（向下偏移 20px），避免与标签重叠
            QPointF vp = calculatePoint(center, maxRadius * 1.28f, angle);
            vp.ry() += 20.0f;
            TextBox valBox = outwardPlacement(vp, 90, 18);
            DecoPainter::setHandwritingFont(painter, 10, false);
            painter.setPen(QColor(180, 90, 70));
            painter.drawText(valBox.rect, valBox.align, actualTexts[i]);
        }
    }

    // ==========================================
    // 5. 数据多边形 — 水彩晕染填充
    // ==========================================
    QVector<float> sc = {
        m_data.taste, m_data.price, m_data.experience,
        m_data.calories, m_data.distance
    };
    QPolygonF dataPolygon;
    for (int i = 0; i < 5; ++i) {
        dataPolygon << calculatePoint(center,
                       maxRadius * qBound(0.0f, sc[i] / 100.0f, 1.0f),
                       i * 72.0f);
    }

    // 水彩晕染填充
    QRadialGradient watercolorGrad(center, maxRadius * 0.9f);
    QColor wcInner(230, 190, 165, 25);
    QColor wcMid(220, 175, 145, 60);
    QColor wcEdge(210, 165, 130, 35);
    watercolorGrad.setColorAt(0.0, wcInner);
    watercolorGrad.setColorAt(0.3, wcMid);
    watercolorGrad.setColorAt(0.7, QColor(215, 170, 140, 45));
    watercolorGrad.setColorAt(1.0, wcEdge);

    painter.setBrush(watercolorGrad);
    painter.setPen(Qt::NoPen);
    QPainterPath dataPath = makePolygonPath(dataPolygon);
    painter.drawPath(dataPath);

    // 边缘手绘线条
    QColor lineColor(190, 150, 120, 180);
    painter.setPen(QPen(lineColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    QPainterPath edgePath = makePolygonPath(dataPolygon);
    painter.drawPath(edgePath);

    // ==========================================
    // 6. 数据顶点 — 暖色圆点
    // ==========================================
    painter.setBrush(DecoPainter::paperBase());
    painter.setPen(QPen(QColor(190, 150, 120), 2.2));
    for (int i = 0; i < 5; ++i) {
        painter.drawEllipse(dataPolygon[i], 5.0, 5.0);
    }

    // ==========================================
    // 7. 底部菜品名称 — 手写体
    // ==========================================
    if (!m_itemName.isEmpty()) {
        DecoPainter::setHandwritingFont(painter, 16, true);
        painter.setPen(DecoPainter::titleBrown());
        painter.drawText(rect().adjusted(0, 0, 0, -10),
                         Qt::AlignBottom | Qt::AlignHCenter, m_itemName);
    }

}
