#include "RhythmFlow.h"
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QLabel>
#include <QApplication>
#include <QKeyEvent>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QRandomGenerator>

RhythmFlow::RhythmFlow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("RhythmFlow v1.0 - Framework");
    resize(800, 600);

    // 创建中心控件（Visualizer）
    m_visualizer = new Visualizer(this);
    setCentralWidget(m_visualizer);

    // 创建音频引擎
    m_audioEngine = new AudioEngine(this);
    connect(m_audioEngine, &AudioEngine::spectrumUpdated, this, [this]() {
        m_visualizer->setSpectrum(m_audioEngine->spectrum());
        });

    // 创建悬浮按钮（简单布局，后续可美化）
    QPushButton* btnOpen = new QPushButton("MP3", m_visualizer);
    btnOpen->setFocusPolicy(Qt::NoFocus);   // 按钮永远不获取键盘焦点
    btnOpen->setGeometry(20, 20, 80, 30);
    btnOpen->setStyleSheet(R"(
    QPushButton {
        background-color: rgba(0, 60, 80, 200);
        color: #00ffff;
        border: 1.5px solid #00cccc;
        border-radius: 8px;
        font-size: 14px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: rgba(0, 180, 180, 220);
        border: 1.5px solid #00ffff;
    }
    QPushButton:pressed {
        background-color: rgba(0, 40, 60, 220);
    }
)");
    connect(btnOpen, &QPushButton::clicked, this, &RhythmFlow::onOpenFile);

    // “Play Sample”按钮（弹出菜单选择内置歌曲）
    QPushButton* btnSample = new QPushButton("Play", m_visualizer);
    btnSample->setFocusPolicy(Qt::NoFocus);
    btnSample->setGeometry(20, 60, 80, 30);   // y=60，位于 Open 按钮下方
    btnSample->setStyleSheet(R"(
    QPushButton {
        background-color: rgba(0, 60, 80, 200);
        color: #00ffff;
        border: 1.5px solid #00cccc;
        border-radius: 8px;
        font-size: 14px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: rgba(0, 180, 180, 220);
        border: 1.5px solid #00ffff;
    }
    QPushButton:pressed {
        background-color: rgba(0, 40, 60, 220);
    }
)");
    connect(btnSample, &QPushButton::clicked, this, &RhythmFlow::onPlaySample);

    // 确保按钮位于最上层（标题栏之上）
    btnOpen->raise();
    btnSample->raise();

    // 设置顶部透明标题栏用于拖动窗口
    setupTitleBar();

    //无边框窗口+启用透明背景+外发光特效
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    QGraphicsDropShadowEffect* glowEffect = new QGraphicsDropShadowEffect(this);
    glowEffect->setBlurRadius(40);          // 模糊半径，控制发光范围
    glowEffect->setOffset(0, 0);            // 偏移为0，产生中心发光
    glowEffect->setColor(QColor(0, 255, 255, 180)); // 发光颜色
    m_visualizer->setGraphicsEffect(glowEffect);

    QLabel* hintLabel = new QLabel("exit", m_visualizer);
    hintLabel->setStyleSheet("color: rgba(200,200,200,100); font-size: 12px; background: transparent;");
    hintLabel->setGeometry(20, height() - 30, 60, 20);
    hintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);  // 让鼠标事件穿透，不影响右键

    // 动态提示标签（带渐变动画）
    m_hintLabel = new QLabel(m_visualizer);
    m_hintLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_hintLabel->setStyleSheet("color: rgba(200,200,200,180); font-size: 14px; background: transparent; padding: 4px;");
    m_hintLabel->setGeometry(20, height() - 55, 300, 25);
    m_hintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hintLabel->hide(); // 默认隐藏

    m_hintEffect = new QGraphicsOpacityEffect(m_hintLabel);
    m_hintLabel->setGraphicsEffect(m_hintEffect);
    m_hintEffect->setOpacity(0.0);

    m_hintTimer = new QTimer(this);
    m_hintTimer->setSingleShot(true);
    connect(m_hintTimer, &QTimer::timeout, this, [this]() {
        // 渐隐动画
        QPropertyAnimation* fadeOut = new QPropertyAnimation(m_hintEffect, "opacity", this);
        fadeOut->setDuration(500);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        connect(fadeOut, &QPropertyAnimation::finished, m_hintLabel, &QLabel::hide);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        });

    // 创建游戏对象
    m_game = new RhythmGame(this);
    m_visualizer->setGame(m_game);
    connect(m_game, &RhythmGame::noteHit, m_visualizer, &Visualizer::onNoteHit);
    connect(m_game, &RhythmGame::noteMissed, m_visualizer, &Visualizer::onNoteMissed);
    connect(m_game, &RhythmGame::scoreChanged, this, [this](int score, int combo) {
        m_visualizer->onComboChanged(combo);
        });

    // 鼓点检测 → 生成音符
    connect(m_audioEngine, &AudioEngine::beatDetected, m_game, &RhythmGame::onBeatDetected);

    // 分数变化更新（可用于外部显示）
    // connect(m_game, &RhythmGame::scoreChanged, ...);

    // 安装事件过滤器（用于全局按键）
    installEventFilter(this);

    // 游戏更新定时器
    QTimer* gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, [this]() {
        if (m_game->isActive()) {
            m_game->update(0.016f);  // 约 60fps
            m_visualizer->update();
        }
        });
    gameTimer->start(16);

    // 内置歌曲列表（名称和 qrc 路径一一对应）
    m_sampleNames << "Alex LeMirage - SOLANA_L"
        << "Dionela _ Jay R - sining_L";
    m_samplePaths << "qrc:/RhythmFlow/resources/singing1.mp3"
        << "qrc:/RhythmFlow/resources/singing2.mp3";

    // 用内置歌曲初始化播放列表（仅一次）
    m_playlist = m_samplePaths;
    m_currentPlayIndex = -1;
}

void RhythmFlow::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.setStyleSheet(R"(
    QMenu {
        background-color: rgba(10, 30, 40, 240);
        color: #00ffff;
        border: 1.5px solid #00aaaa;
        border-radius: 6px;
    }
    QMenu::item {
        padding: 8px 24px;
    }
    QMenu::item:selected {
        background-color: rgba(0, 180, 180, 150);
        color: #ffffff;
    }
)");

    QAction* exitAction = menu.addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &RhythmFlow::onExitApp);

    menu.exec(event->globalPos());
}

void RhythmFlow::onExitApp()
{
    QApplication::quit();
}

void RhythmFlow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Select MP3 File", "", "MP3 Files (*.mp3)");
    if (!fileName.isEmpty()) {
        // 断开旧自动切歌（如果有）
        disconnect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this, nullptr);
        m_audioEngine->playFile(fileName);
        m_currentPlayIndex = -1;   // 标记为不在内置列表中
    }
}

bool RhythmFlow::eventFilter(QObject* obj, QEvent* event)
{
    // ===== 新增：透明标题栏拖动 =====
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

        // ===== 播放器控制快捷键 =====
        switch (keyEvent->key()) {
        case Qt::Key_1: prevTrack(); showHint("Previous"); return true;
        case Qt::Key_2:
            m_audioEngine->togglePause();
            showHint(m_audioEngine->isPaused() ? "Pause" : "Play");
            return true;
        case Qt::Key_3: nextTrack(); showHint("Next"); return true;
        case Qt::Key_4:
            m_playMode = (m_playMode + 1) % 3;
            if (m_playMode == 0) showHint("All");
            else if (m_playMode == 1) showHint("One");
            else showHint("Shuffle");
            return true;
        default: break;
        }

        // 空格切换游戏模式
        if (keyEvent->key() == Qt::Key_Space) {
            toggleGameMode();
            return true;
        }

        // 游戏按键
        if (m_game->isActive()) {
            switch (keyEvent->key()) {
            case Qt::Key_D: m_game->pressKey(0); return true;
            case Qt::Key_F: m_game->pressKey(1); return true;
            case Qt::Key_J: m_game->pressKey(2); return true;
            case Qt::Key_K: m_game->pressKey(3); return true;
            default: break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void RhythmFlow::onPlaySample()
{
    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu {
            background-color: rgba(10, 30, 40, 240);
            color: #00ffff;
            border: 1.5px solid #00aaaa;
            border-radius: 6px;
        }
        QMenu::item {
            padding: 8px 24px;
        }
        QMenu::item:selected {
            background-color: rgba(0, 180, 180, 150);
            color: #ffffff;
        }
    )");

    // 动态生成菜单项
    for (int i = 0; i < m_sampleNames.size(); ++i) {
        QAction* action = menu.addAction(m_sampleNames[i]);
        connect(action, &QAction::triggered, this, [this, i]() {
            onSampleSelected(i);
            });
    }

    // 菜单显示在按钮正下方
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QPoint pos = btn->mapToGlobal(QPoint(0, btn->height()));
        menu.exec(pos);
    }
}

void RhythmFlow::onSampleSelected(int index)
{
    if (index >= 0 && index < m_samplePaths.size()) {
        // 断开旧连接，防止重复触发
        disconnect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this, nullptr);

        m_currentPlayIndex = index;   // 设置当前播放索引
        QString path = m_samplePaths[index];
        m_audioEngine->playFile(QUrl(path));

        // 重新连接自动切歌（仅在内置列表播放时生效）
        connect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia && m_currentPlayIndex >= 0) {
                    nextTrack();
                }
            });
    }
}

void RhythmFlow::toggleGameMode()
{
    m_game->setActive(!m_game->isActive());
    if (!m_game->isActive()) {
        // 退出游戏，恢复纯享模式
    }
}
void RhythmFlow::setupTitleBar()
{
    m_titleBar = new QWidget(m_visualizer);
    m_titleBar->setGeometry(0, 0, width(), 20);          // 顶部20像素高
    m_titleBar->setCursor(Qt::SizeAllCursor);             // 提示可拖动
    m_titleBar->setStyleSheet("background: transparent;");// 完全透明
    m_titleBar->installEventFilter(this);                 // 由主窗口的事件过滤器处理
    m_titleBar->show();
}
void RhythmFlow::showHint(const QString& text)
{
    m_hintLabel->setText(text);
    m_hintLabel->show();
    m_hintTimer->stop(); // 取消之前的定时器

    // 渐显动画
    QPropertyAnimation* fadeIn = new QPropertyAnimation(m_hintEffect, "opacity", this);
    fadeIn->setDuration(300);
    fadeIn->setStartValue(m_hintEffect->opacity());
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    // 停留1.5秒后开始渐隐
    m_hintTimer->start(1500);
}
void RhythmFlow::playByIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) return;
    m_currentPlayIndex = index;
    QString path = m_playlist[index];

    // 断开旧连接
    disconnect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this, nullptr);

    if (path.startsWith("qrc:"))
        m_audioEngine->playFile(QUrl(path));
    else
        m_audioEngine->playFile(path);

    // 重新连接自动切歌
    connect(m_audioEngine->player(), &QMediaPlayer::mediaStatusChanged, this,
        [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia && m_currentPlayIndex >= 0) {
                nextTrack();
            }
        });
}

void RhythmFlow::prevTrack()
{
    if (m_currentPlayIndex < 0 || m_playlist.isEmpty()) return;
    int newIndex = m_currentPlayIndex - 1;
    if (newIndex < 0) newIndex = m_playlist.size() - 1;
    playByIndex(newIndex);
}

void RhythmFlow::nextTrack()
{
    if (m_currentPlayIndex < 0 || m_playlist.isEmpty()) return;
    int newIndex;
    if (m_playMode == 2) {
        // 随机播放
        newIndex = QRandomGenerator::global()->bounded(m_playlist.size());
    }
    else {
        newIndex = m_currentPlayIndex + 1;
        if (newIndex >= m_playlist.size()) {
            newIndex = (m_playMode == 0) ? 0 : m_currentPlayIndex; // 顺序循环或单曲循环
        }
    }
    playByIndex(newIndex);
}

void RhythmFlow::updateModeHint()
{
    if (m_playMode == 0) showHint("All");
    else if (m_playMode == 1) showHint("One");
    else showHint("Shuffle");;
}