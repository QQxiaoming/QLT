#include <QApplication>
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
    ledThread.cmd("rainbow_flow");

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



    return app.exec();
}
