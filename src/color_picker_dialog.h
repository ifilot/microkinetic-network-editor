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
#include <QDialog>

class ColorWheelWidget;
class QLabel;
class QLineEdit;
class QSlider;
class QVBoxLayout;

/**
 * @brief Provide a dialog for choosing colors with a wheel, value slider, and swatches.
 *
 * The dialog keeps a synchronized HSV wheel and hex editor, and includes predefined
 * Solarized swatches loaded from a resource file. It exposes the selected color through
 * `color()` after the dialog is accepted.
 */
class ColorPickerDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Build the picker UI and initialize it from an input color.
     *
     * The constructor wires wheel, value slider, and hex input so each control reflects
     * changes from the others, and it adds optional swatch buttons loaded from YAML.
     *
     * @param initial Starting color shown when the dialog opens.
     * @param parent Optional parent widget for ownership and modality scoping.
     */
    explicit ColorPickerDialog(const QColor& initial, QWidget* parent = nullptr);

    /**
     * @brief Return the currently selected color from the wheel state.
     *
     * This accessor reflects the active HSV selection after user interaction or swatch
     * selection, regardless of whether the dialog has been accepted yet.
     *
     * @return Currently selected color.
     */
    QColor color() const;

private:
    /**
     * @brief Refresh the hex preview label and editor text for a color.
     *
     * The method formats the color as uppercase `#RRGGBB`, updates both display widgets,
     * and applies contrasting foreground text based on luminance for readability.
     *
     * @param c Color to display in hexadecimal form.
     */
    void updateHexDisplay(const QColor& c);
    /**
     * @brief Apply a color to all interactive picker controls.
     *
     * This updates the wheel state, synchronizes the value slider to brightness, and then
     * refreshes the hex preview widgets so the whole dialog reflects the same color.
     *
     * @param c Color to propagate through the dialog controls.
     */
    void applyColor(const QColor& c);
    /**
     * @brief Build the scrollable swatch panel from bundled palette data.
     *
     * The function creates grouped buttons per theme and connects each button to apply its
     * color when clicked. If no layout is provided it exits without creating UI elements.
     *
     * @param container_layout Parent layout that receives the swatch group box.
     */
    void buildSwatches(QVBoxLayout* container_layout);

    ColorWheelWidget* wheel_{nullptr};
    QSlider* valueSlider_{nullptr};
    QLabel* hexLabel_{nullptr};
    QLineEdit* hexEdit_{nullptr};
};
