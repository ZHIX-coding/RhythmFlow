#ifndef RHYTHMGAME_H
#define RHYTHMGAME_H

#include <QObject>
#include <QSoundEffect>
#include <QUrl>
#include <vector>

struct Note {
    int track;
    float y;
    bool isHit;
    bool isMissed;
    bool isStrong;
    float holdLength;
    bool isHoldActive;
};

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

    void update(float dt);
    void onBeatDetected();
    void pressKey(int track);
    void releaseKey(int track);

signals:
    void scoreChanged(int score, int combo);
    void noteHit(int track, float y, int grade);   // grade 0=perfect, 1=good
    void noteMissed(int track);

private:
    bool m_active = false;
    std::vector<Note> m_notes;
    int m_score = 0;
    int m_combo = 0;
    float getCurrentSpeed() const;

    static constexpr float NOTE_SPEED = 1.25f;
    static constexpr float HIT_Y = 0.74f;
    static constexpr float HIT_WINDOW = 0.10f;
    static constexpr float PERFECT_WINDOW = 0.04f;
    static constexpr float MISS_Y = 0.95f;

    QSoundEffect* m_hitSound = nullptr;
};

#endif // RHYTHMGAME_H once