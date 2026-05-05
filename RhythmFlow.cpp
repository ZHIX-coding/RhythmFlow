#include "RhythmFlow.h"
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QLabel>
#include <QApplication>
#include <QKeyEvent>
#include <QTimer>

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
    QPushButton* btnOpen = new QPushButton("Open MP3 File", m_visualizer);
    btnOpen->setFocusPolicy(Qt::NoFocus);   // 按钮永远不获取键盘焦点
    btnOpen->setGeometry(20, 20, 120, 30);
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
    QPushButton* btnSample = new QPushButton("Play Sample", m_visualizer);
    btnSample->setFocusPolicy(Qt::NoFocus);
    btnSample->setGeometry(20, 60, 120, 30);   // y=60，位于 Open 按钮下方
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

    //无边框窗口+启用透明背景+外发光特效
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    QGraphicsDropShadowEffect* glowEffect = new QGraphicsDropShadowEffect(this);
    glowEffect->setBlurRadius(40);          // 模糊半径，控制发光范围
    glowEffect->setOffset(0, 0);            // 偏移为0，产生中心发光
    glowEffect->setColor(QColor(0, 255, 255, 180)); // 发光颜色
    m_visualizer->setGraphicsEffect(glowEffect);

    QLabel* hintLabel = new QLabel("Right-click to exit", m_visualizer);
    hintLabel->setStyleSheet("color: rgba(200,200,200,100); font-size: 12px; background: transparent;");
    hintLabel->setGeometry(20, height() - 30, 150, 20);
    hintLabel->setAttribute(Qt::WA_TransparentForMouseEvents);  // 让鼠标事件穿透，不影响右键

    // 创建游戏对象
    m_game = new RhythmGame(this);
    m_visualizer->setGame(m_game);
    connect(m_game, &RhythmGame::noteHit, m_visualizer, &Visualizer::onNoteHit);
    connect(m_game, &RhythmGame::noteMissed, m_visualizer, &Visualizer::onNoteMissed);

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

    QAction* exitAction = menu.addAction("Exit the program");
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
        m_audioEngine->playFile(fileName);
    }
}
bool RhythmFlow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->isAutoRepeat()) return true;

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
        m_audioEngine->playFile(QUrl(m_samplePaths[index]));
    }
}

void RhythmFlow::toggleGameMode()
{
    m_game->setActive(!m_game->isActive());
    if (!m_game->isActive()) {
        // 退出游戏，恢复纯享模式
    }
}