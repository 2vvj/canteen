#include "gachawidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QtMath>

GachaWidget::GachaWidget(QWidget *parent)
    : QWidget(parent), m_state(DROP), m_bagY(-600), m_ribbonPull(0),
      m_isDragging(false), m_burstFrame(0), m_particleFrame(0),
      m_revealAlpha(0), m_idleBob(0), m_showCloseBtn(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    m_bento = QPixmap("bento.png");
    m_ribbon = QPixmap("ribbon.png");

    m_burstTimer = new QTimer(this);
    m_burstTimer->setInterval(25);
    connect(m_burstTimer, &QTimer::timeout, this, &GachaWidget::onBurstTick);

    m_particleTimer = new QTimer(this);
    m_particleTimer->setInterval(40);
    connect(m_particleTimer, &QTimer::timeout, this, &GachaWidget::onSteamTick);

    m_idleTimer = new QTimer(this);
    m_idleTimer->setInterval(50);
    connect(m_idleTimer, &QTimer::timeout, this, [this]() {
        if (m_state == IDLE) { m_idleBob += 0.04; update(); }
    });
    m_idleTimer->start();
}

void GachaWidget::startDraw(const QVector<Dish> &candidates,
                             const QMap<QString, double> &weights) {
    m_candidates = candidates;
    m_weights = weights;
    m_bagY = -600;
    m_ribbonPull = 0;
    m_isDragging = false;
    m_burstFrame = 0;
    m_particleFrame = 0;
    m_revealAlpha = 0;
    m_idleBob = 0;
    m_showCloseBtn = false;
    m_particles.clear();
    m_sparkles.clear();

    pickWinner();

    auto *dropAnim = new QPropertyAnimation(this, "bagY");
    dropAnim->setDuration(700);
    dropAnim->setStartValue(-600.0);
    // 缩放到显示宽度~400
    double scale = 400.0 / qMax(m_bento.width(), 1);
    int dispH2 = static_cast<int>(m_bento.height() * scale);
    int centerY = height() * 2 / 5 - dispH2 / 2; // 屏幕上方40%处
    if (centerY < 20) centerY = 20;
    dropAnim->setEndValue(centerY);
    dropAnim->setEasingCurve(QEasingCurve::OutBounce);
    connect(dropAnim, &QPropertyAnimation::finished, this, [this]() {
        m_state = IDLE;
        update();
    });
    m_state = DROP;
    dropAnim->start(QAbstractAnimation::DeleteWhenStopped);
    show(); raise(); activateWindow();
}

void GachaWidget::pickWinner() {
    if (m_candidates.isEmpty()) return;
    double totalW = 0;
    for (const auto &d : m_candidates) totalW += m_weights.value(d.name, 0.1);
    double rand = QRandomGenerator::global()->generateDouble() * totalW;
    for (const auto &d : m_candidates) {
        rand -= m_weights.value(d.name, 0.1);
        if (rand <= 0) { m_winner = d; m_winner.drawCount++; return; }
    }
    m_winner = m_candidates.last();
    m_winner.drawCount++;
}

// ============ 绘制 ============

void GachaWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(50, 25, 10, 175));

    int cx = width() / 2;
    int by = static_cast<int>(m_bagY);
    double bob = (m_state == IDLE) ? qSin(m_idleBob) * 2.5 : 0;

    // 便当盒显示尺寸（原图缩放到显示宽度约400px）
    double bentoScale = 400.0 / qMax(m_bento.width(), 1);
    int dispW = static_cast<int>(m_bento.width() * bentoScale);
    int dispH = static_cast<int>(m_bento.height() * bentoScale);

    // ---- 便当盒 ----
    if (!m_bento.isNull()) {
        int bx = cx - dispW / 2;
        p.drawPixmap(QRect(bx, by + static_cast<int>(bob), dispW, dispH), m_bento);
    }

    // ---- 丝带（叠在便当盒上方，随拖拽右移） ----
    double pull = m_ribbonPull;
    if (!m_ribbon.isNull() && m_state != BURST && m_state != REVEAL) {
        double ribbonScale = bentoScale; // 同比例缩放
        int rw = static_cast<int>(m_ribbon.width() * ribbonScale);
        int rh = static_cast<int>(m_ribbon.height() * ribbonScale);
        int ribbonX = cx - rw / 2 + static_cast<int>(pull * 300); // 拖拽可移300px
        int ribbonY = by + dispH / 2 - rh / 2 + static_cast<int>(bob);

        p.save();
        p.drawPixmap(QRect(ribbonX, ribbonY, rw, rh), m_ribbon);
        p.restore();

        // 提示
        if (m_state == IDLE && pull < 0.05) {
            p.setPen(QColor(255, 255, 255, 140 + static_cast<int>(qSin(m_idleBob * 2) * 60)));
            QFont hintFont("Microsoft YaHei", 13);
            p.setFont(hintFont);
            p.drawText(QRectF(cx - 120, by + dispH + 25, 240, 25), Qt::AlignHCenter, "向右拖动丝带 →");
        }
    }

    // ---- 拉开后的暖光 ----
    if (pull > 0.1 && m_state != BURST && m_state != REVEAL) {
        QPointF center(cx, by + dispH / 2 + bob);
        QRadialGradient glow(center, qMin(dispW, dispH) / 2);
        glow.setColorAt(0, QColor(255, 240, 200, static_cast<int>(pull * 100)));
        glow.setColorAt(1, QColor(255, 220, 150, 0));
        p.setBrush(glow); p.setPen(Qt::NoPen);
        p.drawEllipse(center, dispW / 2, dispH / 2);
    }

    // ---- 光芒爆发 ----
    if (m_state == BURST) {
        int alpha = qMin(200, m_burstFrame * 10);
        double r = m_burstFrame * 16;
        QPointF center(cx, by + dispH / 2 + bob);

        for (int layer = 0; layer < 3; ++layer) {
            QRadialGradient glow(center, r * (1 + layer * 0.5));
            QColor c = layer == 0 ? QColor(255, 245, 200) :
                       layer == 1 ? QColor(255, 210, 120) : QColor(255, 170, 70);
            glow.setColorAt(0, QColor(c.red(), c.green(), c.blue(), alpha / (layer + 1)));
            glow.setColorAt(0.5, QColor(c.red(), c.green(), c.blue(), alpha / (layer + 2)));
            glow.setColorAt(1, QColor(c.red(), c.green(), c.blue(), 0));
            p.setBrush(glow);
            p.setPen(Qt::NoPen);
            p.drawEllipse(center, r * (1 + layer * 0.5), r * (1 + layer * 0.5));
        }

        // 射线
        p.setPen(QPen(QColor(255, 240, 150, alpha / 2), 1.5));
        for (int i = 0; i < 16; ++i) {
            double angle = i * M_PI / 8 + m_burstFrame * 0.08;
            int len = static_cast<int>(r) + 20 + m_burstFrame * 3;
            p.drawLine(QPointF(center.x(), center.y()),
                       QPointF(center.x() + qCos(angle) * len,
                               center.y() + qSin(angle) * len));
        }

        // 闪光粒子
        for (const auto &sp : m_sparkles) {
            double sx = center.x() + sp.x * (1 + m_burstFrame * 0.05);
            double sy = center.y() + sp.y * (1 + m_burstFrame * 0.05);
            double sz = 3 + sp.size * 5;
            p.setBrush(QColor(255, 250, 210, qBound(0, (int)sp.alpha, 255)));
            p.setPen(Qt::NoPen);
            QPainterPath star;
            for (int i = 0; i < 4; ++i) {
                double a = i * M_PI / 2;
                star.moveTo(sx + qCos(a) * sz, sy + qSin(a) * sz);
                a += M_PI / 4;
                star.lineTo(sx + qCos(a) * sz * 0.3, sy + qSin(a) * sz * 0.3);
            }
            star.closeSubpath();
            p.drawPath(star);
        }
    }

    // ---- 蒸汽粒子 ----
    if (m_state == BURST || m_state == REVEAL) {
        for (const auto &pt : m_particles) {
            int a = qBound(0, (int)pt.x, 200);
            double r = pt.y;
            double sx = cx + qSin(r * 5 + m_particleFrame * 0.08) * 50;
            double sy = by - 40 - r * 120 + bob;
            QRadialGradient sg(sx, sy, r * 14);
            sg.setColorAt(0, QColor(255, 252, 245, a));
            sg.setColorAt(1, QColor(255, 242, 225, 0));
            p.setBrush(sg);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(sx, sy), r * 14, r * 14);
        }
    }

    // ---- 菜品浮现 ----
    if (m_state == REVEAL && !m_winner.name.isEmpty()) {
        int a = static_cast<int>(m_revealAlpha * 255);
        int cardY = by + dispH + 20;

        // 食物emoji
        QFont emojiFont("Segoe UI Emoji", 48);
        p.setFont(emojiFont);
        p.drawText(QRectF(cx - 60, cardY - 60, 120, 60), Qt::AlignCenter, "🍱");

        // 菜名卡片
        p.setBrush(QColor(255, 252, 245, a));
        p.setPen(QPen(QColor(200, 165, 120, a), 2.5));
        p.drawRoundedRect(QRectF(cx - 180, cardY, 360, 80), 18, 18);

        p.setPen(QColor(90, 45, 15, a));
        QFont nameFont("Microsoft YaHei", 22, QFont::Bold);
        p.setFont(nameFont);
        p.drawText(QRectF(cx - 160, cardY + 8, 320, 32), Qt::AlignHCenter, m_winner.name);

        p.setPen(QColor(130, 85, 45, static_cast<int>(a * 0.75)));
        QFont detailFont("Microsoft YaHei", 13);
        p.setFont(detailFont);
        QString detail = m_winner.restaurant + "    ¥" + QString::number(m_winner.price, 'f', 1)
                         + "    " + QString::number(m_winner.calories, 'f', 0) + "kcal";
        p.drawText(QRectF(cx - 160, cardY + 38, 320, 28), Qt::AlignHCenter, detail);
    }

    // ---- 关闭按钮 ----
    if (m_showCloseBtn) {
        int btnY = by + dispH + 115;
        QRectF btnR(cx - 55, btnY, 110, 38);
        p.setBrush(QColor(255, 255, 250, 230));
        p.setPen(QPen(QColor(190, 150, 110), 2));
        p.drawRoundedRect(btnR, 19, 19);
        p.setPen(QColor(110, 70, 35));
        QFont btnFont("Microsoft YaHei", 14, QFont::Bold);
        p.setFont(btnFont);
        p.drawText(btnR, Qt::AlignCenter, "确 认");
        p.setPen(QColor(255, 255, 255, 120));
        QFont hintFont("Microsoft YaHei", 10);
        p.setFont(hintFont);
        p.drawText(QRectF(cx - 120, btnY + 42, 240, 18), Qt::AlignCenter, "点击任意位置关闭");
    }
}

// ============ 动画逻辑（不变） ============

void GachaWidget::startBurst() {
    m_state = BURST; m_burstFrame = 0; m_particleFrame = 0;
    m_particles.clear(); m_sparkles.clear();
    for (int i = 0; i < 30; ++i) {
        Particle pt; pt.x = QRandomGenerator::global()->bounded(200);
        pt.y = QRandomGenerator::global()->generateDouble() * 1.8; m_particles.append(pt);
    }
    for (int i = 0; i < 35; ++i) {
        double angle = QRandomGenerator::global()->generateDouble() * M_PI * 2;
        Sparkle sp; sp.x = qCos(angle) * QRandomGenerator::global()->bounded(180.0);
        sp.y = qSin(angle) * QRandomGenerator::global()->bounded(180.0);
        sp.size = QRandomGenerator::global()->generateDouble();
        sp.alpha = 200 + QRandomGenerator::global()->generateDouble() * 55;
        m_sparkles.append(sp);
    }
    m_burstTimer->start(); m_particleTimer->start();
}

void GachaWidget::onBurstTick() {
    m_burstFrame++;
    for (auto &sp : m_sparkles) {
        sp.alpha -= 6;
        if (sp.alpha < 0) sp.alpha = 200 + QRandomGenerator::global()->generateDouble() * 55;
    }
    if (m_burstFrame > 28) {
        m_burstTimer->stop(); m_state = REVEAL; m_revealAlpha = 0;
        auto *revealTimer = new QTimer(this);
        connect(revealTimer, &QTimer::timeout, this, [this, revealTimer]() {
            m_revealAlpha += 0.055;
            if (m_revealAlpha >= 1.0) {
                m_revealAlpha = 1.0; revealTimer->stop(); revealTimer->deleteLater();
                m_particleTimer->stop(); m_showCloseBtn = true;
            }
            update();
        });
        revealTimer->start(30);
        return;
    }
    update();
}

void GachaWidget::onSteamTick() {
    m_particleFrame++;
    for (auto &p : m_particles) { p.x -= 5; if (p.x < 0) p.x = 200; }
    update();
}

// ============ 鼠标拖拽丝带 ============

void GachaWidget::mousePressEvent(QMouseEvent *event) {
    if (m_showCloseBtn && m_state == REVEAL) {
        m_idleTimer->stop();
        hide(); // 先隐藏避免闪烁
        emit dishSelected(m_winner);
        deleteLater();
        return;
    }
    if (m_state != IDLE) return;
    // 丝带右端为拖拽点
    double scale2 = 400.0 / qMax(m_bento.width(), 1);
    int cx = width() / 2;
    int by = static_cast<int>(m_bagY);
    int bdh = static_cast<int>(m_bento.height() * scale2);
    int rw = static_cast<int>(m_ribbon.width() * scale2);
    int rh = static_cast<int>(m_ribbon.height() * scale2);
    int ribbonLeft = cx - rw / 2;
    int ribbonRight = ribbonLeft + rw;
    int ribbonY = by + bdh / 2 - rh / 2;
    // 可拖拽区域：丝带中右段（不要太靠边）
    QRectF ribbonTail(ribbonRight - 130, ribbonY - 10, 140, rh + 20);
    if (ribbonTail.contains(event->pos())) {
        m_isDragging = true; m_state = DRAGGING;
        m_lastMousePos = event->pos(); setCursor(Qt::ClosedHandCursor);
    }
}

void GachaWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_state == IDLE) {
        double sc3 = 400.0 / qMax(m_bento.width(), 1);
        int cx2 = width() / 2, by2 = static_cast<int>(m_bagY);
        int bdh2 = static_cast<int>(m_bento.height() * sc3);
        int rw2 = static_cast<int>(m_ribbon.width() * sc3);
        int rh2 = static_cast<int>(m_ribbon.height() * sc3);
        int ribbonRight2 = cx2 - rw2 / 2 + rw2;
        int ribbonY2 = by2 + bdh2 / 2 - rh2 / 2;
        QRectF tail(ribbonRight2 - 130, ribbonY2 - 10, 140, rh2 + 20);
        setCursor(tail.contains(event->pos()) ? Qt::OpenHandCursor : Qt::ArrowCursor);
        return;
    }
    if (!m_isDragging || m_state != DRAGGING) return;
    int dx = event->pos().x() - m_lastMousePos.x();
    m_lastMousePos = event->pos();
    if (dx > 0) m_ribbonPull = qMin(1.0, m_ribbonPull + dx / 350.0);
    update();
}

void GachaWidget::mouseReleaseEvent(QMouseEvent *) {
    if (!m_isDragging) return;
    m_isDragging = false; setCursor(Qt::ArrowCursor);
    if (m_ribbonPull >= 0.5) {
        m_ribbonPull = 1.0; update(); startBurst();
    } else {
        auto *snapBack = new QPropertyAnimation(this, "ribbonPull");
        snapBack->setDuration(350);
        snapBack->setStartValue(m_ribbonPull); snapBack->setEndValue(0.0);
        snapBack->setEasingCurve(QEasingCurve::OutBack);
        connect(snapBack, &QPropertyAnimation::finished, this, [this]() {
            m_state = IDLE; update();
        });
        snapBack->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

double GachaWidget::ribbonPull() const { return m_ribbonPull; }
void GachaWidget::setRibbonPull(double v) { m_ribbonPull = v; update(); }
double GachaWidget::bagY() const { return m_bagY; }
void GachaWidget::setBagY(double y) { m_bagY = y; update(); }
double GachaWidget::dragProgress() const { return m_ribbonPull; }
void GachaWidget::setDragProgress(double p) { m_ribbonPull = p; update(); }
