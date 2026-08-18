#include "RobotFaceWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <cmath>

RobotFaceWidget::RobotFaceWidget(QWidget *parent)
    : QWidget(parent)
    , animationTimer(new QTimer(this))
    , expression(Expression::Happy)
    , animationFrame(0)
{
    setMinimumSize(160, 160);
    setAttribute(Qt::WA_OpaquePaintEvent);

    animationTimer->setInterval(33);
    connect(animationTimer, &QTimer::timeout, this, &RobotFaceWidget::advanceAnimation);
    animationTimer->start();
}

void RobotFaceWidget::setExpression(Expression newExpression)
{
    if (expression == newExpression)
        return;

    expression = newExpression;
    animationFrame = 0;
    update();
}

void RobotFaceWidget::advanceAnimation()
{
    ++animationFrame;
    update();
}

void RobotFaceWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    switch (expression) {
    case Expression::Happy:
        setExpression(Expression::Cute);
        break;
    case Expression::Cute:
        setExpression(Expression::Shy);
        break;
    case Expression::Shy:
        setExpression(Expression::Happy);
        break;
    }
}

void RobotFaceWidget::drawDotEyes(QPainter &painter, qreal size, qreal verticalOffset) const
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    const qreal radius = size * 0.035;
    const qreal eyeY = size * 0.42 + verticalOffset;
    painter.drawEllipse(QPointF(size * 0.31, eyeY), radius, radius);
    painter.drawEllipse(QPointF(size * 0.69, eyeY), radius, radius);
}

void RobotFaceWidget::drawSparkleEye(QPainter &painter, qreal centerX, qreal centerY,
                                     qreal size, qreal verticalScale, qreal pupilOffset) const
{
    const qreal eyeRadius = size * 0.068;
    const qreal eyeRadiusY = eyeRadius * 1.18 * verticalScale;
    const QPointF eyeCenter(centerX, centerY);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(eyeCenter, eyeRadius, eyeRadiusY);

    const qreal pupilRadius = eyeRadius * 0.54;
    const qreal pupilRadiusY = pupilRadius * 1.12 * verticalScale;
    const QPointF pupilCenter(centerX + pupilOffset, centerY + eyeRadiusY * 0.16);
    painter.setBrush(QColor("#222238"));
    painter.drawEllipse(pupilCenter, pupilRadius, pupilRadiusY);

    painter.setBrush(Qt::white);
    painter.drawEllipse(QPointF(pupilCenter.x() - pupilRadius * 0.28,
                                pupilCenter.y() - pupilRadiusY * 0.3),
                pupilRadius * 0.3, pupilRadiusY * 0.3);
}

void RobotFaceWidget::drawSmile(QPainter &painter, qreal size, qreal verticalOffset) const
{
    QPainterPath mouth;
    const QRectF mouthArea(size * 0.32, size * 0.67 + verticalOffset, size * 0.36, size * 0.16);
    mouth.moveTo(mouthArea.left(), mouthArea.top() + mouthArea.height() * 0.18);
    mouth.cubicTo(mouthArea.left() + mouthArea.width() * 0.25, mouthArea.top() + mouthArea.height() * 0.78,
                  mouthArea.right() - mouthArea.width() * 0.25, mouthArea.top() + mouthArea.height() * 0.78,
                  mouthArea.right(), mouthArea.top() + mouthArea.height() * 0.18);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::white, size * 0.018, Qt::SolidLine, Qt::RoundCap));
    painter.drawPath(mouth);
}

void RobotFaceWidget::drawHappyExpression(QPainter &painter, qreal size) const
{
    const qreal breath = std::sin(animationFrame * 0.06) * size * 0.012;
    drawDotEyes(painter, size, breath);
    drawSmile(painter, size, breath);
}

void RobotFaceWidget::drawCuteExpression(QPainter &painter, qreal size) const
{
    const qreal breath = std::sin(animationFrame * 0.06) * size * 0.012;
    const qreal eyeY = size * 0.42 + breath;

    drawSparkleEye(painter, size * 0.31, eyeY, size);

    const int blinkFrame = animationFrame % 105;
    const qreal blinkProgress = blinkFrame >= 84 && blinkFrame <= 96
        ? 1.0 - std::abs(blinkFrame - 90) / 6.0
        : 0.0;
    if (blinkProgress < 0.85) {
        drawSparkleEye(painter, size * 0.69, eyeY, size, 1.0 - blinkProgress);
    } else {
        QPainterPath wink;
        wink.moveTo(size * 0.61, eyeY);
        wink.quadTo(size * 0.69, eyeY + size * 0.045, size * 0.77, eyeY);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::white, size * 0.02, Qt::SolidLine, Qt::RoundCap));
        painter.drawPath(wink);
    }

    drawSmile(painter, size, breath);
}

void RobotFaceWidget::drawShyExpression(QPainter &painter, qreal size) const
{
    const qreal breath = std::sin(animationFrame * 0.06) * size * 0.012;
    const int blushAlpha = 115 + static_cast<int>((std::sin(animationFrame * 0.08) + 1.0) * 25.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 170, 194, blushAlpha));
    painter.drawEllipse(QPointF(size * 0.23, size * 0.58 + breath), size * 0.075, size * 0.035);
    painter.drawEllipse(QPointF(size * 0.77, size * 0.58 + breath), size * 0.075, size * 0.035);

    const qreal gazeOffset = std::sin(animationFrame * 0.045) * size * 0.008;
    const qreal eyeY = size * 0.42 + breath;
    drawSparkleEye(painter, size * 0.31, eyeY, size, 1.0, gazeOffset);
    drawSparkleEye(painter, size * 0.69, eyeY, size, 1.0, gazeOffset);
    drawSmile(painter, size, breath);
}

void RobotFaceWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), Qt::black);

    const qreal size = qMin(width(), height());
    const QPointF origin((width() - size) / 2.0, (height() - size) / 2.0);
    painter.translate(origin);
    painter.translate(0, -size * 0.06);
    painter.translate(size * 0.5, size * 0.5);
    painter.scale(1.3, 1.3);
    painter.translate(-size * 0.5, -size * 0.5);

    switch (expression) {
    case Expression::Happy:
        drawHappyExpression(painter, size);
        break;
    case Expression::Cute:
        drawCuteExpression(painter, size);
        break;
    case Expression::Shy:
        drawShyExpression(painter, size);
        break;
    }
}