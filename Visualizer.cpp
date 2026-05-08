#include "Visualizer.h"
#include <QTimer>
#include <QRandomGenerator>
#include <QPainter>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QtMath>
#include "RhythmGame.h"
#include <QDateTime>

// 构造函数：初始化频谱缓冲区、粒子系统、加载素材
Visualizer::Visualizer(QWidget* parent) : QWidget(parent)
{
    m_spectrum.resize(BAR_COUNT, 0.0f);
    setMouseTracking(true);

    initParticles();

    m_particleTimer = new QTimer(this);
    connect(m_particleTimer, &QTimer::timeout, this, &Visualizer::updateParticles);
    m_particleTimer->start(16);

    // 加载音符素材
    m_pixNoteNormal.load(":/RhythmFlow/resources/note1.png");
    m_pixNoteStrong.load(":/RhythmFlow/resources/note2.png");
    m_useImageNotes = !m_pixNoteNormal.isNull() && !m_pixNoteStrong.isNull();

    // 加载特效素材
    m_pixHitEffect.load(":/RhythmFlow/resources/image1.png");
    m_pixPerfect.load(":/RhythmFlow/resources/perfect.png");
    m_pixGood.load(":/RhythmFlow/resources/good.png");
    m_pixMiss.load(":/RhythmFlow/resources/MISS.png");
}

// 接收频谱数据，触发重绘
void Visualizer::setSpectrum(const std::vector<float>& data)
{
    if (data.size() == BAR_COUNT) {
        m_spectrum = data;
        update();
    }
}

// 主绘制函数：粒子 → 涟漪 → 柱子 → 游戏界面
void Visualizer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 10, 20, 30));

    drawParticles(painter);

    // 涟漪（粒子层之上，柱子层之下）
    for (const auto& r : m_ripples) {
        float alpha = r.lifetime / r.maxLifetime;
        QColor rippleColor(255, 200, 100, static_cast<int>(120 * alpha));
        painter.setPen(QPen(rippleColor, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPointF(r.x, r.y), r.radius, r.radius);
    }

    if (m_spectrum.empty()) return;

    // 绘制频谱柱子
    int barWidth = width() / BAR_COUNT;
    int maxTotalHeight = height() - 100;

    for (int i = 0; i < BAR_COUNT; ++i) {
        float energy = m_spectrum[i];
        float exponent = 2.3f;
        float boosted = powf(energy, exponent) / 650.0f;
        if (boosted > 120.0f) boosted = 120.0f;
        if (boosted < 1.0f) boosted = 1.0f;

        int barHeight = static_cast<int>(boosted * BAR_HEIGHT_SCALE);
        barHeight = qMin(barHeight, maxTotalHeight);
        if (barHeight < 2) barHeight = 2;

        QLinearGradient grad(0, height(), 0, height() - barHeight);

        // 游戏模式下降低柱子透明度
        int alpha0 = 220, alpha1 = 240, alpha2 = 255;
        if (m_game && m_game->isActive()) {
            alpha0 = 180; alpha1 = 100; alpha2 = 120;
        }
        grad.setColorAt(0, QColor(0, 255, 255, alpha0));
        grad.setColorAt(0.6, QColor(180, 255, 255, alpha1));
        grad.setColorAt(1, QColor(255, 255, 255, alpha2));

        painter.fillRect(i * barWidth, height() - barHeight, barWidth - 2, barHeight, grad);
    }
    drawGame(painter);
}

// 鼠标按下：设置聚拢目标 + 生成多圈涟漪
void Visualizer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_mousePos = QPointF(
            qBound(0.0, event->position().x() / width(), 1.0),
            qBound(0.0, event->position().y() / height(), 1.0));

        // 生成 5 圈涟漪
        int rippleCount = 5;
        float baseMaxRadius = 40.0f;
        float radiusStep = 20.0f;
        float lifetime = 1.0f;
        for (int i = 0; i < rippleCount; ++i) {
            Ripple rip;
            rip.x = event->position().x();
            rip.y = event->position().y();
            rip.radius = 5.0f;
            rip.maxRadius = baseMaxRadius + i * radiusStep;
            rip.lifetime = lifetime;
            rip.maxLifetime = lifetime;
            m_ripples.push_back(rip);
        }
    }
    QWidget::mousePressEvent(event);
}

// 鼠标释放：取消聚拢
void Visualizer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) m_mousePressed = false;
    QWidget::mouseReleaseEvent(event);
}

// 鼠标移动：更新聚拢目标位置
void Visualizer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_mousePressed) {
        m_mousePos = QPointF(
            qBound(0.0, event->position().x() / width(), 1.0),
            qBound(0.0, event->position().y() / height(), 1.0));
    }
    QWidget::mouseMoveEvent(event);
}

// 初始化 2000 个粒子（椭圆分布）
void Visualizer::initParticles()
{
    const int PARTICLE_COUNT = 2000;
    m_particles.resize(PARTICLE_COUNT);

    const float centerX = 0.5f, centerY = 0.35f;
    const float radiusX = 0.45f, radiusY = 0.35f;

    for (auto& p : m_particles) {
        float angle = QRandomGenerator::global()->generateDouble() * 2.0f * M_PI;
        float r = sqrt(QRandomGenerator::global()->generateDouble());
        p.x = centerX + r * radiusX * cos(angle);
        p.y = centerY + r * radiusY * sin(angle);
        p.vx = (QRandomGenerator::global()->generateDouble() - 0.5f) * 0.002f;
        p.vy = (QRandomGenerator::global()->generateDouble() - 0.5f) * 0.002f;
        p.life = 1.0f;
    }
}

// 每帧更新粒子物理：向心力 + 切向力 + 低频扰动 + 拖尾
void Visualizer::updateParticles()
{
    if (m_spectrum.empty()) return;

    float bassEnergy = 0.0f;
    for (int i = 0; i < 3; ++i) bassEnergy += m_spectrum[i];
    bassEnergy /= 3.0f;
    float bassNorm = qBound(0.0f, bassEnergy / 60.0f, 1.0f);

    QPointF targetAttractor;
    if (m_mousePressed) {
        targetAttractor = QPointF(m_mousePos.x(), qMin(m_mousePos.y(), 0.65f));
    }
    else {
        targetAttractor = QPointF(0.5f, 0.35f);
    }
    m_currentAttractor = m_currentAttractor * (1.0f - ATTRACTOR_SMOOTHING) + targetAttractor * ATTRACTOR_SMOOTHING;

    float attractBase = m_mousePressed ? 0.04f : 0.006f;
    float tangentialBase = m_mousePressed ? 0.001f : 0.0008f;
    float idealDist = m_mousePressed ? 0.06f : 0.15f;
    const float damping = 0.97f, minDist = 0.01f;

    for (auto& p : m_particles) {
        float dx = m_currentAttractor.x() - p.x;
        float dy = m_currentAttractor.y() - p.y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < minDist) dist = minDist;

        float radialX = dx / dist, radialY = dy / dist;
        float tangentX = -radialY, tangentY = radialX;

        float attractStrength = attractBase;
        float radialForce = attractStrength * (dist * 2.5f);
        float distFactor = exp(-fabs(dist - idealDist) * 4.0f);
        float tangentialForce = tangentialBase * distFactor;

        p.vx += radialX * radialForce + tangentX * tangentialForce;
        p.vy += radialY * radialForce + tangentY * tangentialForce;

        float microNoise = 0.0008f;
        p.vx += (QRandomGenerator::global()->generateDouble() - 0.5f) * microNoise;
        p.vy += (QRandomGenerator::global()->generateDouble() - 0.5f) * microNoise;

        if (bassNorm > 0.1f) {
            float noise = (QRandomGenerator::global()->generateDouble() - 0.5f) * 0.015f * bassNorm;
            p.vx += noise; p.vy += noise;
        }

        p.vx *= damping; p.vy *= damping;

        p.prevX = p.x; p.prevY = p.y;
        p.x += p.vx; p.y += p.vy;

        if (p.x < 0.0f) { p.x = 0.0f; p.vx *= -0.3f; }
        if (p.x > 1.0f) { p.x = 1.0f; p.vx *= -0.3f; }
        if (p.y < 0.0f) { p.y = 0.0f; p.vy *= -0.3f; }
        if (p.y > 1.0f) { p.y = 1.0f; p.vy *= -0.15f; }
    }

    updateEffects(0.016f);

    // 更新涟漪
    for (auto& r : m_ripples) {
        r.lifetime -= 0.016f;
        r.radius = 5.0f + (r.maxRadius - 5.0f) * (1.0f - r.lifetime / r.maxLifetime);
    }
    m_ripples.erase(std::remove_if(m_ripples.begin(), m_ripples.end(),
        [](const Ripple& r) { return r.lifetime <= 0.0f; }), m_ripples.end());

    update();
}

// 绘制粒子（光点 + 运动轨迹）
void Visualizer::drawParticles(QPainter& painter)
{
    if (m_spectrum.empty() || m_particles.empty()) return;

    float bassEnergy = 0.0f;
    for (int i = 0; i < 3; ++i) bassEnergy += m_spectrum[i];
    bassEnergy /= 3.0f;
    float warmth = qBound(0.0f, bassEnergy / 60.0f, 1.0f);

    QColor coldColor, warmColor;
    if (m_mousePressed) {
        coldColor = QColor(50, 220, 255, 200);
        warmColor = QColor(220, 245, 255, 240);
    }
    else {
        coldColor = QColor(0, 200, 255, 160);
        warmColor = QColor(200, 230, 255, 200);
    }

    float baseSize = qMin(width(), height()) * 0.004f;
    if (m_mousePressed) baseSize *= 1.0f;

    for (const auto& p : m_particles) {
        float t = warmth;
        if (m_mousePressed) {
            float d = sqrt(pow(p.x - m_mousePos.x(), 2) + pow(p.y - m_mousePos.y(), 2));
            t = qBound(0.0f, 1.0f - d * 0.8f, 1.0f);
        }

        int r = coldColor.red() * (1 - t) + warmColor.red() * t;
        int g = coldColor.green() * (1 - t) + warmColor.green() * t;
        int b = coldColor.blue() * (1 - t) + warmColor.blue() * t;
        int a = coldColor.alpha() * (1 - t) + warmColor.alpha() * t;
        QColor particleColor(r, g, b, a);

        float screenX = p.x * width(), screenY = p.y * height();
        float radius = baseSize * (0.5f + bassEnergy / 60.0f);

        // 运动轨迹
        float prevScreenX = p.prevX * width(), prevScreenY = p.prevY * height();
        float dx = screenX - prevScreenX, dy = screenY - prevScreenY;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > 0.5f && dist < 15.0f) {
            QPen trailPen(QColor(r, g, b, a * 0.3f), radius * 0.6f);
            painter.setPen(trailPen);
            painter.drawLine(QPointF(prevScreenX, prevScreenY), QPointF(screenX, screenY));
        }

        QRadialGradient grad(screenX, screenY, radius);
        grad.setColorAt(0.0, particleColor);
        grad.setColorAt(0.6, QColor(r, g, b, a * 0.4));
        grad.setColorAt(1.0, QColor(r, g, b, 0));
        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(screenX, screenY), radius, radius);
    }
}

// 绘制游戏界面：轨道线、判定线、音符、分数、连击数、特效
void Visualizer::drawGame(QPainter& painter)
{
    if (!m_game || !m_game->isActive()) return;

    const int trackCount = 4;
    const float trackSpacing = width() / (trackCount + 1.0f);
    const float hitYPos = height() * 0.74f;

    // 轨道线（亮白霓虹光晕）
    QColor trackColor(255, 255, 255, 220);
    for (int i = 0; i < trackCount; ++i) {
        float x = (i + 1) * trackSpacing;
        for (int glow = 2; glow >= 0; --glow) {
            painter.setPen(QPen(QColor(255, 200, 50, 30 - glow * 8), 1 + glow * 3));
            painter.drawLine(QLineF(x, 0.0, x, hitYPos));
        }
        painter.setPen(QPen(trackColor, 2));
        painter.drawLine(QLineF(x, 0.0, x, hitYPos));
    }

    // 判定线（动态反馈）
    QColor lineColor(255, 255, 255, 220);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QColor dynamicLineColor = lineColor;
    int dynamicLineWidth = 2, glowAlphaBoost = 0;

    if (now - m_lastHitTimeMs < 200) {
        float t = 1.0f - (now - m_lastHitTimeMs) / 200.0f;
        dynamicLineColor = QColor(255, 255, 255, 220 + 35 * t);
        dynamicLineWidth = 2 + 2 * t;
        glowAlphaBoost = 60 * t;
    }
    else if (now - m_lastMissTimeMs < 200) {
        float t = 1.0f - (now - m_lastMissTimeMs) / 200.0f;
        dynamicLineColor = QColor(180, 180, 180, 200 + 55 * t);
        dynamicLineWidth = 2 + 4 * t;
        glowAlphaBoost = 60 * t;
    }

    for (int glow = 3; glow >= 0; --glow) {
        int alpha = qMin(255, 40 - glow * 10 + glowAlphaBoost);
        painter.setPen(QPen(QColor(255, 220, 80, alpha), 2 + glow * 4));
        painter.drawLine(QLineF(0.0, hitYPos, static_cast<float>(width()), hitYPos));
    }
    painter.setPen(QPen(dynamicLineColor, dynamicLineWidth));
    painter.drawLine(QLineF(0.0, hitYPos, static_cast<float>(width()), hitYPos));

    // 音符（PNG 素材）
    painter.setPen(Qt::NoPen);
    for (const auto& note : m_game->notes()) {
        float x = (note.track + 1) * trackSpacing;
        float y = note.y * height();
        if (m_useImageNotes) {
            QPixmap& pix = note.isStrong ? m_pixNoteStrong : m_pixNoteNormal;
            float targetW = 100.0f;
            float targetH = targetW * (pix.height() / (float)pix.width());
            QRectF targetRect(x - targetW / 2, y - targetH / 2, targetW, targetH);
            painter.drawPixmap(targetRect, pix, pix.rect());
        }
    }

    // 分数（左上角）
    painter.setPen(QColor(255, 255, 255, 200));
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.drawText(20, 40, QString("Score: %1").arg(m_game->score()));

    // 静态连击数（右上角）
    int combo = m_game->combo();
    if (combo > 0) {
        painter.setPen(QColor(255, 255, 255, 220));
        painter.setFont(QFont("Arial", 20, QFont::Bold));
        QRectF comboRect(width() - 200, 40, 180, 30);
        painter.drawText(comboRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(combo));
    }

    drawEffects(painter);
}

// 更新特效生命周期
void Visualizer::updateEffects(float dt)
{
    for (auto& e : m_hitEffects) e.lifetime -= dt;
    m_hitEffects.erase(std::remove_if(m_hitEffects.begin(), m_hitEffects.end(),
        [](const HitEffect& e) { return e.lifetime <= 0.0f; }), m_hitEffects.end());
}

// 绘制特效：击中图片、Perfect/Good/Miss 文字、动态连击
void Visualizer::drawEffects(QPainter& painter)
{
    for (const auto& e : m_hitEffects) {
        if (e.type == HitEffect::ComboText) {
            painter.save();
            QFont font("Arial", 60, QFont::Bold);
            painter.setFont(font);
            float progress = e.lifetime / 0.4f;
            float scale = 1.0f + (1.0f - progress) * 0.4f;
            int alpha = static_cast<int>(180 * progress);
            painter.setPen(QColor(180, 180, 180, alpha));
            painter.translate(e.x, e.y);
            painter.scale(scale, scale);
            painter.drawText(QRectF(-60, -30, 120, 60), Qt::AlignCenter, QString::number(e.combo));
            painter.restore();
        }
        else {
            QPixmap* pix = nullptr;
            float sizeW = 0.0f;
            switch (e.type) {
            case HitEffect::HitImage: pix = &m_pixHitEffect; sizeW = 80.0f; break;
            case HitEffect::TextPerfect: pix = &m_pixPerfect; sizeW = 140.0f; break;
            case HitEffect::TextGood: pix = &m_pixGood; sizeW = 120.0f; break;
            case HitEffect::TextMiss: pix = &m_pixMiss; sizeW = 120.0f; break;
            default: continue;
            }
            if (!pix || pix->isNull()) continue;
            float alpha = qBound(0.0f, e.lifetime / 0.3f, 1.0f);
            painter.setOpacity(alpha);
            float h = sizeW * (pix->height() / (float)pix->width());
            painter.drawPixmap(QRectF(e.x - sizeW / 2, e.y - h / 2, sizeW, h), *pix, pix->rect());
        }
    }
    painter.setOpacity(1.0f);
}

// 击中音符：生成击中特效 + Perfect/Good 文字（判定线附近）
void Visualizer::onNoteHit(int track, float y, int grade)
{
    m_lastHitTimeMs = QDateTime::currentMSecsSinceEpoch();
    const int trackCount = 4;
    float spacing = width() / (trackCount + 1.0f);
    float sx = (track + 1) * spacing;
    float sy = y * height();

    m_hitEffects.push_back({ sx, sy, HitEffect::HitImage, 0.15f });

    float textY = height() * 0.74f - 50;
    HitEffect::Type textType = (grade == 0) ? HitEffect::TextPerfect : HitEffect::TextGood;
    m_hitEffects.push_back({ sx, textY, textType, 0.25f });
}

// 错过音符：生成 Miss 文字
void Visualizer::onNoteMissed(int track)
{
    m_lastMissTimeMs = QDateTime::currentMSecsSinceEpoch();
    const int trackCount = 4;
    float spacing = width() / (trackCount + 1.0f);
    float sx = (track + 1) * spacing;
    float textY = height() * 0.74f - 50;
    m_hitEffects.push_back({ sx, textY, HitEffect::TextMiss, 0.25f });
}

// 连击变化：生成动态连击弹出特效（淡灰大字，右上角）
void Visualizer::onComboChanged(int combo)
{
    if (combo > 0) {
        m_hitEffects.push_back({ width() - 200.0f, 60.0f, HitEffect::ComboText, 0.4f, combo });
    }
}