/**************************************************************************
 *   This file is part of MICROKINETIC NETWORK EDITOR.                    *
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   MICROKINETIC NETWORK EDITOR (MNE) is free software:                  *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   MNE is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "color_wheel_widget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include <cmath>

/**
 * @brief Create the widget and enforce its minimum display size.
 *
 * The constructor sets the widget minimum size to the same dimensions as the preferred
 * size so the wheel remains fully visible and interactive in the picker layout.
 *
 * @param parent Optional parent widget that owns this control.
 */
ColorWheelWidget::ColorWheelWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(minimumSizeHint());
}

/**
 * @brief Report the preferred dimensions used for layout.
 *
 * The widget uses a square preferred size so the painted wheel retains the correct
 * circular geometry without distortion.
 *
 * @return Preferred width and height for the control.
 */
QSize ColorWheelWidget::sizeHint() const {
    return QSize(220, 220);
}

/**
 * @brief Report the minimum dimensions allowed for the widget.
 *
 * The minimum size matches the preferred size to prevent shrinking that would degrade
 * wheel readability and pointer precision.
 *
 * @return Minimum width and height for stable rendering.
 */
QSize ColorWheelWidget::minimumSizeHint() const {
    return sizeHint();
}

/**
 * @brief Apply a new HSV color state and notify listeners.
 *
 * The method decomposes the provided color into hue, saturation, and value components,
 * triggers a repaint, and emits `colorChanged` with the normalized color currently held
 * by the widget.
 *
 * @param c Color to convert into the wheel's internal HSV coordinates.
 */
void ColorWheelWidget::setColor(const QColor& c) {
    c.getHsvF(&hue_, &saturation_, &value_);
    update();
    emit colorChanged(color());
}

/**
 * @brief Build a QColor from the current HSV state.
 *
 * This method returns a color generated from `hue_`, `saturation_`, and `value_`, which
 * represent the current marker position and brightness level of the wheel.
 *
 * @return Current color represented by the widget.
 */
QColor ColorWheelWidget::color() const {
    QColor c;
    c.setHsvF(hue_, saturation_, value_);
    return c;
}

/**
 * @brief Paint the circular HSV wheel and current selection marker.
 *
 * The implementation shades each pixel inside the wheel radius according to its polar
 * coordinates, then draws an outline marker at the active hue/saturation position.
 *
 * @param event Paint event supplied by Qt (unused by the implementation).
 */
void ColorWheelWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int side = qMin(width(), height());
    const QPointF center(width() / 2.0, height() / 2.0);
    const qreal radius = side / 2.0 - 6.0;

    QImage img(side, side, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    for (int y = 0; y < side; ++y) {
        for (int x = 0; x < side; ++x) {
            const qreal dx = x - side / 2.0;
            const qreal dy = y - side / 2.0;
            const qreal r = std::sqrt(dx * dx + dy * dy);

            if (r > radius) {
                continue;
            }

            qreal angle = std::atan2(-dy, dx);
            qreal hue = std::fmod(angle / (2.0 * M_PI) + 1.0, 1.0);
            qreal sat = qMin(1.0, r / radius);

            QColor c;
            c.setHsvF(hue, sat, value_);
            img.setPixelColor(x, y, c);
        }
    }

    p.drawImage(
        QRectF(center.x() - side / 2.0, center.y() - side / 2.0, side, side),
        img);

    const qreal a = hue_ * 2.0 * M_PI;
    const qreal r = saturation_ * radius;

    QPointF sel(center.x() + r * std::cos(a), center.y() - r * std::sin(a));

    p.setBrush(Qt::white);
    p.setPen(QPen(Qt::black, 2));
    p.drawEllipse(sel, 6, 6);
}

/**
 * @brief Start color selection at the mouse press position.
 *
 * A press immediately maps pointer coordinates to hue/saturation and emits a color
 * update so the dialog reflects the selected point without requiring dragging.
 *
 * @param e Mouse press event carrying the clicked position.
 */
void ColorWheelWidget::mousePressEvent(QMouseEvent* e) {
    updateFromPosition(e->pos());
}

/**
 * @brief Update color selection while the left mouse button is dragged.
 *
 * The handler only reacts during left-button drags, converting each pointer position
 * into new hue/saturation coordinates for continuous wheel interaction.
 *
 * @param e Mouse move event with cursor position and button state.
 */
void ColorWheelWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        updateFromPosition(e->pos());
    }
}

/**
 * @brief Convert a widget-space position into hue and saturation values.
 *
 * The function computes polar coordinates around the wheel center, wraps the angle into
 * the [0,1) hue interval, clamps saturation to the wheel radius, repaints, and emits
 * the resulting color.
 *
 * @param pos Cursor position in local widget coordinates.
 */
void ColorWheelWidget::updateFromPosition(const QPoint& pos) {
    const QPointF center(width() / 2.0, height() / 2.0);
    QPointF d = pos - center;

    const qreal angle = std::atan2(-d.y(), d.x());
    const qreal dist = std::sqrt(d.x() * d.x() + d.y() * d.y());

    hue_ = std::fmod((angle / (2.0 * M_PI)) + 1.0, 1.0);
    saturation_ = qMin(1.0, dist / (qMin(width(), height()) / 2.0));

    update();
    emit colorChanged(color());
}
