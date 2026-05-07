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
    void onComboChanged(int combo);

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
        float prevX = 0.0f, prevY = 0.0f;  // 上一帧位置
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

    struct HitEffect {
        float x, y;
        enum Type { HitImage, TextPerfect, TextGood, TextMiss, ComboText }; // 新增 ComboText
        Type type;
        float lifetime;
        int combo = 0; // 用于 ComboText 类型，存储当前连击数
    };
    std::vector<HitEffect> m_hitEffects;
    QPixmap m_pixHitEffect;
    QPixmap m_pixPerfect;
    QPixmap m_pixGood;
    QPixmap m_pixMiss;

    void updateEffects(float dt);
    void drawEffects(QPainter& painter);

    // 判定线动态反馈
    qint64 m_lastHitTimeMs = 0;
    qint64 m_lastMissTimeMs = 0;

    // 涟漪特效
    struct Ripple {
        float x, y;        // 屏幕坐标
        float radius;      // 当前半径
        float maxRadius;   // 最大半径
        float lifetime;    // 剩余生命（秒）
        float maxLifetime; // 总生命（秒）
    };
    std::vector<Ripple> m_ripples;
};

#endif // VISUALIZER_H#pragma once