#include "ChartWidgets.h"
#include "DecoPainter.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QToolTip>
#include <QtMath>
#include <algorithm>

// 柱状图用色 — 统一暖调
static const QList<QColor> kChartColors = {
    QColor(160, 140, 120),
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
    setMouseTracking(true);
    setStyleSheet("QToolTip { background-color: #FDFBF7; color: #3A3530; "
                  "border: 1px solid #C8BAB0; padding: 4px 8px; "
                  "font-family: 'Microsoft YaHei'; font-size: 11px; }");
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

void LineChartWidget::setAverageLine(double value, const QColor &color) {
    m_avgValue = value;
    m_avgColor = color;
    update();
}

QRectF LineChartWidget::calcChartArea() const {
    return QRectF(50, 14, width() - 64, height() - 48);
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

    // 9.25 悬停点高亮 — 光晕 + 放大实心点
    if (m_hoveredIndex >= 0 && m_hoveredIndex < n) {
        QPointF hp = pts[m_hoveredIndex];
        // 外光晕
        QRadialGradient glow(hp, 9.0);
        QColor glowC = m_lineColor;
        glow.setColorAt(0.0, QColor(glowC.red(), glowC.green(), glowC.blue(), 80));
        glow.setColorAt(1.0, QColor(glowC.red(), glowC.green(), glowC.blue(), 0));
        painter.setBrush(glow);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(hp, 9.0, 9.0);
        // 外环
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(m_lineColor.lighter(140), 2.5));
        painter.drawEllipse(hp, 7.5, 7.5);
        // 实心点
        painter.setBrush(m_lineColor);
        painter.setPen(QPen(Qt::white, 1.5));
        painter.drawEllipse(hp, 4.5, 4.5);
    }

    // 9.5 均值虚线 — 线用数据色，标签用表头 accent 色
    if (m_avgValue > 0 && niceMax > 0) {
        double avgY = chartArea.bottom() - (m_avgValue / niceMax) * chartArea.height();
        avgY = qBound(chartArea.top(), avgY, chartArea.bottom());
        QPen avgPen(QColor(160, 140, 120), 0.8, Qt::DashLine, Qt::RoundCap);
        avgPen.setDashPattern({6.0, 5.0});
        painter.setPen(avgPen);
        painter.drawLine(QPointF(chartArea.left(), avgY), QPointF(chartArea.right(), avgY));

        // 均值标签 — 表头 accent 色
        QFont avgFont("Microsoft YaHei", 8);
        avgFont.setItalic(true);
        painter.setFont(avgFont);
        painter.setPen(m_avgColor);
        QString avgText = QString::fromUtf8("均值 %1").arg(m_avgValue, 0, 'f', 1);
        painter.drawText(QRectF(chartArea.right() - 90, avgY - 16, 86, 14),
                         Qt::AlignRight | Qt::AlignVCenter, avgText);
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

void LineChartWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressPos = event->globalPosition();
        m_pressing = true;
    }
    QWidget::mousePressEvent(event);
}

void LineChartWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (m_pressing) {
        m_pressing = false;
        int dx = event->globalPosition().toPoint().x() - m_pressPos.x();
        if (dx > 60) {
            emit swiped(-1);  // 右拖 → 看更早
        } else if (dx < -60) {
            emit swiped(+1);  // 左拖 → 看更新
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void LineChartWidget::leaveEvent(QEvent *) {
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        update();
        QToolTip::hideText();
    }
}

void LineChartWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_points.isEmpty()) {
        if (m_hoveredIndex != -1) { m_hoveredIndex = -1; update(); QToolTip::hideText(); }
        return;
    }

    QRectF chartArea = calcChartArea();
    int n = m_points.size();

    double maxVal = 1.0;
    for (const auto &pt : m_points)
        if (pt.value > maxVal) maxVal = pt.value;
    double niceMax = std::ceil(maxVal / 5.0) * 5.0;

    int newHover = -1;
    for (int i = 0; i < n; ++i) {
        double x = chartArea.left() + chartArea.width() * i / (n - 1);
        double normalized = m_points[i].value / niceMax;
        double y = chartArea.bottom() - normalized * chartArea.height();
        double wobble = 1.2 * qSin(i * 1.7);
        QPointF ptPos(x + wobble, y + 1.0 * qCos(i * 2.3));

        double dist = QLineF(event->position(), ptPos).length();
        if (dist < 15.0) {
            newHover = i;
            QString tip = QString("%1\n%2: %3")
                .arg(m_points[i].label)
                .arg(m_yAxisLabel)
                .arg(m_points[i].value, 0, 'f', 1);
            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
            break;
        }
    }
    if (newHover != m_hoveredIndex) {
        m_hoveredIndex = newHover;
        update();
    }
    if (newHover == -1) QToolTip::hideText();
}

// ==========================================
// BarChartWidget 实现
// ==========================================
BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(220);
    setMouseTracking(true);
    setStyleSheet("QToolTip { background-color: #FDFBF7; color: #3A3530; "
                  "border: 1px solid #C8BAB0; padding: 4px 8px; "
                  "font-family: 'Microsoft YaHei'; font-size: 11px; }");
}

void BarChartWidget::setData(const QVector<BarData> &bars, const QString &title) {
    m_bars = bars;
    m_title = title;
    update();
}

void BarChartWidget::setAverageLine(double value, const QColor &color) {
    m_avgValue = value;
    m_avgColor = color;
    update();
}

QRectF BarChartWidget::calcChartArea() const {
    return QRectF(50, 14, width() - 64, height() - 48);
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

    // 7. 绘制柱状条 — 固定柱宽，间距自适应
    const double kFixedBarW = 52.0;
    double totalBarsW = n * kFixedBarW;
    double barWidth = qMin(kFixedBarW, chartArea.width() / (n * 1.6));
    totalBarsW = n * barWidth;
    double gap = (chartArea.width() - totalBarsW) / (n + 1);

    QFont valFont("Microsoft YaHei", 8);
    QFont xFont("Microsoft YaHei", 7);

    for (int i = 0; i < n; ++i) {
        double barH = (m_bars[i].value / niceMax) * chartArea.height();
        double x = chartArea.left() + gap + i * (barWidth + gap);
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

        // 悬停高亮 — 亮边 + 光晕
        if (i == m_hoveredIndex) {
            QColor hlColor = barColor.lighter(150);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(hlColor, 2.5));
            painter.drawPath(barPath);
            // 底部光晕条
            QLinearGradient glowGrad(barRect.bottomLeft(), barRect.topRight());
            glowGrad.setColorAt(0.0, QColor(hlColor.red(), hlColor.green(), hlColor.blue(), 50));
            glowGrad.setColorAt(1.0, QColor(hlColor.red(), hlColor.green(), hlColor.blue(), 0));
            painter.setBrush(glowGrad);
            painter.setPen(Qt::NoPen);
            painter.drawPath(barPath);
        }

        // 值标签 — 居中于柱子
        painter.setPen(DecoPainter::titleBrown());
        painter.setFont(valFont);
        double valW = qMax(bw + 20, 40.0);
        painter.drawText(QRectF(x - (valW - bw) / 2.0, y - 17, valW, 15),
                         Qt::AlignCenter, QString::number(m_bars[i].value, 'f', 0));

        // X轴标签 — 居中于柱子
        painter.setPen(DecoPainter::textBrown());
        painter.setFont(xFont);
        double labelW = qMax(bw + 24, 50.0);
        painter.drawText(QRectF(x - (labelW - bw) / 2.0, chartArea.bottom() + 4, labelW, 18),
                         Qt::AlignHCenter | Qt::AlignTop, m_bars[i].label);
    }

    // 7.5 均值虚线 — 线用数据色，标签用表头 accent 色
    if (m_avgValue > 0 && niceMax > 0) {
        double avgY = chartArea.bottom() - (m_avgValue / niceMax) * chartArea.height();
        avgY = qBound(chartArea.top(), avgY, chartArea.bottom());
        QPen avgPen(QColor(160, 140, 120), 0.8, Qt::DashLine, Qt::RoundCap);
        avgPen.setDashPattern({6.0, 5.0});
        painter.setPen(avgPen);
        painter.drawLine(QPointF(chartArea.left(), avgY), QPointF(chartArea.right(), avgY));

        QFont avgFont("Microsoft YaHei", 8);
        avgFont.setItalic(true);
        painter.setFont(avgFont);
        painter.setPen(m_avgColor);
        QString avgText = QString::fromUtf8("均值 %1").arg(m_avgValue, 0, 'f', 1);
        painter.drawText(QRectF(chartArea.right() - 90, avgY - 16, 86, 14),
                         Qt::AlignRight | Qt::AlignVCenter, avgText);
    }

    // 8. 底部标题
    if (!m_title.isEmpty()) {
        painter.setPen(DecoPainter::titleBrown());
        DecoPainter::setHandwritingFont(painter, 13, true);
        painter.drawText(rect().adjusted(0, 0, 0, -8),
                         Qt::AlignBottom | Qt::AlignHCenter, m_title);
    }
}

void BarChartWidget::leaveEvent(QEvent *) {
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        update();
        QToolTip::hideText();
    }
}

void BarChartWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_bars.isEmpty()) {
        if (m_hoveredIndex != -1) { m_hoveredIndex = -1; update(); QToolTip::hideText(); }
        return;
    }

    QRectF chartArea = calcChartArea();
    int n = m_bars.size();

    double maxVal = 1.0;
    for (const auto &bar : m_bars)
        if (bar.value > maxVal) maxVal = bar.value;
    double niceMax = std::ceil(maxVal / 5.0) * 5.0;

    const double kFixedBarW = 52.0;
    double barWidth = qMin(kFixedBarW, chartArea.width() / (n * 1.6));
    double totalBarsW = n * barWidth;
    double gap = (chartArea.width() - totalBarsW) / (n + 1);

    int newHover = -1;
    for (int i = 0; i < n; ++i) {
        double barH = (m_bars[i].value / niceMax) * chartArea.height();
        double x = chartArea.left() + gap + i * (barWidth + gap);
        double y = chartArea.bottom() - barH;
        QRectF barRect(x, y, barWidth, barH);

        if (barRect.contains(event->position())) {
            newHover = i;
            QString tip = QString("%1: %2")
                .arg(m_bars[i].label)
                .arg(m_bars[i].value, 0, 'f', 1);
            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
            break;
        }
    }
    if (newHover != m_hoveredIndex) {
        m_hoveredIndex = newHover;
        update();
    }
    if (newHover == -1) QToolTip::hideText();
}
