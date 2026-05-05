#include "Visualizer.h"
#include <QTimer>
#include <QRandomGenerator>
#include <QPainter>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QtMath>
#include "RhythmGame.h"

Visualizer::Visualizer(QWidget* parent) : QWidget(parent)
{
    m_spectrum.resize(BAR_COUNT, 0.0f);
    setMouseTracking(true);

    initParticles();

    m_particleTimer = new QTimer(this);
    connect(m_particleTimer, &QTimer::timeout, this, &Visualizer::updateParticles);
    m_particleTimer->start(16);

    // ===== 加载音符素材 =====
    m_pixNoteNormal.load(":/RhythmFlow/resources/note1.png");
    m_pixNoteStrong.load(":/RhythmFlow/resources/note2.png");

    if (!m_pixNoteNormal.isNull() && !m_pixNoteStrong.isNull()) {
        m_useImageNotes = true;
    }
    else {
        m_useImageNotes = false;
    }

    // 加载特效素材
    m_pixHitEffect.load(":/RhythmFlow/resources/image1.png");
    m_pixPerfect.load(":/RhythmFlow/resources/perfect.png");
    m_pixGood.load(":/RhythmFlow/resources/good.png");
    m_pixMiss.load(":/RhythmFlow/resources/MISS.png");
}

void Visualizer::setSpectrum(const std::vector<float>& data)
{
    if (data.size() == BAR_COUNT) {
        m_spectrum = data;
        update();
    }
}

void Visualizer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 10, 20, 30));

    drawParticles(painter);

    if (m_spectrum.empty())
        return;

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
            alpha0 = 180;   // 底部青色半透明
            alpha1 = 100;  // 中部浅青白
            alpha2 = 120;  // 顶部白色
        }

        grad.setColorAt(0, QColor(0, 255, 255, alpha0));
        grad.setColorAt(0.6, QColor(180, 255, 255, alpha1));
        grad.setColorAt(1, QColor(255, 255, 255, alpha2));

        painter.fillRect(i * barWidth, height() - barHeight,
            barWidth - 2, barHeight, grad);
    }
    drawGame(painter);
}

void Visualizer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_mousePos = QPointF(
            qBound(0.0, event->position().x() / width(), 1.0),
            qBound(0.0, event->position().y() / height(), 1.0)
        );
    }
    QWidget::mousePressEvent(event);
}

void Visualizer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void Visualizer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_mousePressed) {
        m_mousePos = QPointF(
            qBound(0.0, event->position().x() / width(), 1.0),
            qBound(0.0, event->position().y() / height(), 1.0)
        );
    }
    QWidget::mouseMoveEvent(event);
}

void Visualizer::initParticles()
{
    const int PARTICLE_COUNT = 2000;
    m_particles.resize(PARTICLE_COUNT);

    const float centerX = 0.5f;
    const float centerY = 0.35f;
    const float radiusX = 0.45f;
    const float radiusY = 0.35f;

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

    m_currentAttractor.setX(m_currentAttractor.x() * (1.0f - ATTRACTOR_SMOOTHING) + targetAttractor.x() * ATTRACTOR_SMOOTHING);
    m_currentAttractor.setY(m_currentAttractor.y() * (1.0f - ATTRACTOR_SMOOTHING) + targetAttractor.y() * ATTRACTOR_SMOOTHING);

    float attractBase = m_mousePressed ? 0.04f : 0.006f;
    float tangentialBase = m_mousePressed ? 0.001f : 0.0008f;
    float idealDist = m_mousePressed ? 0.06f : 0.15f;

    const float damping = 0.97f;
    const float minDist = 0.01f;

    for (auto& p : m_particles) {
        float dx = m_currentAttractor.x() - p.x;
        float dy = m_currentAttractor.y() - p.y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < minDist) dist = minDist;

        float radialX = dx / dist;
        float radialY = dy / dist;
        float tangentX = -radialY;
        float tangentY = radialX;

        float attractStrength = attractBase * (1.0f - bassNorm * 0.f);
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
            p.vx += noise;
            p.vy += noise;
        }

        p.vx *= damping;
        p.vy *= damping;

        p.x += p.vx;
        p.y += p.vy;

        if (p.x < 0.0f) { p.x = 0.0f; p.vx *= -0.3f; }
        if (p.x > 1.0f) { p.x = 1.0f; p.vx *= -0.3f; }
        if (p.y < 0.0f) { p.y = 0.0f; p.vy *= -0.3f; }
        if (p.y > 1.0f) {
            p.y = 1.0f;
            p.vy *= -0.15f;
            if (QRandomGenerator::global()->bounded(1.0) < 0.1f) p.vy -= 0.005f;
        }
    }

    // 更新特效生命周期
    updateEffects(0.016f);
    update();
}

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
            float distToMouse = sqrt(pow(p.x - m_mousePos.x(), 2) + pow(p.y - m_mousePos.y(), 2));
            t = qBound(0.0f, 1.0f - distToMouse * 0.8f, 1.0f);
        }

        int r = coldColor.red() * (1 - t) + warmColor.red() * t;
        int g = coldColor.green() * (1 - t) + warmColor.green() * t;
        int b = coldColor.blue() * (1 - t) + warmColor.blue() * t;
        int a = coldColor.alpha() * (1 - t) + warmColor.alpha() * t;

        QColor particleColor(r, g, b, a);

        float screenX = p.x * width();
        float screenY = p.y * height();
        float radius = baseSize * (0.7f + 0.5f * t);

        QRadialGradient grad(screenX, screenY, radius);
        grad.setColorAt(0.0, particleColor);
        grad.setColorAt(0.6, QColor(r, g, b, a * 0.4));
        grad.setColorAt(1.0, QColor(r, g, b, 0));

        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(screenX, screenY), radius, radius);
    }
}

void Visualizer::drawGame(QPainter& painter)
{
    if (!m_game || !m_game->isActive()) return;

    const int trackCount = 4;
    const float trackSpacing = width() / (trackCount + 1.0f);
    const float hitYPos = height() * 0.74f;

    // 轨道线
    QColor trackColor(255, 255, 255, 220);
    for (int i = 0; i < trackCount; ++i) {
        float x = (i + 1) * trackSpacing;
        for (int glow = 2; glow >= 0; --glow) {
            int alpha = 30 - glow * 8;
            int penWidth = 1 + glow * 3;
            painter.setPen(QPen(QColor(255, 200, 50, alpha), penWidth));
            painter.drawLine(QLineF(x, 0.0, x, hitYPos));
        }
        painter.setPen(QPen(trackColor, 2));
        painter.drawLine(QLineF(x, 0.0, x, hitYPos));
    }

    // 判定线
    QColor lineColor(255, 255, 255, 220);
    float lineY = hitYPos;
    float screenW = static_cast<float>(width());
    for (int glow = 3; glow >= 0; --glow) {
        int alpha = 40 - glow * 10;
        int penWidth = 2 + glow * 4;
        painter.setPen(QPen(QColor(255, 220, 80, alpha), penWidth));
        painter.drawLine(QLineF(0.0, lineY, screenW, lineY));
    }
    painter.setPen(QPen(lineColor, 2));
    painter.drawLine(QLineF(0.0, lineY, screenW, lineY));

    // 绘制音符（仅素材图片）
    painter.setPen(Qt::NoPen);
    for (const auto& note : m_game->notes()) {
        float x = (note.track + 1) * trackSpacing;
        float y = note.y * height();

        if (m_useImageNotes) {
            QPixmap& pix = note.isStrong ? m_pixNoteStrong : m_pixNoteNormal;
            float targetW = note.isStrong ? 100.0f : 100.0f;
            float targetH = targetW * (pix.height() / (float)pix.width());
            QRectF targetRect(x - targetW / 2, y - targetH / 2, targetW, targetH);
            painter.drawPixmap(targetRect, pix, pix.rect());
        }
    }

    // 分数和连击
    painter.setPen(QColor(255, 255, 255, 200));
    painter.setFont(QFont(QStringLiteral("Arial"), 16, QFont::Bold));
    painter.drawText(20, 40, QString("Score: %1").arg(m_game->score()));
    painter.drawText(20, 65, QString("Combo: %1").arg(m_game->combo()));

    // 绘制特效（最上层）
    drawEffects(painter);
}

// ========== 特效相关函数 ==========
void Visualizer::updateEffects(float dt)
{
    for (auto& e : m_hitEffects)
        e.lifetime -= dt;
    m_hitEffects.erase(std::remove_if(m_hitEffects.begin(), m_hitEffects.end(),
        [](const HitEffect& e) { return e.lifetime <= 0.0f; }),
        m_hitEffects.end());
}

void Visualizer::drawEffects(QPainter& painter)
{
    for (const auto& e : m_hitEffects) {
        QPixmap* pix = nullptr;
        float sizeW = 0.0f;

        switch (e.type) {
        case HitEffect::HitImage: pix = &m_pixHitEffect; sizeW = 80.0f; break; // 击中图片宽度
        case HitEffect::TextPerfect: pix = &m_pixPerfect; sizeW = 140.0f; break; // Perfect 宽度
        case HitEffect::TextGood: pix = &m_pixGood; sizeW = 120.0f; break; // Good 宽度
        case HitEffect::TextMiss: pix = &m_pixMiss; sizeW = 120.0f; break; // Miss 宽度
        }
        if (!pix || pix->isNull()) continue;

        float alpha = qBound(0.0f, e.lifetime / 0.3f, 1.0f);
        painter.setOpacity(alpha);
        float h = sizeW * (pix->height() / (float)pix->width());
        painter.drawPixmap(QRectF(e.x - sizeW / 2, e.y - h / 2, sizeW, h), *pix, pix->rect());
    }
    painter.setOpacity(1.0f);
}

void Visualizer::onNoteHit(int track, float y, int grade)
{
    const int trackCount = 4;
    float spacing = width() / (trackCount + 1.0f);
    float sx = (track + 1) * spacing;
    float sy = y * height();

    // 1. 击中特效（图片）
    m_hitEffects.push_back({ sx, sy, HitEffect::HitImage, 0.2f });

    // 2. 判定文字（不重叠，放在判定线右上方）
    float textX = width() * 0.92f;
    float textY = height() * 0.74f - 75;
    HitEffect::Type textType = (grade == 0) ? HitEffect::TextPerfect : HitEffect::TextGood;
    m_hitEffects.push_back({ textX, textY, textType, 0.3f });
}

void Visualizer::onNoteMissed(int track)
{
    float textX = width() * 0.92f;
    float textY = height() * 0.74f - 95;
    m_hitEffects.push_back({ textX, textY, HitEffect::TextMiss, 0.3f });
}