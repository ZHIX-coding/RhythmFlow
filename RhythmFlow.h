#ifndef RHYTHMFLOW_H
#define RHYTHMFLOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QEvent>
#include <QKeyEvent>
#include "AudioEngine.h"
#include "Visualizer.h"
#include "RhythmGame.h"
#include <QSoundEffect>

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
};

#endif // RHYTHMFLOW_H