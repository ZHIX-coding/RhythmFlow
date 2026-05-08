#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <vector>

class RhythmGame;

// Visualizer：可视化绘制类
// 负责频谱柱子、粒子系统、涟漪特效、游戏界面绘制、判定特效
class Visualizer : public QWidget
{
    Q_OBJECT
public:
    static constexpr int BAR_COUNT = 64;             // 频谱柱数量
    static constexpr float BAR_HEIGHT_SCALE = 3.0f;  // 柱子高度系数

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

    // 粒子结构体
    struct Particle {
        float x, y;
        float vx, vy;
        float life;
        float prevX = 0.0f, prevY = 0.0f;   // 上一帧位置（轨迹）
    };
    std::vector<Particle> m_particles;
    QTimer* m_particleTimer = nullptr;

    // 鼠标交互
    bool m_mousePressed = false;
    QPointF m_mousePos = QPointF(0.5, 0.35);
    QPointF m_currentAttractor = QPointF(0.5, 0.35);
    static constexpr float ATTRACTOR_SMOOTHING = 0.3f;

    void initParticles();
    void updateParticles();
    void drawParticles(QPainter& painter);

    // 游戏相关
    RhythmGame* m_game = nullptr;
    void drawGame(QPainter& painter);

    // 音符素材
    QPixmap m_pixNoteNormal;
    QPixmap m_pixNoteStrong;
    bool m_useImageNotes = false;

    // 特效系统
    struct HitEffect {
        float x, y;
        enum Type { HitImage, TextPerfect, TextGood, TextMiss, ComboText };
        Type type;
        float lifetime;
        int combo = 0;
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
        float x, y;
        float radius;
        float maxRadius;
        float lifetime;
        float maxLifetime;
    };
    std::vector<Ripple> m_ripples;
};

#endif // VISUALIZER_H