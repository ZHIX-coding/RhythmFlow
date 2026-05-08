#include "AudioEngine.h"
#include <QUrl>
#include <cmath>
#include <QDebug>

// 构造函数：初始化 FFT、播放器、音频捕获器
AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
    m_fftCfg = kiss_fft_alloc(1024, 0, nullptr, nullptr);   // 1024 点 FFT
    m_spectrum.resize(BAR_COUNT, 0.0f);

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.5f);

    m_bufferOutput = new QAudioBufferOutput(this);
    m_player->setAudioBufferOutput(m_bufferOutput);
    connect(m_bufferOutput, &QAudioBufferOutput::audioBufferReceived,
        this, &AudioEngine::processAudioBuffer);

    m_prevSpectrum.resize(BAR_COUNT, 0.0f);
}

// 析构函数：释放 FFT 资源
AudioEngine::~AudioEngine()
{
    if (m_fftCfg) free(m_fftCfg);
}

// 播放本地文件
void AudioEngine::playFile(const QString& filePath)
{
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_player->play();
}

// 播放资源文件（qrc 路径）
void AudioEngine::playFile(const QUrl& url)
{
    m_player->setSource(url);
    m_player->play();
}

// 暂停播放
void AudioEngine::pause()
{
    m_player->pause();
}

// 恢复播放
void AudioEngine::resume()
{
    m_player->play();
}

// 切换暂停/播放状态
void AudioEngine::togglePause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

// 查询是否处于暂停状态
bool AudioEngine::isPaused() const
{
    return m_player->playbackState() == QMediaPlayer::PausedState;
}

// 处理音频缓冲区（核心信号处理）
// 1. 提取 PCM 数据
// 2. 执行 1024 点 FFT
// 3. 计算 64 频段对数能量
// 4. 频谱通量法检测鼓点
void AudioEngine::processAudioBuffer(const QAudioBuffer& buffer)
{
    const qint16* data = buffer.constData<qint16>();
    int sampleCount = buffer.sampleCount();
    int channelCount = buffer.format().channelCount();

    if (sampleCount == 0 || channelCount == 0) return;

    // ---------- FFT ----------
    const int fftSize = 1024;
    std::vector<kiss_fft_cpx> in(fftSize), out(fftSize);

    // 取第一个声道，归一化到 [-1, 1]
    for (int i = 0; i < fftSize && i * channelCount < sampleCount; ++i) {
        in[i].r = data[i * channelCount] / 32768.0f;
        in[i].i = 0.0f;
    }
    for (int i = sampleCount / channelCount; i < fftSize; ++i) {
        in[i].r = 0.0f;
        in[i].i = 0.0f;
    }
    kiss_fft(m_fftCfg, in.data(), out.data());

    // ---------- 64 频段能量 ----------
    for (int band = 0; band < BAR_COUNT; ++band) {
        int start = band * (fftSize / 2) / BAR_COUNT;
        int end = (band + 1) * (fftSize / 2) / BAR_COUNT;
        float sum = 0.0f;
        for (int i = start; i < end; ++i) {
            float mag = sqrtf(out[i].r * out[i].r + out[i].i * out[i].i);
            sum += mag;
        }
        float avg = sum / (end - start);
        float db = 20.0f * log10f(avg + 1e-6f);                  // 转分贝
        m_spectrum[band] = qMax(db + 80.0f, 0.0f);              // 动态范围调整
    }

    // ---------- 鼓点检测（频谱通量法）----------
    float flux = 0.0f;
    for (int i = 0; i < BAR_COUNT; ++i) {
        float diff = m_spectrum[i] - m_prevSpectrum[i];
        if (diff > 0) flux += diff;
    }

    static float avgFlux = 0.0f;
    avgFlux = avgFlux * 0.97f + flux * 0.03f;   // 平滑平均通量

    if (m_beatCooldown <= 0 && flux > avgFlux * 1.25f && flux > 0.8f) {
        emit beatDetected();
        m_beatCooldown = 8;   // 冷却约 200ms
    }
    if (m_beatCooldown > 0) m_beatCooldown--;

    m_prevSpectrum = m_spectrum;

    emit spectrumUpdated();
}