#include "RhythmFlow.h"
#include "AudioEngine.h"
#include "Visualizer.h"
#include "RhythmGame.h"
#include "gif.h"

#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QApplication>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QPropertyAnimation>

// 构造函数：创建所有子模块、连接信号、初始化 UI 和参数
RhythmFlow::RhythmFlow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("RhythmFlow v1.0 - Framework");
    resize(800, 600);

    m_visualizer = new Visualizer(this);
    setCentralWidget(m_visualizer);

    m_audioEngine = new AudioEngine(this);
    connect(m_audioEngine, &AudioEngine::spectrumUpdated, this, [this]() {
        m_visualizer->setSpectrum(m_audioEngine->spectrum());
        });

    // "MP3" 按钮
    QPushButton* btnOpen = new QPushButton("MP3", m_visualizer);
    btnOpen->setFocusPolicy(Qt::NoFocus);
    btnOpen->setGeometry(20, 20, 80, 30);
    btnOpen->setStyleSheet(R"(
        QPushButton { background-color: rgba(0,60,80,200); color:#00ffff; border:1.5px solid #00cccc;
                      border-radius:8px; font-size:14px; font-weight:bold; }
        QPushButton:hover { background-color: rgba(0,180,180,220); border:1.5px solid #00ffff; }
        QPushButton:pressed { background-color: rgba(0,40,60,220); }
    )");
    connect(btnOpen, &QPushButton::clicked, this, &RhythmFlow::onOpenFile);

    // "Play" 按钮
    QPushButton* btnSample = new QPushButton("Play", m_visualizer);
    btnSample->setFocusPolicy(Qt::NoFocus);
    btnSample->setGeometry(20, 60, 80, 30);
    btnSample->setStyleSheet(R"(
        QPushButton { background-color: rgba(0,60,80,200); color:#00ffff; border:1.5px solid #00cccc;
                      border-radius:8px; font-size:14px; font-weight:bold; }
        QPushButton:hover { background-color: rgba(0,180,180,220); border:1.5px solid #00ffff; }
        QPushButton:pressed { background-color: rgba(0,40,60,220); }
    )");
    connect(btnSample, &QPushButton::clicked, this, &RhythmFlow::onPlaySample);

    btnOpen->raise(); btnSample->raise();

    setupTitleBar();

    // 无边框 + 透明背景 + 外发光
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    QGraphicsDropShadowEffect* glowEffect = new QGraphicsDropShadowEffect(this);
    glowEffect->setBlurRadius(40);
    glowEffect->setOffset(0, 0);
    glowEffect->setColor(QColor(0, 255, 255, 180));
    m_visualizer->setGraphicsEffect(glowEffect);

    // 退出提示
    QLabel* hintLabel = new QLabel("exit", m_visualizer);
    hintLabel->setStyleSheet("color: rgba(200,200,200,100); font-size:12px; background:transparent;");
    hintLabel->setGeometry(20, height() - 30, 60, 20);
    hintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    // 动态提示标签
    m_hintLabel = new QLabel(m_visualizer);
    m_hintLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_hintLabel->setStyleSheet("color: rgba(200,200,200,180); font-size:14px; background:transparent; padding:4px;");
    m_hintLabel->setGeometry(20, height() - 55, 300, 25);
    m_hintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hintLabel->hide();

    m_hintEffect = new QGraphicsOpacityEffect(m_hintLabel);
    m_hintLabel->setGraphicsEffect(m_hintEffect);
    m_hintEffect->setOpacity(0.0);

    m_hintTimer = new QTimer(this);
    m_hintTimer->setSingleShot(true);
    connect(m_hintTimer, &QTimer::timeout, this, [this]() {
        QPropertyAnimation* fadeOut = new QPropertyAnimation(m_hintEffect, "opacity", this);
        fadeOut->setDuration(500);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        connect(fadeOut, &QPropertyAnimation::finished, m_hintLabel, &QLabel::hide);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        });

    // 游戏对象
    m_game = new RhythmGame(this);
    m_visualizer->setGame(m_game);
    connect(m_game, &RhythmGame::noteHit, m_visualizer, &Visualizer::onNoteHit);
    connect(m_game, &RhythmGame::noteMissed, m_visualizer, &Visualizer::onNoteMissed);
    connect(m_game, &RhythmGame::scoreChanged, this, [this](int, int combo) {
        m_visualizer->onComboChanged(combo);
        });
    connect(m_audioEngine, &AudioEngine::beatDetected, m_game, &RhythmGame::onBeatDetected);

    installEventFilter(this);

    // 游戏更新定时器
    QTimer* gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, [this]() {
        if (m_game->isActive()) { m_game->update(0.016f); m_visualizer->update(); }
        });
    gameTimer->start(16);

    // 内置歌曲列表
    m_sampleNames << "Alex LeMirage" << "Dionela _ Jay R"
        << "晴天" << "稻香" << "always online" << "不潮不用花钱"
        << "心墙" << "黑夜问白天" << "唯一" << "泡沫" << "倒数"
        << "beauty and a beat" << "As Long As You Love Me" << "野人";
    m_samplePaths << "qrc:/RhythmFlow/resources/singing1.mp3"
        << "qrc:/RhythmFlow/resources/singing2.mp3"
        << "qrc:/RhythmFlow/resources/singing3.mp3"
        << "qrc:/RhythmFlow/resources/singing4.mp3"
        << "qrc:/RhythmFlow/resources/singing5.mp3"
        << "qrc:/RhythmFlow/resources/singing6.mp3"
        << "qrc:/RhythmFlow/resources/singing7.mp3"
        << "qrc:/RhythmFlow/resources/singing8.mp3"
        << "qrc:/RhythmFlow/resources/singing9.mp3"
        << "qrc:/RhythmFlow/resources/singing10.mp3"
        << "qrc:/RhythmFlow/resources/singing11.mp3"
        << "qrc:/RhythmFlow/resources/singing12.mp3"
        << "qrc:/RhythmFlow/resources/singing13.mp3"
        << "qrc:/RhythmFlow/resources/singing14.mp3";
    m_playlist = m_samplePaths;
    m_currentPlayIndex = -1;

    // GIF 录制定时器
    m_recordTimer = new QTimer(this);
    m_recordTimer->setInterval(83);
    connect(m_recordTimer, &QTimer::timeout, this, [this]() {
        if (!m_isRecording || !m_gifWriter) return;
        GifWriter* w = static_cast<GifWriter*>(m_gifWriter);
        QPixmap pix = m_visualizer->grab().scaled(480, 360, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QImage img = pix.toImage().convertToFormat(QImage::Format_RGBA8888);
        GifWriteFrame(w, img.constBits(), 480, 360, 10);
        m_recordFrameCount++;
        if (m_recordFrameCount >= m_recordTotalFrames) stopRecording();
        });
}

// 右键菜单：Help / Exit
void RhythmFlow::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu { background-color: rgba(10,30,40,240); color:#00ffff; border:1.5px solid #00aaaa; border-radius:6px; }
        QMenu::item { padding:8px 24px; }
        QMenu::item:selected { background-color: rgba(0,180,180,150); color:#ffffff; }
    )");
    QAction* helpAction = menu.addAction("Help");
    connect(helpAction, &QAction::triggered, this, &RhythmFlow::onShowHelp);
    QAction* exitAction = menu.addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &RhythmFlow::onExitApp);
    menu.exec(event->globalPos());
}

// 退出程序
void RhythmFlow::onExitApp() { QApplication::quit(); }

// 打开本地 MP3 文件
void RhythmFlow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select MP3 File", "", "MP3 Files (*.mp3)");
    if (!fileName.isEmpty()) {
        disconnect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this, nullptr);
        m_audioEngine->playFile(fileName);
        m_currentPlayIndex = -1;
    }
}

// 全局按键事件过滤器
bool RhythmFlow::eventFilter(QObject* obj, QEvent* event)
{
    // 标题栏拖动
    if (obj == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragStartPos = me->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove && m_dragging) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            move(me->globalPosition().toPoint() - m_dragStartPos);
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease && m_dragging) {
            m_dragging = false;
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->isAutoRepeat()) return true;

        if (keyEvent->key() == Qt::Key_F1) { onShowHelp(); return true; }

        switch (keyEvent->key()) {
        case Qt::Key_1: prevTrack(); showHint("Previous"); return true;
        case Qt::Key_2: m_audioEngine->togglePause(); showHint(m_audioEngine->isPaused() ? "Pause" : "Play"); return true;
        case Qt::Key_3: nextTrack(); showHint("Next"); return true;
        case Qt::Key_4:
            m_playMode = (m_playMode + 1) % 3;
            if (m_playMode == 0) showHint("All");
            else if (m_playMode == 1) showHint("One");
            else showHint("Shuffle");
            return true;
        case Qt::Key_R: toggleRecording(); return true;
        }

        if (keyEvent->key() == Qt::Key_Space) { toggleGameMode(); return true; }

        if (m_game->isActive()) {
            switch (keyEvent->key()) {
            case Qt::Key_D: m_game->pressKey(0); return true;
            case Qt::Key_F: m_game->pressKey(1); return true;
            case Qt::Key_J: m_game->pressKey(2); return true;
            case Qt::Key_K: m_game->pressKey(3); return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// 弹出内置歌曲菜单
void RhythmFlow::onPlaySample()
{
    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu { background-color: rgba(10,30,40,240); color:#00ffff; border:1.5px solid #00aaaa; border-radius:6px; }
        QMenu::item { padding:8px 24px; }
        QMenu::item:selected { background-color: rgba(0,180,180,150); color:#ffffff; }
    )");
    for (int i = 0; i < m_sampleNames.size(); ++i) {
        QAction* action = menu.addAction(m_sampleNames[i]);
        connect(action, &QAction::triggered, this, [this, i]() { onSampleSelected(i); });
    }
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) menu.exec(btn->mapToGlobal(QPoint(0, btn->height())));
}

// 播放选中的内置歌曲
void RhythmFlow::onSampleSelected(int index)
{
    if (index < 0 || index >= m_samplePaths.size()) return;
    disconnect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this, nullptr);
    m_currentPlayIndex = index;
    m_audioEngine->playFile(QUrl(m_samplePaths[index]));
    connect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this,
        [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia && m_currentPlayIndex >= 0) nextTrack();
        });
}

// 切换游戏模式
void RhythmFlow::toggleGameMode() { m_game->setActive(!m_game->isActive()); }

// 创建顶部透明拖动栏
void RhythmFlow::setupTitleBar()
{
    m_titleBar = new QWidget(m_visualizer);
    m_titleBar->setGeometry(0, 0, width(), 20);
    m_titleBar->setCursor(Qt::SizeAllCursor);
    m_titleBar->setStyleSheet("background: transparent;");
    m_titleBar->installEventFilter(this);
    m_titleBar->show();
}

// 显示动态提示文字（渐显 → 停留 → 渐隐）
void RhythmFlow::showHint(const QString& text)
{
    m_hintLabel->setText(text);
    m_hintLabel->show();
    m_hintTimer->stop();

    QPropertyAnimation* fadeIn = new QPropertyAnimation(m_hintEffect, "opacity", this);
    fadeIn->setDuration(300);
    fadeIn->setStartValue(m_hintEffect->opacity());
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    m_hintTimer->start(1500);
}

// 按索引播放内置播放列表中的歌曲
void RhythmFlow::playByIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) return;
    m_currentPlayIndex = index;
    disconnect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this, nullptr);
    if (m_playlist[index].startsWith("qrc:"))
        m_audioEngine->playFile(QUrl(m_playlist[index]));
    else
        m_audioEngine->playFile(m_playlist[index]);
    connect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this,
        [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia && m_currentPlayIndex >= 0) nextTrack();
        });
}

// 播放上一首
void RhythmFlow::prevTrack()
{
    if (m_currentPlayIndex < 0 || m_playlist.isEmpty()) return;
    int newIndex = m_currentPlayIndex - 1;
    if (newIndex < 0) newIndex = m_playlist.size() - 1;
    playByIndex(newIndex);
}

// 播放下一首（根据播放模式）
void RhythmFlow::nextTrack()
{
    if (m_currentPlayIndex < 0 || m_playlist.isEmpty()) return;
    int newIndex;
    if (m_playMode == 2) {
        newIndex = QRandomGenerator::global()->bounded(m_playlist.size());
    }
    else {
        newIndex = m_currentPlayIndex + 1;
        if (newIndex >= m_playlist.size())
            newIndex = (m_playMode == 0) ? 0 : m_currentPlayIndex;
    }
    playByIndex(newIndex);
}

// 显示播放模式提示
void RhythmFlow::updateModeHint()
{
    if (m_playMode == 0) showHint("All");
    else if (m_playMode == 1) showHint("One");
    else showHint("Shuffle");
}

// 显示帮助对话框（F1 / 右键菜单）
void RhythmFlow::onShowHelp()
{
    QMessageBox::information(this, "RhythmFlow Help",
        "操作说明：\n\n"
        "【音乐播放】\n  1  上一首\n  2  播放 / 暂停\n  3  下一首\n  4  切换播放模式\n\n"
        "【可视化】\n  鼠标左键点击：粒子聚拢 + 涟漪\n  拖动顶部区域：移动窗口\n  右键：菜单\n\n"
        "【游戏模式】\n  空格键：进入/退出游戏\n  D/F/J/K：击打音符\n  连击越高，下落越快\n\n"
        "【其他】\n  F1：帮助\n  R：录制 GIF");
}

// 开始 GIF 录制
void RhythmFlow::startRecording()
{
    if (m_isRecording) return;
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    m_recordFilePath = desktop + "/RhythmFlow_" + timestamp + ".gif";

    m_gifWriter = new GifWriter;
    GifWriter* w = static_cast<GifWriter*>(m_gifWriter);
    if (!GifBegin(w, m_recordFilePath.toUtf8().constData(), 480, 360, 10)) {
        delete w; m_gifWriter = nullptr; showHint("GIF Error"); return;
    }
    m_recordFrameCount = 0;
    m_recordTotalFrames = 60;
    m_isRecording = true;
    m_recordTimer->start();
    showHint("Recording...");
}

// 停止 GIF 录制
void RhythmFlow::stopRecording()
{
    if (!m_isRecording || !m_gifWriter) return;
    m_recordTimer->stop();
    GifWriter* w = static_cast<GifWriter*>(m_gifWriter);
    GifEnd(w);
    delete w; m_gifWriter = nullptr;
    m_isRecording = false;
    showHint("GIF saved");
}

// 切换录制状态
void RhythmFlow::toggleRecording()
{
    if (m_isRecording) stopRecording(); else startRecording();
}