#include <QApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <array>
#include <atomic>
#include <cstdio>
#include <functional>

#include "GestureThread.h"
#include "HardwareHal.h"
#include "LedThread.h"
#include "RobotFaceWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    HardwareHal hal;
    if (!hal.initialize()) {
        std::fprintf(stderr, "Failed to initialize hardware HAL\n");
        return 1;
    }

    RobotFaceWidget face;
    face.setWindowFlag(Qt::FramelessWindowHint);
    face.showFullScreen();

    LedThread ledThread(&hal);
    ledThread.start();

    const auto setFaceExpression = [&face](RobotFaceWidget::Expression expression) {
        QMetaObject::invokeMethod(&face, [expression, &face] {
            face.setExpression(expression);
        }, Qt::QueuedConnection);
    };
    const auto setCustomImage = [&face](const QString &imagePath) {
        QMetaObject::invokeMethod(&face, [imagePath, &face] {
            face.setCustomImage(imagePath);
        }, Qt::QueuedConnection);
    };
    struct StartupStep {
        int durationMs;
        bool waitForPress;
        std::function<void()> action;
    };
    const std::array<StartupStep, 14> startupSequence = {{
        {2000, false, [&ledThread, setFaceExpression] {
            // Hello, 我最爱宝贝，今天是七夕节，这是我送给你的礼物
            system("tinyplay /root/1.wav");
            ledThread.cmd("rainbow_flow");
            setFaceExpression(RobotFaceWidget::Expression::Happy);
        }},
        {2000, false, [&ledThread, setFaceExpression] {
            // 今天是我们在一起的1186天，我们一起走过来三个春秋
            system("tinyplay /root/2.wav");
            ledThread.cmd("blink 255 100 180 260");
            setFaceExpression(RobotFaceWidget::Expression::Cute);
        }},
        {2000, false, [&ledThread, setFaceExpression] {
            // 回忆我们的相识相知，过去的每一天我都非常的幸福
            system("tinyplay /root/3.wav");
            ledThread.cmd("rainbow_flow");
            setFaceExpression(RobotFaceWidget::Expression::Love);
        }},
        {2000, false, [&ledThread, setFaceExpression] {
            // 三年前我们在紫金山，第一次牵你的手，我的心砰砰跳
            system("tinyplay /root/4.wav");
            ledThread.cmd("blink 255 70 150 180");
            setFaceExpression(RobotFaceWidget::Expression::Excited);
        }},
        {2000, false, [&ledThread, setFaceExpression] {
            // 两年前我们一起旅行，在海边，在山上，在草原，在沙漠，我想永远和你在一起
            system("tinyplay /root/5.wav");
            ledThread.cmd("fixed 60 150 255");
            setFaceExpression(RobotFaceWidget::Expression::Happy);
        }},
        {2000, false, [&ledThread, setFaceExpression] {
            // 一年前我们终于携手走进婚姻的殿堂，共同筑起我们的小家，我在外漂泊的我终于有了自己的小家
            system("tinyplay /root/6.wav");
            ledThread.cmd("blink 255 180 70 420");
            setFaceExpression(RobotFaceWidget::Expression::Love);
        }},
        {2000, false, [&ledThread, setFaceExpression] {
            // 而现在，我们也即将拥有属于我们自己的小宝宝，这些天辛苦了你，我想对你说，我爱你，我的宝贝
            system("tinyplay /root/6.wav");
            ledThread.cmd("blink 255 100 180 140");
            setFaceExpression(RobotFaceWidget::Expression::Shy);
        }},
        {0, true, [&ledThread, setFaceExpression] {
            // 宝贝，谢谢你，我会一直陪伴在你身边，守护你，爱你，直到永远，现在，请你点击机器人的头顶
            system("tinyplay /root/6.wav");
            ledThread.cmd("fixed 255 70 140");
            setFaceExpression(RobotFaceWidget::Expression::Love);
        }},
        {2000, false, [&ledThread, setFaceExpression, setCustomImage] {
            // [播放音乐] 照片1
            system("tinyplay /root/7.wav");
            ledThread.cmd("rainbow_flow");
            setFaceExpression(RobotFaceWidget::Expression::Custom);
            setCustomImage("/root/1.jpg");
        }},
        {2000, false, [&ledThread, setFaceExpression, setCustomImage] {
            // 照片2
            ledThread.cmd("fixed 255 120 80");
            setFaceExpression(RobotFaceWidget::Expression::Custom);
            setCustomImage("/root/2.jpg");
        }},
        {2000, false, [&ledThread, setFaceExpression, setCustomImage] {
            // 照片3
            ledThread.cmd("fixed 80 160 255");
            setFaceExpression(RobotFaceWidget::Expression::Custom);
            setCustomImage("/root/3.jpg");
        }},
        {2000, false, [&ledThread, setFaceExpression, setCustomImage] {
            // 照片4
            ledThread.cmd("blink 255 100 180 300");
            setFaceExpression(RobotFaceWidget::Expression::Custom);
            setCustomImage("/root/4.jpg");
        }},
        {2000, false, [&ledThread, setFaceExpression, setCustomImage] {
            // 照片5
            ledThread.cmd("fixed 180 80 255");
            setFaceExpression(RobotFaceWidget::Expression::Custom);
            setCustomImage("/root/5.jpg");
        }},
        {0, false, [&ledThread, setFaceExpression] {
            ledThread.cmd("rainbow_flow");
            setFaceExpression(RobotFaceWidget::Expression::Happy);
        }},
    }};

    QThread startupThread;
    QObject startupWorker;
    QTimer startupTimer(&startupWorker);
    startupTimer.setSingleShot(true);
    std::size_t startupStep = 0;
    std::atomic<bool> startupSequenceFinished{false};
    startupWorker.moveToThread(&startupThread);
    const auto applyStartupStep = [&startupSequence, &startupTimer, &startupSequenceFinished](std::size_t index) {
        const StartupStep& step = startupSequence[index];
        step.action();
        // Terminal step: no timer, no press wait, sequence is considered complete.
        startupSequenceFinished = !step.waitForPress && step.durationMs <= 0;
        if (!step.waitForPress && step.durationMs > 0) {
            startupTimer.start(step.durationMs);
        }
    };
    std::function<void()> advanceStartupStep;
    advanceStartupStep = [&startupSequence, &startupStep, &startupTimer, &startupSequenceFinished, applyStartupStep] {
        ++startupStep;
        if (startupStep >= startupSequence.size()) {
            startupTimer.stop();
            startupSequenceFinished = true;
            return;
        }

        applyStartupStep(startupStep);
    };
    const auto restartStartupSequence = [&startupStep, &startupTimer, applyStartupStep] {
        startupStep = 0;
        startupTimer.stop();
        applyStartupStep(startupStep);
    };
    QObject::connect(&startupTimer, &QTimer::timeout, &startupWorker,
                     [&advanceStartupStep] { advanceStartupStep(); });
    QObject::connect(&startupThread, &QThread::started, &startupWorker, restartStartupSequence);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &startupThread, &QThread::quit);
    startupThread.start();

    qRegisterMetaType<HardwareHal::HeadPetGesture>("HardwareHal::HeadPetGesture");
    GestureThread gestureThread(&hal);
    QObject::connect(&gestureThread, &GestureThread::gestureDetected, &app,
                     [&face, &startupWorker, &startupSequence, &startupStep, &startupSequenceFinished,
                      &advanceStartupStep, restartStartupSequence](HardwareHal::HeadPetGesture gesture) {
                         switch (gesture) {
                         case HardwareHal::HeadPetGesture::Press:
                             //std::printf("gesture: press\n");
                             //face.setExpression(RobotFaceWidget::Expression::Custom);
                             //face.setCustomImage("/root/1.jpg");
                             if (startupStep < startupSequence.size()
                                 && startupSequence[startupStep].waitForPress) {
                                 QMetaObject::invokeMethod(&startupWorker, advanceStartupStep,
                                                           Qt::QueuedConnection);
                             }
                             break;
                         case HardwareHal::HeadPetGesture::Release:
                             //std::printf("gesture: release\n");
                             break;
                         case HardwareHal::HeadPetGesture::SwipeForward:
                             //std::printf("gesture: swipe forward\n");
                             if (startupSequenceFinished) {
                                 QMetaObject::invokeMethod(&startupWorker, restartStartupSequence,
                                                           Qt::QueuedConnection);
                             }
                             break;
                         case HardwareHal::HeadPetGesture::SwipeBackward:
                             //std::printf("gesture: swipe backward\n");
                             break;
                         case HardwareHal::HeadPetGesture::None:
                             break;
                         }
                     },
                     Qt::QueuedConnection);
    gestureThread.start();

    const int result = app.exec();
    startupThread.wait();
    return result;
}
