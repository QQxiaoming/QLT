#include <QApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <array>
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
    struct StartupStep {
        int durationMs;
        std::function<void()> action;
    };
    const std::array<StartupStep, 4> startupSequence = {{
        {2500, [&ledThread, setFaceExpression] {
            ledThread.cmd("rainbow_flow");
            setFaceExpression(RobotFaceWidget::Expression::Happy);
        }},
        {1800, [&ledThread, setFaceExpression] {
            ledThread.cmd("fixed 255 100 180");
            setFaceExpression(RobotFaceWidget::Expression::Cute);
        }},
        {3000, [&ledThread, setFaceExpression] {
            ledThread.cmd("blink 255 80 140 350");
            setFaceExpression(RobotFaceWidget::Expression::Shy);
        }},
        {0, [&ledThread, setFaceExpression] {
            ledThread.cmd("rainbow_flow");
            setFaceExpression(RobotFaceWidget::Expression::Happy);
        }},
    }};

    QThread startupThread;
    QObject startupWorker;
    QTimer startupTimer(&startupWorker);
    startupTimer.setSingleShot(true);
    std::size_t startupStep = 0;
    startupWorker.moveToThread(&startupThread);
    const auto applyStartupStep = [&startupSequence, &startupTimer](std::size_t index) {
        const StartupStep& step = startupSequence[index];
        step.action();
        if (step.durationMs > 0) {
            startupTimer.start(step.durationMs);
        }
    };
    const auto restartStartupSequence = [&startupStep, &startupTimer, applyStartupStep] {
        startupStep = 0;
        startupTimer.stop();
        applyStartupStep(startupStep);
    };
    QObject::connect(&startupTimer, &QTimer::timeout, &startupWorker,
                     [&startupSequence, &startupStep, &startupTimer, applyStartupStep] {
                         ++startupStep;
                         if (startupStep >= startupSequence.size()) {
                             startupTimer.stop();
                             return;
                         }

                         applyStartupStep(startupStep);
                     });
    QObject::connect(&startupThread, &QThread::started, &startupWorker, restartStartupSequence);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &startupThread, &QThread::quit);
    startupThread.start();

    qRegisterMetaType<HardwareHal::HeadPetGesture>("HardwareHal::HeadPetGesture");
    GestureThread gestureThread(&hal);
    QObject::connect(&gestureThread, &GestureThread::gestureDetected, &app,
                     [&face, &startupWorker, restartStartupSequence](HardwareHal::HeadPetGesture gesture) {
                         switch (gesture) {
                         case HardwareHal::HeadPetGesture::Press:
                             //std::printf("gesture: press\n");
                             //face.setExpression(RobotFaceWidget::Expression::Custom);
                             //face.setCustomImage("/root/1.jpg");
                             break;
                         case HardwareHal::HeadPetGesture::Release:
                             //std::printf("gesture: release\n");
                             break;
                         case HardwareHal::HeadPetGesture::SwipeForward:
                             //std::printf("gesture: swipe forward\n");
                             QMetaObject::invokeMethod(&startupWorker, restartStartupSequence,
                                                       Qt::QueuedConnection);
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
