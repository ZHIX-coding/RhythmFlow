#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioBufferOutput>
#include <QAudioBuffer>
#include <vector>

extern "C" {
#include "kiss_fft.h"
}

class AudioEngine : public QObject
{
    Q_OBJECT
public:
    static constexpr int BAR_COUNT = 64;

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    void playFile(const QString& filePath);
    const std::vector<float>& spectrum() const { return m_spectrum; }
    void playFile(const QUrl& url);
    // 新增播放控制
    void pause();
    void resume();
    void togglePause();
    bool isPaused() const;
    QMediaPlayer* player() const { return m_player; }

signals:
    void spectrumUpdated();   // 频谱更新
    void beatDetected();      // 检测到鼓点

private slots:
    void processAudioBuffer(const QAudioBuffer& buffer);

private:
    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QAudioBufferOutput* m_bufferOutput = nullptr;
    kiss_fft_cfg m_fftCfg = nullptr;
    std::vector<float> m_spectrum;

    // 鼓点检测相关
    std::vector<float> m_prevSpectrum;
    int m_beatCooldown = 0;
};

#endif // AUDIOENGINE_H