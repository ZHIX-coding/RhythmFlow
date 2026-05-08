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

// AudioEngine：音频引擎
// 负责 MP3 播放、内部音频捕获、FFT 频谱分析、鼓点检测
class AudioEngine : public QObject
{
    Q_OBJECT
public:
    static constexpr int BAR_COUNT = 64;   // 频谱频段数量

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    // 播放本地文件（QString 路径）
    void playFile(const QString& filePath);
    // 播放资源文件（QUrl 路径，用于 qrc 内置歌曲）
    void playFile(const QUrl& url);
    // 获取当前频谱数据（只读）
    const std::vector<float>& spectrum() const { return m_spectrum; }

    // 播放控制
    void pause();
    void resume();
    void togglePause();
    bool isPaused() const;

    // 获取内部播放器指针（用于连接信号）
    QMediaPlayer* player() const { return m_player; }

signals:
    void spectrumUpdated();   // 频谱更新信号（触发重绘）
    void beatDetected();      // 鼓点检测信号（触发游戏音符生成）

private slots:
    // 处理音频缓冲区数据（QAudioBufferOutput 回调）
    void processAudioBuffer(const QAudioBuffer& buffer);

private:
    QMediaPlayer* m_player = nullptr;             // 媒体播放器
    QAudioOutput* m_audioOutput = nullptr;         // 音频输出控制
    QAudioBufferOutput* m_bufferOutput = nullptr;  // 内部音频捕获器
    kiss_fft_cfg m_fftCfg = nullptr;              // FFT 配置句柄
    std::vector<float> m_spectrum;                 // 64 频段频谱能量数组

    // 鼓点检测
    std::vector<float> m_prevSpectrum;   // 上一帧频谱（用于计算通量）
    int m_beatCooldown = 0;              // 鼓点冷却计数器
};

#endif // AUDIOENGINE_H