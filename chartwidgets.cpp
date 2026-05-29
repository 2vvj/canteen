#include "ChartWidgets.h"
#include "DecoPainter.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QtMath>
#include <algorithm>

// 莫兰迪暖色系（图表柱状图用 — 暖调低饱和）
static const QList<QColor> kChartColors = {
    QColor("#D4C5B8"),  // 暖米金
    QColor("#C8B8A5"),  // 暖土
    QColor("#B8B8A0"),  // 暖绿
    QColor("#C0B0B8"),  // 暖紫
    QColor("#D0C0A8"),  // 暖杏
    QColor("#C0B0A0"),  // 暖棕
    QColor("#A8B8B0"),  // 暖青
    QColor("#C8B0A8"),  // 暖粉
};

// ==========================================
// 静态辅助：绘制装饰云朵
// ==========================================
static void drawCloud(QPainter &painter, const QPointF &pos, float size, const QColor &color) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    float s = size;
    painter.drawEllipse(pos, s * 0.5, s * 0.35);
    painter.drawEllipse(QPointF(pos.x() - s * 0.3, pos.y() + s * 0.05), s * 0.35, s * 0.25);
    painter.drawEllipse(QPointF(pos.x() + s * 0.35, pos.y() + s * 0.05), s * 0.4, s * 0.28);
    painter.drawEllipse(QPointF(pos.x() - s * 0.1, pos.y() + s * 0.1), s * 0.3, s * 0.2);
    painter.drawEllipse(QPointF(pos.x() + s * 0.15, pos.y() + s * 0.12), s * 0.25, s * 0.18);
}

// ==========================================
// LineChartWidget 实现
// ==========================================
LineChartWidget::LineChartWidget(QWidget *parent) : QWidget(parent) {
    m_lineColor = QColor(200, 150, 120);
    setMinimumHeight(220);
}

void LineChartWidget::setData(const QVector<DataPoint> &points, const QString &title,
                               const QColor &lineColor) {
    m_points = points;
    m_title = title;
    m_lineColor = lineColor;
    update();
}

void LineChartWidget::setYAxisLabel(const QString &label) {
    m_yAxisLabel = label;
    update();
}

QRectF LineChartWidget::calcChartArea() const {
    return QRectF(50, 12, width() - 68, height() - 52);
}

void LineChartWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    // 1. 暖米手账纸背景 — 有机形状
    QRadialGradient bgGradient(rect().center(), qMax(w, h) * 0.75);
    bgGradient.setColorAt(0.0, QColor(253, 251, 247));
    bgGradient.setColorAt(0.5, QColor(250, 246, 240));
    bgGradient.setColorAt(1.0, QColor(245, 240, 232));
    painter.setBrush(bgGradient);
    painter.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeOrganicRect(
        QRectF(rect()).adjusted(2, 2, -2, -2), 2.5f, 21);
    painter.drawPath(bgPath);
    DecoPainter::drawPaperTexture(painter, rect());

    // 1.5 手绘边框
    DecoPainter::drawSketchyBorder(&painter, bgPath, QColor(43, 43, 43, 40), 1, 0.6f);

    // 2. 水彩晕染装饰
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.10f, h * 0.08f), 18,
                                       QColor(250, 235, 215, 28));
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.85f, h * 0.88f), 15,
                                       QColor(245, 230, 215, 22));

    // 3. 计算图表区域
    QRectF chartArea = calcChartArea();

    if (m_points.isEmpty()) {
        painter.setPen(DecoPainter::lightBrown());
        DecoPainter::setHandwritingFont(painter, 13);
        painter.drawText(chartArea, Qt::AlignCenter, "暂无数据");
        return;
    }

    // 4. 确定最大值（圆整到5的倍数）
    double maxVal = 1.0;
    for (const auto &pt : m_points)
        if (pt.value > maxVal) maxVal = pt.value;
    double niceMax = std::ceil(maxVal / 5.0) * 5.0;
    int numLines = 5;
    int n = m_points.size();

    // 5. 浅色图表区域背景
    painter.setBrush(QColor(250, 247, 243, 80));
    painter.setPen(QPen(QColor(229, 221, 211, 60), 1));
    painter.drawRoundedRect(chartArea, 8, 8);

    // 6. Y轴网格线
    QColor gridColor(200, 190, 178, 60);
    QFont labelFont("Microsoft YaHei", 8);
    for (int i = 0; i <= numLines; ++i) {
        double y = chartArea.bottom() - (chartArea.height() * i / numLines);
        painter.setPen(QPen(gridColor, 0.8, Qt::DotLine, Qt::RoundCap));
        painter.drawLine(QPointF(chartArea.left(), y), QPointF(chartArea.right(), y));

        double val = niceMax * i / numLines;
        painter.setPen(DecoPainter::textBrown());
        painter.setFont(labelFont);
        painter.drawText(QRectF(0, y - 10, chartArea.left() - 4, 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(val, 'f', 0));
    }

    // 7. X轴标签（日期，自动跳过防重叠）
    QFont xFont("Microsoft YaHei", 7);
    painter.setFont(xFont);
    painter.setPen(DecoPainter::textBrown());
    int labelStep = (n > 12) ? (n / 8) : 1;
    for (int i = 0; i < n; ++i) {
        if (i % labelStep != 0 && i != n - 1) continue;
        double x = chartArea.left() + chartArea.width() * i / qMax(1, n - 1);
        painter.drawText(QRectF(x - 30, chartArea.bottom() + 4, 60, 18),
                         Qt::AlignHCenter | Qt::AlignTop, m_points[i].label);
    }

    // 8. 数据折线（带手绘波动）
    QVector<QPointF> pts;
    for (int i = 0; i < n; ++i) {
        double x = chartArea.left() + chartArea.width() * i / (n - 1);
        double normalized = m_points[i].value / niceMax;
        double y = chartArea.bottom() - normalized * chartArea.height();
        double wobble = 1.2 * qSin(i * 1.7);
        pts.append(QPointF(x + wobble, y + 1.0 * qCos(i * 2.3)));
    }

    // 渐变填充区域
    QPainterPath fillPath;
    fillPath.moveTo(pts[0]);
    for (int i = 1; i < n; ++i)
        fillPath.lineTo(pts[i]);
    fillPath.lineTo(chartArea.right(), chartArea.bottom());
    fillPath.lineTo(chartArea.left(), chartArea.bottom());
    fillPath.closeSubpath();

    QLinearGradient fillGrad(chartArea.topLeft(), chartArea.bottomLeft());
    fillGrad.setColorAt(0.0, QColor(m_lineColor.red(), m_lineColor.green(), m_lineColor.blue(), 50));
    fillGrad.setColorAt(1.0, QColor(m_lineColor.red(), m_lineColor.green(), m_lineColor.blue(), 8));
    painter.setBrush(fillGrad);
    painter.setPen(Qt::NoPen);
    painter.drawPath(fillPath);

    // 折线
    QPen linePen(m_lineColor, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(linePen);
    QPainterPath strokePath;
    strokePath.moveTo(pts[0]);
    for (int i = 1; i < n; ++i)
        strokePath.lineTo(pts[i]);
    painter.drawPath(strokePath);

    // 9. 数据点圆点
    painter.setBrush(Qt::white);
    painter.setPen(QPen(m_lineColor, 2));
    for (int i = 0; i < n; ++i) {
        painter.drawEllipse(pts[i], 4.0, 4.0);
    }

    // 10. Y轴标签
    if (!m_yAxisLabel.isEmpty()) {
        painter.save();
        painter.translate(10, chartArea.center().y());
        painter.rotate(-90);
        DecoPainter::setHandwritingFont(painter, 10);
        painter.setPen(DecoPainter::textBrown());
        painter.drawText(QRect(-50, -10, 100, 20), Qt::AlignCenter, m_yAxisLabel);
        painter.restore();
    }

    // 11. 底部标题
    if (!m_title.isEmpty()) {
        painter.setPen(DecoPainter::titleBrown());
        DecoPainter::setHandwritingFont(painter, 13, true);
        painter.drawText(rect().adjusted(0, 0, 0, -8),
                         Qt::AlignBottom | Qt::AlignHCenter, m_title);
    }
}

// ==========================================
// BarChartWidget 实现
// ==========================================
BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(220);
}

void BarChartWidget::setData(const QVector<BarData> &bars, const QString &title) {
    m_bars = bars;
    m_title = title;
    update();
}

QRectF BarChartWidget::calcChartArea() const {
    return QRectF(50, 12, width() - 68, height() - 52);
}

void BarChartWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    // 1. 暖米手账纸背景 — 有机形状
    QRadialGradient bgGradient(rect().center(), qMax(w, h) * 0.75);
    bgGradient.setColorAt(0.0, QColor(253, 251, 247));
    bgGradient.setColorAt(0.5, QColor(250, 246, 240));
    bgGradient.setColorAt(1.0, QColor(245, 240, 232));
    painter.setBrush(bgGradient);
    painter.setPen(Qt::NoPen);
    QPainterPath bgPath = DecoPainter::makeOrganicRect(
        QRectF(rect()).adjusted(2, 2, -2, -2), 2.5f, 23);
    painter.drawPath(bgPath);
    DecoPainter::drawPaperTexture(painter, rect());

    // 1.5 手绘边框
    DecoPainter::drawSketchyBorder(&painter, bgPath, QColor(43, 43, 43, 40), 1, 0.6f);

    // 2. 水彩晕染装饰
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.10f, h * 0.08f), 16,
                                       QColor(250, 235, 215, 25));
    DecoPainter::drawWatercolorSplotch(painter, QPointF(w * 0.82f, h * 0.88f), 14,
                                       QColor(245, 230, 215, 20));

    // 3. 图表区域
    QRectF chartArea = calcChartArea();

    if (m_bars.isEmpty()) {
        painter.setPen(DecoPainter::lightBrown());
        DecoPainter::setHandwritingFont(painter, 13);
        painter.drawText(chartArea, Qt::AlignCenter, "暂无数据");
        return;
    }

    // 4. 最大值
    double maxVal = 1.0;
    for (const auto &bar : m_bars)
        if (bar.value > maxVal) maxVal = bar.value;
    double niceMax = std::ceil(maxVal / 5.0) * 5.0;
    int numLines = 4;
    int n = m_bars.size();

    // 5. 浅色图表背景
    painter.setBrush(QColor(250, 247, 243, 80));
    painter.setPen(QPen(QColor(229, 221, 211, 60), 1));
    painter.drawRoundedRect(chartArea, 8, 8);

    // 6. 网格线
    QColor gridColor(200, 190, 178, 60);
    QFont labelFont("Microsoft YaHei", 8);
    for (int i = 0; i <= numLines; ++i) {
        double y = chartArea.bottom() - (chartArea.height() * i / numLines);
        painter.setPen(QPen(gridColor, 0.8, Qt::DotLine, Qt::RoundCap));
        painter.drawLine(QPointF(chartArea.left(), y), QPointF(chartArea.right(), y));

        double val = niceMax * i / numLines;
        painter.setPen(DecoPainter::textBrown());
        painter.setFont(labelFont);
        painter.drawText(QRectF(0, y - 10, chartArea.left() - 4, 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(val, 'f', 0));
    }

    // 7. 绘制柱状条
    double slotWidth = chartArea.width() / n;
    double barWidth = slotWidth * 0.65;
    double gap = slotWidth * 0.35;

    QFont valFont("Microsoft YaHei", 8);
    QFont xFont("Microsoft YaHei", 7);

    for (int i = 0; i < n; ++i) {
        double barH = (m_bars[i].value / niceMax) * chartArea.height();
        double x = chartArea.left() + slotWidth * i + gap * 0.5;
        double y = chartArea.bottom() - barH;
        double bw = barWidth;

        QRectF barRect(x, y, bw, barH);
        QColor barColor = kChartColors[i % kChartColors.size()];

        // 圆角柱状条
        QPainterPath barPath;
        barPath.addRoundedRect(barRect, 4, 4);
        painter.setBrush(barColor);
        painter.setPen(QPen(barColor.darker(130), 1));
        painter.drawPath(barPath);

        // 值标签
        painter.setPen(DecoPainter::titleBrown());
        painter.setFont(valFont);
        painter.drawText(QRectF(x - 8, y - 17, bw + 16, 15),
                         Qt::AlignCenter, QString::number(m_bars[i].value, 'f', 0));

        // X轴标签
        painter.setPen(DecoPainter::textBrown());
        painter.setFont(xFont);
        painter.drawText(QRectF(x - 12, chartArea.bottom() + 4, bw + 24, 18),
                         Qt::AlignHCenter | Qt::AlignTop, m_bars[i].label);
    }

    // 8. 底部标题
    if (!m_title.isEmpty()) {
        painter.setPen(DecoPainter::titleBrown());
        DecoPainter::setHandwritingFont(painter, 13, true);
        painter.drawText(rect().adjusted(0, 0, 0, -8),
                         Qt::AlignBottom | Qt::AlignHCenter, m_title);
    }
}
