#include "RhythmGame.h"
#include <QRandomGenerator>
#include <cmath>

// 构造函数：初始化击打音效
RhythmGame::RhythmGame(QObject* parent) : QObject(parent) {
    m_hitSound = new QSoundEffect(this);
    m_hitSound->setSource(QUrl("qrc:/RhythmFlow/resources/Pickup.wav"));
    m_hitSound->setVolume(0.5f);
}

// 设置游戏激活状态，退出时重置数据
void RhythmGame::setActive(bool active)
{
    if (m_active == active) return;
    m_active = active;
    if (!active) {
        m_notes.clear();
        m_score = 0;
        m_combo = 0;
        emit scoreChanged(0, 0);
    }
}

// 鼓点检测回调：随机轨道生成音符，10% 概率为强拍
void RhythmGame::onBeatDetected()
{
    if (!m_active) return;
    int track = QRandomGenerator::global()->bounded(4);
    bool isStrong = (QRandomGenerator::global()->bounded(100) < 10);
    m_notes.push_back({ track, -0.1f, false, false, isStrong, 0.0f, false });
}

// 每帧更新：音符下落、错过判定、清除已处理音符
void RhythmGame::update(float dt)
{
    if (!m_active) return;
    for (auto& note : m_notes) {
        note.y += getCurrentSpeed() * dt;
        if (!note.isHit && note.y > MISS_Y) {
            emit noteMissed(note.track);
            note.isMissed = true;
            m_combo = 0;
            emit scoreChanged(m_score, m_combo);
        }
    }
    m_notes.erase(std::remove_if(m_notes.begin(), m_notes.end(),
        [](const Note& n) { return n.isHit || n.isMissed; }),
        m_notes.end());
}

// 按键判定：查找最近的未判定音符，区分 Perfect/Good
void RhythmGame::pressKey(int track)
{
    if (!m_active) return;
    Note* bestNote = nullptr;
    float bestDist = HIT_WINDOW;
    for (auto& note : m_notes) {
        if (note.track == track && !note.isHit && !note.isMissed) {
            float dist = std::abs(note.y - HIT_Y);
            if (dist < bestDist) {
                bestDist = dist;
                bestNote = &note;
            }
        }
    }
    if (bestNote) {
        bestNote->isHit = true;
        float dist = std::abs(bestNote->y - HIT_Y);
        int grade = (dist < PERFECT_WINDOW) ? 0 : 1;
        emit noteHit(bestNote->track, bestNote->y, grade);

        m_combo++;
        int comboBonus = qMin(m_combo / 10, 5);     // 每 10 连击 +1 分加成
        m_score += 100 + comboBonus * 20;
        if (bestNote->isStrong) m_score += 50;       // 强拍额外加分
        emit scoreChanged(m_score, m_combo);

        if (m_hitSound) m_hitSound->play();
    }
}

// 松开按键（预留长按功能）
void RhythmGame::releaseKey(int track)
{
    Q_UNUSED(track);
}

// 根据连击数计算当前下落速度
// 每 25 连击速度 +25%
float RhythmGame::getCurrentSpeed() const
{
    float base = NOTE_SPEED;
    int comboLevel = m_combo / 25;
    float multiplier = 1.0f + comboLevel * 0.25f;
    return base * multiplier;
}