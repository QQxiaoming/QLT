#include <QApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <cstdio>

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

    QThread startupThread;
    QObject startupWorker;
    startupWorker.moveToThread(&startupThread);
    QObject::connect(&startupThread, &QThread::started, &startupWorker,
                     [&face, &ledThread, &startupThread, &startupWorker] {
                         const auto setFaceExpression = [&face](RobotFaceWidget::Expression expression) {
                             QMetaObject::invokeMethod(&face, [expression, &face] {
                                 face.setExpression(expression);
                             }, Qt::QueuedConnection);
                         };

                         ledThread.cmd("rainbow_flow");
                         setFaceExpression(RobotFaceWidget::Expression::Happy);

                         QTimer::singleShot(2500, &startupWorker, [&ledThread, setFaceExpression] {
                             ledThread.cmd("fixed 255 100 180");
                             setFaceExpression(RobotFaceWidget::Expression::Cute);
                         });
                         QTimer::singleShot(5000, &startupWorker, [&ledThread, setFaceExpression] {
                             ledThread.cmd("blink 255 80 140 350");
                             setFaceExpression(RobotFaceWidget::Expression::Shy);
                         });
                         QTimer::singleShot(7500, &startupWorker, [&ledThread, setFaceExpression] {
                             ledThread.cmd("rainbow_flow");
                             setFaceExpression(RobotFaceWidget::Expression::Happy);
                         });
                         QTimer::singleShot(9000, &startupWorker, [&startupThread] {
                             startupThread.quit();
                         });
                     });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &startupThread, &QThread::quit);
    startupThread.start();

    qRegisterMetaType<HardwareHal::HeadPetGesture>("HardwareHal::HeadPetGesture");
    GestureThread gestureThread(&hal);
    QObject::connect(&gestureThread, &GestureThread::gestureDetected, &app,
                     [&face](HardwareHal::HeadPetGesture gesture) {
                         switch (gesture) {
                         case HardwareHal::HeadPetGesture::Press:
                             std::printf("gesture: press\n");
                             face.setExpression(RobotFaceWidget::Expression::Custom);
                             face.setCustomImage("/root/1.jpg");
                             break;
                         case HardwareHal::HeadPetGesture::Release:
                             std::printf("gesture: release\n");
                             break;
                         case HardwareHal::HeadPetGesture::SwipeForward:
                             std::printf("gesture: swipe forward\n");
                             break;
                         case HardwareHal::HeadPetGesture::SwipeBackward:
                             std::printf("gesture: swipe backward\n");
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
