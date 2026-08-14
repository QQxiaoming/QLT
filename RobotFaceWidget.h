#ifndef ROBOTFACEWIDGET_H
#define ROBOTFACEWIDGET_H

#include <QWidget>

class QTimer;

class RobotFaceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RobotFaceWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void advanceAnimation();

private:
    enum Expression {
        Happy,
        Sad
    };

    void drawEye(QPainter &painter, const QPointF &center, qreal radius, qreal openness);

    QTimer *animationTimer;
    Expression expression;
    int animationFrame;
    int blinkFrame;
};

#endif