#ifndef ROBOTFACEWIDGET_H
#define ROBOTFACEWIDGET_H

#include <QWidget>

class QTimer;
class QMouseEvent;

class RobotFaceWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Expression {
        Happy,
        Cute,
        Shy
    };

    explicit RobotFaceWidget(QWidget *parent = nullptr);
    void setExpression(Expression expression);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void advanceAnimation();

private:
    void drawHappyExpression(QPainter &painter, qreal size) const;
    void drawCuteExpression(QPainter &painter, qreal size) const;
    void drawShyExpression(QPainter &painter, qreal size) const;
    void drawDotEyes(QPainter &painter, qreal size, qreal verticalOffset = 0.0) const;
    void drawSparkleEye(QPainter &painter, qreal centerX, qreal centerY, qreal size,
                        qreal verticalScale = 1.0, qreal pupilOffset = 0.0) const;
    void drawSmile(QPainter &painter, qreal size, qreal verticalOffset = 0.0) const;

    QTimer *animationTimer;
    Expression expression;
    int animationFrame;
};

#endif