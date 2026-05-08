#ifndef RHYTHMFLOW_H
#define RHYTHMFLOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsOpacityEffect>
#include "AudioEngine.h"
#include "Visualizer.h"
#include "RhythmGame.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

class RhythmFlow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RhythmFlow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();
    void onExitApp();
    void onPlaySample();           // 点击按钮时弹出菜单
    void onSampleSelected(int index); // 用户选择了菜单中的某首歌
    void onShowHelp();   // 新增：显示帮助

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    AudioEngine* m_audioEngine;
    Visualizer* m_visualizer;
    RhythmGame* m_game;
    void toggleGameMode();
    // 内置歌曲数据
    QStringList m_sampleNames;
    QStringList m_samplePaths;

    // 窗口拖动相关
    QWidget* m_titleBar = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    void setupTitleBar();

    // 播放列表（仅内置歌曲）
    QStringList m_playlist;
    int m_currentPlayIndex = -1;
    int m_playMode = 0;                 // 0=顺序循环, 1=单曲循环, 2=随机播放

    // 提示文字动画
    QLabel* m_hintLabel = nullptr;
    QGraphicsOpacityEffect* m_hintEffect = nullptr;
    QTimer* m_hintTimer = nullptr;
    void showHint(const QString& text);

    // 播放器控制函数
    void playByIndex(int index);
    void prevTrack();
    void nextTrack();
    void updateModeHint();

    // GIF 录制
    QTimer* m_recordTimer = nullptr;
    bool m_isRecording = false;
    void* m_gifWriter = nullptr;   // 指向 GifWriter 实例
    int m_recordFrameCount = 0;
    QString m_recordFilePath;
    int m_recordTotalFrames = 0;
    void startRecording();
    void stopRecording();
    void toggleRecording();
};

#endif // RHYTHMFLOW_H