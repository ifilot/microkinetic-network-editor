/**************************************************************************
 *   This file is part of MICROKINETIC NETWORK EDITOR.                    *
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   MICROKINETIC NETWORK EDITOR is free software:                        *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   MANAGLYPH is distributed in the hope that it will be useful,         *
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

class ColorPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ColorPickerDialog(const QColor& initial, QWidget* parent = nullptr);

    QColor color() const;

private:
    void updateHexDisplay(const QColor& c);
    void applyColor(const QColor& c);
    void buildSwatches(QVBoxLayout* container_layout);

    ColorWheelWidget* wheel_{nullptr};
    QSlider* valueSlider_{nullptr};
    QLabel* hexLabel_{nullptr};
    QLineEdit* hexEdit_{nullptr};
};
