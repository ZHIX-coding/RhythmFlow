#ifndef RHYTHMFLOW_H
#define RHYTHMFLOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QTimer>
#include <QStringList>
#include <QPoint>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

class AudioEngine;
class Visualizer;
class RhythmGame;

// RhythmFlow：主窗口
// 负责模块串联、事件分发、播放器管理、GIF 录制
class RhythmFlow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RhythmFlow(QWidget* parent = nullptr);
    ~RhythmFlow() = default;

private slots:
    void onOpenFile();              // 打开本地 MP3
    void onExitApp();               // 退出程序
    void onPlaySample();            // 弹出内置歌曲菜单
    void onSampleSelected(int index);// 播放选中的内置歌曲
    void onShowHelp();              // 显示帮助对话框

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    AudioEngine* m_audioEngine = nullptr;
    Visualizer* m_visualizer = nullptr;
    RhythmGame* m_game = nullptr;

    // 内置歌曲数据
    QStringList m_sampleNames;
    QStringList m_samplePaths;

    // 窗口拖动
    QWidget* m_titleBar = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    void setupTitleBar();

    // 播放列表（仅内置歌曲）
    QStringList m_playlist;
    int m_currentPlayIndex = -1;
    int m_playMode = 0;   // 0=顺序, 1=单曲, 2=随机

    // 提示动画
    QLabel* m_hintLabel = nullptr;
    QGraphicsOpacityEffect* m_hintEffect = nullptr;
    QTimer* m_hintTimer = nullptr;
    void showHint(const QString& text);

    // 播放控制
    void playByIndex(int index);
    void prevTrack();
    void nextTrack();
    void updateModeHint();
    void toggleGameMode();

    // GIF 录制
    QTimer* m_recordTimer = nullptr;
    bool m_isRecording = false;
    void* m_gifWriter = nullptr;
    int m_recordFrameCount = 0;
    int m_recordTotalFrames = 0;
    QString m_recordFilePath;
    void startRecording();
    void stopRecording();
    void toggleRecording();
};

#endif // RHYTHMFLOW_H