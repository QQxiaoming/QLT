#include <QApplication>

#include "RobotFaceWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    RobotFaceWidget face;
    face.setWindowFlag(Qt::FramelessWindowHint);
    face.showFullScreen();
    return app.exec();
}
