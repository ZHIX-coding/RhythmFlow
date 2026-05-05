#include "RhythmGame.h"
#include <QRandomGenerator>
#include <cmath>

RhythmGame::RhythmGame(QObject* parent) : QObject(parent) {
    m_hitSound = new QSoundEffect(this);
    m_hitSound->setSource(QUrl("qrc:/RhythmFlow/resources/Pickup.wav"));
    m_hitSound->setVolume(0.5f);
}

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

void RhythmGame::onBeatDetected()
{
    if (!m_active) return;
    int track = QRandomGenerator::global()->bounded(4);
    bool isStrong = (QRandomGenerator::global()->bounded(100) < 10);
    m_notes.push_back({ track, -0.1f, false, false, isStrong, 0.0f, false });
}

void RhythmGame::update(float dt)
{
    if (!m_active) return;
    for (auto& note : m_notes) {
        note.y += NOTE_SPEED * dt;
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
        int comboBonus = qMin(m_combo / 10, 5);
        m_score += 100 + comboBonus * 20;
        if (bestNote->isStrong) m_score += 50;
        emit scoreChanged(m_score, m_combo);

        if (m_hitSound)
            m_hitSound->play();
    }
}

void RhythmGame::releaseKey(int track)
{
    Q_UNUSED(track);
}