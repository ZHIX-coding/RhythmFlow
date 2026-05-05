#include "AudioEngine.h"
#include <QUrl>
#include <cmath>
#include <QDebug>

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
    m_fftCfg = kiss_fft_alloc(1024, 0, nullptr, nullptr);
    m_spectrum.resize(BAR_COUNT, 0.0f);

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.5f);

    m_bufferOutput = new QAudioBufferOutput(this);
    m_player->setAudioBufferOutput(m_bufferOutput);
    connect(m_bufferOutput, &QAudioBufferOutput::audioBufferReceived,
        this, &AudioEngine::processAudioBuffer);
    m_spectrum.resize(BAR_COUNT, 0.0f);
    m_prevSpectrum.resize(BAR_COUNT, 0.0f);
}

AudioEngine::~AudioEngine()
{
    if (m_fftCfg)
        free(m_fftCfg);
}

void AudioEngine::playFile(const QString& filePath)
{
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_player->play();
}

void AudioEngine::processAudioBuffer(const QAudioBuffer& buffer)
{
    const qint16* data = buffer.constData<qint16>();
    int sampleCount = buffer.sampleCount();
    int channelCount = buffer.format().channelCount();

    if (sampleCount == 0 || channelCount == 0)
        return;

    const int fftSize = 1024;
    std::vector<kiss_fft_cpx> in(fftSize), out(fftSize);

    for (int i = 0; i < fftSize && i * channelCount < sampleCount; ++i) {
        in[i].r = data[i * channelCount] / 32768.0f;
        in[i].i = 0.0f;
    }
    for (int i = sampleCount / channelCount; i < fftSize; ++i) {
        in[i].r = 0.0f;
        in[i].i = 0.0f;
    }

    kiss_fft(m_fftCfg, in.data(), out.data());

    for (int band = 0; band < BAR_COUNT; ++band) {
        int start = band * (fftSize / 2) / BAR_COUNT;
        int end = (band + 1) * (fftSize / 2) / BAR_COUNT;
        float sum = 0.0f;
        for (int i = start; i < end; ++i) {
            float mag = sqrtf(out[i].r * out[i].r + out[i].i * out[i].i);
            sum += mag;
        }
        float avg = sum / (end - start);
        float db = 20.0f * log10f(avg + 1e-6f);
        m_spectrum[band] = qMax(db + 80.0f, 0.0f);  // 调整过灵敏度
    }
    // 鼓点检测（频谱通量法）
    float flux = 0.0f;
    for (int i = 0; i < BAR_COUNT; ++i) {
        float diff = m_spectrum[i] - m_prevSpectrum[i];
        if (diff > 0) flux += diff;
    }

    // 更稳定的动态阈值：更新速度放慢
    static float avgFlux = 0.0f;
    avgFlux = avgFlux * 0.97f + flux * 0.03f;  // 原本 0.9/0.1，现在平滑很多

    // 动态阈值倍数：1.2 倍历史平均，同时要求 flux 不能太小（> 0.5）
    if (m_beatCooldown <= 0 && flux > avgFlux * 1.25f && flux > 0.8f) {
        emit beatDetected();
        m_beatCooldown = 8;  // 冷却约 150ms（假设每帧 25ms 左右）
    }
    if (m_beatCooldown > 0) m_beatCooldown--;

    // 保存当前频谱供下一帧比较
    m_prevSpectrum = m_spectrum;

    emit spectrumUpdated();
}

void AudioEngine::playFile(const QUrl& url)
{
    m_player->setSource(url);
    m_player->play();
}