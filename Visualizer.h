#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <vector>

class RhythmGame;

class Visualizer : public QWidget
{
    Q_OBJECT
public:
    static constexpr int BAR_COUNT = 64;
    static constexpr float BAR_HEIGHT_SCALE = 3.0f;

    explicit Visualizer(QWidget* parent = nullptr);
    ~Visualizer() = default;

    void setSpectrum(const std::vector<float>& data);
    void setGame(RhythmGame* game) { m_game = game; }

public slots:
    void onNoteHit(int track, float y, int grade);
    void onNoteMissed(int track);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    std::vector<float> m_spectrum;

    struct Particle {
        float x, y;
        float vx, vy;
        float life;
    };
    std::vector<Particle> m_particles;
    QTimer* m_particleTimer = nullptr;

    bool m_mousePressed = false;
    QPointF m_mousePos = QPointF(0.5, 0.35);
    QPointF m_currentAttractor = QPointF(0.5, 0.35);
    static constexpr float ATTRACTOR_SMOOTHING = 0.3f;

    void initParticles();
    void updateParticles();
    void drawParticles(QPainter& painter);

    RhythmGame* m_game = nullptr;
    void drawGame(QPainter& painter);

    QPixmap m_pixNoteNormal;
    QPixmap m_pixNoteStrong;
    bool m_useImageNotes = false;

    // ÌØÐ§ÏµÍ³
    struct HitEffect {
        float x, y;
        enum Type { HitImage, TextPerfect, TextGood, TextMiss };
        Type type;
        float lifetime;
    };
    std::vector<HitEffect> m_hitEffects;
    QPixmap m_pixHitEffect;
    QPixmap m_pixPerfect;
    QPixmap m_pixGood;
    QPixmap m_pixMiss;

    void updateEffects(float dt);
    void drawEffects(QPainter& painter);
};

#endif // VISUALIZER_H#pragma once
