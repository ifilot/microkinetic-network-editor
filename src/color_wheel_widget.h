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

#pragma once

#include <QColor>
#include <QWidget>

/**
 * @brief Render an HSV color wheel and emit updates when the user selects a point on it.
 *
 * Hue is encoded as angle around the center and saturation as radial distance, while value
 * is controlled externally and reused when repainting. Mouse interaction updates hue and
 * saturation and notifies listeners through the `colorChanged` signal.
 */
class ColorWheelWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Create the widget and enforce its minimum display size.
     *
     * The constructor sets the widget minimum size to the same dimensions as the preferred
     * size so the wheel remains fully visible and interactive in the picker layout.
     *
     * @param parent Optional parent widget that owns this control.
     */
    explicit ColorWheelWidget(QWidget* parent = nullptr);

    /**
     * @brief Apply a new HSV color state and notify listeners.
     *
     * The method decomposes the provided color into hue, saturation, and value components,
     * triggers a repaint, and emits `colorChanged` with the normalized color currently held
     * by the widget.
     *
     * @param c Color to convert into the wheel's internal HSV coordinates.
     */
    void setColor(const QColor& c);
    /**
     * @brief Build a QColor from the current HSV state.
     *
     * This method returns a color generated from `hue_`, `saturation_`, and `value_`, which
     * represent the current marker position and brightness level of the wheel.
     *
     * @return Current color represented by the widget.
     */
    QColor color() const;
    /**
     * @brief Report the preferred dimensions used for layout.
     *
     * The widget uses a square preferred size so the painted wheel retains the correct
     * circular geometry without distortion.
     *
     * @return Preferred width and height for the control.
     */
    QSize sizeHint() const override;
    /**
     * @brief Report the minimum dimensions allowed for the widget.
     *
     * The minimum size matches the preferred size to prevent shrinking that would degrade
     * wheel readability and pointer precision.
     *
     * @return Minimum width and height for stable rendering.
     */
    QSize minimumSizeHint() const override;

signals:
    /**
     * @brief Emit the color selected on the wheel.
     *
     * The signal is sent whenever interaction or programmatic updates change the internal
     * HSV state, allowing connected widgets to stay synchronized.
     *
     * @param color Newly selected color in RGB/HSV-compatible form.
     */
    void colorChanged(const QColor& color);

protected:
    /**
     * @brief Paint the circular HSV wheel and current selection marker.
     *
     * The implementation shades each pixel inside the wheel radius according to its polar
     * coordinates, then draws an outline marker at the active hue/saturation position.
     *
     * @param event Paint event supplied by Qt (unused by the implementation).
     */
    void paintEvent(QPaintEvent* event) override;
    /**
     * @brief Start color selection at the mouse press position.
     *
     * A press immediately maps pointer coordinates to hue/saturation and emits a color
     * update so the dialog reflects the selected point without requiring dragging.
     *
     * @param event Mouse press event carrying the clicked position.
     */
    void mousePressEvent(QMouseEvent* event) override;
    /**
     * @brief Update color selection while the left mouse button is dragged.
     *
     * The handler only reacts during left-button drags, converting each pointer position
     * into new hue/saturation coordinates for continuous wheel interaction.
     *
     * @param event Mouse move event with cursor position and button state.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    /**
     * @brief Convert a widget-space position into hue and saturation values.
     *
     * The function computes polar coordinates around the wheel center, wraps the angle into
     * the [0,1) hue interval, clamps saturation to the wheel radius, repaints, and emits
     * the resulting color.
     *
     * @param pos Cursor position in local widget coordinates.
     */
    void updateFromPosition(const QPoint& pos);

    qreal hue_{0.0};
    qreal saturation_{0.0};
    qreal value_{1.0};
};
