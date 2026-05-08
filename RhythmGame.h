#ifndef RHYTHMGAME_H
#define RHYTHMGAME_H

#include <QObject>
#include <QSoundEffect>
#include <QUrl>
#include <vector>

// 音符数据结构
struct Note {
    int track;          // 轨道编号 0~3
    float y;            // 相对位置（0=顶部，1=底部）
    bool isHit;         // 是否已被击中
    bool isMissed;      // 是否已错过
    bool isStrong;      // 是否为强拍音符
    float holdLength;   // 长按长度（预留）
    bool isHoldActive;  // 长按是否激活（预留）
};

// RhythmGame：节奏游戏逻辑
// 负责音符生成、下落更新、按键判定、计分连击、音效播放
class RhythmGame : public QObject
{
    Q_OBJECT
public:
    explicit RhythmGame(QObject* parent = nullptr);

    bool isActive() const { return m_active; }
    void setActive(bool active);

    int score() const { return m_score; }
    int combo() const { return m_combo; }
    const std::vector<Note>& notes() const { return m_notes; }

    void update(float dt);          // 每帧更新音符位置
    void onBeatDetected();          // 鼓点信号 → 生成音符
    void pressKey(int track);       // 按下按键判定
    void releaseKey(int track);     // 松开按键（预留）

signals:
    void scoreChanged(int score, int combo);
    void noteHit(int track, float y, int grade);    // grade: 0=Perfect, 1=Good
    void noteMissed(int track);

private:
    bool m_active = false;
    std::vector<Note> m_notes;
    int m_score = 0;
    int m_combo = 0;

    float getCurrentSpeed() const;   // 根据连击数计算当前下落速度

    static constexpr float NOTE_SPEED = 1.25f;       // 基础下落速度
    static constexpr float HIT_Y = 0.74f;            // 判定线位置
    static constexpr float HIT_WINDOW = 0.10f;       // 判定窗口
    static constexpr float PERFECT_WINDOW = 0.04f;   // Perfect 判定窗口
    static constexpr float MISS_Y = 0.95f;           // 错过线

    QSoundEffect* m_hitSound = nullptr;   // 击打音效
};

#endif // RHYTHMGAME_H