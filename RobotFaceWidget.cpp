#include "RobotFaceWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <cmath>

RobotFaceWidget::RobotFaceWidget(QWidget *parent)
    : QWidget(parent)
    , animationTimer(new QTimer(this))
    , expression(Happy)
    , animationFrame(0)
    , blinkFrame(0)
{
    setMinimumSize(160, 160);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setCursor(Qt::PointingHandCursor);

    animationTimer->setInterval(33);
    connect(animationTimer, &QTimer::timeout, this, &RobotFaceWidget::advanceAnimation);
    animationTimer->start();
}

void RobotFaceWidget::advanceAnimation()
{
    ++animationFrame;
    const int blinkCycle = animationFrame % 150;
    blinkFrame = blinkCycle >= 132 && blinkCycle < 140 ? blinkCycle - 132 : 0;
    update();
}

void RobotFaceWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        expression = expression == Happy ? Sad : Happy;

    QWidget::mousePressEvent(event);
}

void RobotFaceWidget::drawEye(QPainter &painter, const QPointF &center, qreal radius, qreal openness)
{
    painter.save();
    painter.translate(center);
    painter.scale(1.0, openness);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#222238"));
    painter.drawEllipse(QPointF(0, 0), radius, radius);

    painter.setBrush(QColor("#FFFFFF"));
    painter.drawEllipse(QPointF(-radius * 0.25, -radius * 0.28), radius * 0.22, radius * 0.25);
    painter.setBrush(QColor("#A6F2FF"));
    painter.drawEllipse(QPointF(radius * 0.22, radius * 0.34), radius * 0.11, radius * 0.13);
    painter.restore();
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

    const qreal breath = std::sin(animationFrame * 0.06) * size * 0.012;

    const qreal blink = blinkFrame == 0 ? 1.0 : qMax(0.08, 1.0 - std::abs(4 - blinkFrame) * 0.23);
    const qreal eyeY = size * 0.42 + breath;
    drawEye(painter, QPointF(size * 0.31, eyeY), size * 0.135, blink);
    drawEye(painter, QPointF(size * 0.69, eyeY), size * 0.135, blink);

    QPainterPath mouth;
    const QRectF mouthArea(size * 0.32, size * 0.67 + breath, size * 0.36, size * 0.16);
    mouth.moveTo(mouthArea.left(), mouthArea.top() + mouthArea.height() * (expression == Happy ? 0.25 : 0.75));
    mouth.quadTo(mouthArea.center().x(), mouthArea.top() + mouthArea.height() * (expression == Happy ? 1.12 : -0.12),
                 mouthArea.right(), mouthArea.top() + mouthArea.height() * (expression == Happy ? 0.25 : 0.75));
    painter.setPen(QPen(QColor("#FFFFFF"), size * 0.035, Qt::SolidLine, Qt::RoundCap));
    painter.drawPath(mouth);

    if (expression == Sad) {
        const qreal drop = (animationFrame % 45) / 45.0;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#63D5F7"));
        painter.drawEllipse(QPointF(size * 0.84, size * (0.52 + drop * 0.25)), size * 0.035, size * 0.058);
    }
}