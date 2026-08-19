#include "RobotFaceWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <algorithm>
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

    if (expressionIsAnimated())
        animationTimer->start();
    else
        animationTimer->stop();

    update();
}

bool RobotFaceWidget::setCustomImage(const QString &imagePath)
{
    QPixmap image(imagePath);
    if (image.isNull())
        return false;

    customImage = image;
    if (expression != Expression::Custom) {
        setExpression(Expression::Custom);
    } else {
        // Keep expression unchanged but force repaint for new image content.
        update();
    }
    return true;
}

void RobotFaceWidget::advanceAnimation()
{
    ++animationFrame;
    update();
}

bool RobotFaceWidget::expressionIsAnimated() const
{
    return expression != Expression::Custom;
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

void RobotFaceWidget::drawTwinkleStar(QPainter &painter, qreal centerX, qreal centerY,
                                      qreal starRadius, qreal opacity) const
{
    if (opacity <= 0.0)
        return;

    QPainterPath star;
    for (int i = 0; i < 8; ++i) {
        const qreal angle = i * M_PI / 4.0;
        const qreal radius = (i % 2 == 0) ? starRadius : starRadius * 0.35;
        const QPointF point(centerX + radius * std::sin(angle), centerY - radius * std::cos(angle));
        if (i == 0)
            star.moveTo(point);
        else
            star.lineTo(point);
    }
    star.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 235, 150, static_cast<int>(opacity * 235)));
    painter.drawPath(star);
}

void RobotFaceWidget::drawBalloon(QPainter &painter, qreal centerX, qreal centerY, qreal size,
                                  const QColor &color) const
{
    const qreal balloonWidth = size * 0.09;
    const qreal balloonHeight = size * 0.115;

    QPainterPath balloon;
    balloon.addEllipse(QPointF(centerX, centerY), balloonWidth, balloonHeight);
    balloon.moveTo(centerX - balloonWidth * 0.18, centerY + balloonHeight * 0.92);
    balloon.lineTo(centerX, centerY + balloonHeight * 1.18);
    balloon.lineTo(centerX + balloonWidth * 0.18, centerY + balloonHeight * 0.92);
    balloon.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPath(balloon);

    painter.setPen(QPen(color.darker(130), size * 0.004));
    painter.drawLine(QPointF(centerX, centerY + balloonHeight * 1.18),
                     QPointF(centerX, centerY + balloonHeight * 2.6));
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

void RobotFaceWidget::drawExcitedExpression(QPainter &painter, qreal size) const
{
    const qreal breath = std::sin(animationFrame * 0.06) * size * 0.012;
    const qreal eyeY = size * 0.42 + breath;

    drawSparkleEye(painter, size * 0.31, eyeY, size);
    drawSparkleEye(painter, size * 0.69, eyeY, size);

    struct StarSpot { qreal dx; qreal dy; qreal radius; qreal speed; qreal phase; };
    static const StarSpot leftStars[] = {
        {-0.09, -0.10, 0.022, 0.10, 0.0},
        {0.10, -0.06, 0.016, 0.13, 2.1},
        {-0.02, -0.14, 0.013, 0.16, 4.2},
    };
    static const StarSpot rightStars[] = {
        {0.09, -0.10, 0.022, 0.10, 1.4},
        {-0.10, -0.06, 0.016, 0.13, 3.5},
        {0.02, -0.14, 0.013, 0.16, 5.6},
    };
    for (const StarSpot &star : leftStars) {
        const qreal opacity = std::max(0.0, std::sin(animationFrame * star.speed + star.phase));
        drawTwinkleStar(painter, size * (0.31 + star.dx), eyeY + size * star.dy,
                        size * star.radius, opacity);
    }
    for (const StarSpot &star : rightStars) {
        const qreal opacity = std::max(0.0, std::sin(animationFrame * star.speed + star.phase));
        drawTwinkleStar(painter, size * (0.69 + star.dx), eyeY + size * star.dy,
                        size * star.radius, opacity);
    }

    drawSmile(painter, size, breath);
}

void RobotFaceWidget::drawLoveExpression(QPainter &painter, qreal size) const
{
    const qreal breath = std::sin(animationFrame * 0.06) * size * 0.012;
    const qreal eyeY = size * 0.42 + breath;

    drawSparkleEye(painter, size * 0.31, eyeY, size);
    drawSparkleEye(painter, size * 0.69, eyeY, size);
    drawSmile(painter, size, breath);

    struct Balloon { qreal xFraction; qreal phase; QColor color; };
    static const Balloon balloons[] = {
        {0.16, 0.0, QColor("#FF6F9F")},
        {0.42, 90.0, QColor("#8F7BFF")},
        {0.66, 190.0, QColor("#5AC8FA")},
        {0.86, 40.0, QColor("#FFC857")},
    };
    constexpr qreal loopFrames = 260.0;
    for (const Balloon &balloon : balloons) {
        const qreal t = std::fmod(animationFrame + balloon.phase, loopFrames) / loopFrames;
        const qreal centerY = size * 1.15 - t * size * 1.5;
        const qreal sway = std::sin((animationFrame + balloon.phase) * 0.03) * size * 0.02;
        drawBalloon(painter, size * balloon.xFraction + sway, centerY, size, balloon.color);
    }
}

void RobotFaceWidget::drawCustomImage(QPainter &painter) const
{
    if (customImage.isNull())
        return;

    const QPixmap scaledImage = customImage.scaled(size(), Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation);
    const QPoint imageOrigin((width() - scaledImage.width()) / 2,
                             (height() - scaledImage.height()) / 2);
    painter.drawPixmap(imageOrigin, scaledImage);
}

void RobotFaceWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), Qt::black);

    if (expression == Expression::Custom) {
        drawCustomImage(painter);
        return;
    }

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
    case Expression::Excited:
        drawExcitedExpression(painter, size);
        break;
    case Expression::Love:
        drawLoveExpression(painter, size);
        break;
    case Expression::Custom:
        break;
    }
}