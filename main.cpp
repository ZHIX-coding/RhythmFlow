#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QSplashScreen>
#include <QPainter>
#include <QRandomGenerator>
#include "RhythmFlow.h"

// 自定义启动画面类
// 程序启动时显示深色背景、旋转粒子、青白渐变标题
// 2.5 秒后自动淡出，主窗口同步淡入
class SplashScreen : public QSplashScreen
{
public:
    SplashScreen()
        : QSplashScreen(QPixmap(800, 600))   // 创建 800x600 空白画布
    {
        // 透明度特效（初始完全透明）
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(effect);
        effect->setOpacity(0.0);

        // 淡入动画（0 → 1，持续 500ms）
        QPropertyAnimation* fadeIn = new QPropertyAnimation(effect, "opacity", this);
        fadeIn->setDuration(500);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);

        // 淡出动画（1 → 0，持续 500ms）
        QPropertyAnimation* fadeOut = new QPropertyAnimation(effect, "opacity", this);
        fadeOut->setDuration(500);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        // 淡出结束后关闭启动画面并释放内存
        connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
            close();
            deleteLater();
            });

        show();
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

        // 背景粒子动画定时器（每 50ms 更新一次）
        QTimer* animTimer = new QTimer(this);
        connect(animTimer, &QTimer::timeout, this, [this]() {
            m_phase += 0.05f;
            repaint();
            });
        animTimer->start(50);

        // 2.5 秒后开始淡出
        QTimer::singleShot(2500, this, [fadeOut]() {
            fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
            });
    }

protected:
    // 绘制启动画面内容
    void drawContents(QPainter* painter) override
    {
        // 背景：深青黑色
        painter->fillRect(rect(), QColor(10, 30, 40));

        // 60 个旋转粒子（模拟星云）
        painter->setPen(Qt::NoPen);
        for (int i = 0; i < 60; ++i) {
            float angle = m_phase * 0.3f + i * 0.4f;
            float radius = 100.0f + 60.0f * sinf(m_phase * 0.7f + i * 0.2f);
            float x = 400 + radius * cosf(angle);
            float y = 300 + radius * sinf(angle) * 0.6f;
            int alpha = 30 + QRandomGenerator::global()->bounded(70);
            painter->setBrush(QColor(0, 200, 255, alpha));
            painter->drawEllipse(QPointF(x, y), 2.0f, 2.0f);
        }

        // 主标题 "RhythmFlow"（青白渐变）
        QFont titleFont("Arial", 48, QFont::Bold);
        painter->setFont(titleFont);
        QLinearGradient textGrad(rect().center().x() - 200, rect().center().y() - 100,
            rect().center().x() + 200, rect().center().y());
        textGrad.setColorAt(0, QColor(0, 238, 255));
        textGrad.setColorAt(1, QColor(255, 255, 255));
        painter->setPen(QPen(QBrush(textGrad), 1));
        painter->drawText(QRectF(0, rect().center().y() - 120, rect().width(), 120),
            Qt::AlignHCenter | Qt::AlignBottom, "RhythmFlow");

        // 副标题
        QFont subFont("Arial", 14);
        painter->setFont(subFont);
        painter->setPen(QColor(180, 180, 180, 180));
        painter->drawText(QRectF(0, rect().center().y(), rect().width(), 30),
            Qt::AlignHCenter | Qt::AlignTop,
            "Audio-Driven Visualizer & Rhythm Game");

        // 底部 Loading 提示
        QFont loadFont("Arial", 10);
        painter->setFont(loadFont);
        painter->setPen(QColor(128, 128, 128, 150));
        painter->drawText(QRectF(0, rect().bottom() - 40, rect().width(), 30),
            Qt::AlignHCenter | Qt::AlignBottom, "Loading...");
    }

private:
    float m_phase = 0.0f;   // 粒子动画相位
};

// 程序主入口
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);   // 防止启动画面关闭导致程序退出

    SplashScreen* splash = new SplashScreen();   // 显示启动画面

    RhythmFlow w;   // 创建主窗口

    // 主窗口初始完全透明，通过透明度特效控制淡入
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(&w);
    w.setGraphicsEffect(effect);
    effect->setOpacity(0.0);
    w.show();

    splash->raise();   // 确保启动画面在主窗口上方

    // 2.5 秒后主窗口开始淡入
    QTimer::singleShot(2500, &w, [&]() {
        QPropertyAnimation* fadeIn = new QPropertyAnimation(effect, "opacity", &w);
        fadeIn->setDuration(500);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        // 动画结束后移除透明度特效，恢复窗口正常渲染
        QObject::connect(fadeIn, &QPropertyAnimation::finished, &w, [&w]() {
            w.setGraphicsEffect(nullptr);
            });
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        });

    return app.exec();
}