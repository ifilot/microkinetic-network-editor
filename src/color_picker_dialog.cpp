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
 
#include "color_picker_dialog.h"

#include "color_wheel_widget.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace {
QColor idealTextColor(const QColor& bg) {
    const double luminance =
        0.299 * bg.redF() +
        0.587 * bg.greenF() +
        0.114 * bg.blueF();
    return luminance > 0.5 ? Qt::black : Qt::white;
}
}  // namespace

ColorPickerDialog::ColorPickerDialog(const QColor& initial, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Select color"));
    setModal(true);
    resize(400, 450);

    auto* main = new QVBoxLayout(this);

    auto* center = new QHBoxLayout();

    wheel_ = new ColorWheelWidget(this);
    center->addWidget(wheel_, 1);

    valueSlider_ = new QSlider(Qt::Vertical, this);
    valueSlider_->setRange(0, 100);
    valueSlider_->setFixedWidth(22);
    center->addWidget(valueSlider_);

    main->addLayout(center);

    hexLabel_ = new QLabel(this);
    hexLabel_->setAlignment(Qt::AlignCenter);
    hexLabel_->setMinimumHeight(36);
    main->addWidget(hexLabel_);

    hexEdit_ = new QLineEdit(this);
    hexEdit_->setPlaceholderText("#RRGGBB");
    main->addWidget(hexEdit_);

    const qreal v = initial.valueF();

    wheel_->setColor(initial);
    valueSlider_->setValue(static_cast<int>(v * 100));
    updateHexDisplay(initial);

    connect(wheel_, &ColorWheelWidget::colorChanged, this, [this](const QColor& c) {
        const int value = static_cast<int>(c.valueF() * 100.0);
        if (value != valueSlider_->value()) {
            valueSlider_->blockSignals(true);
            valueSlider_->setValue(value);
            valueSlider_->blockSignals(false);
        }
        updateHexDisplay(c);
    });

    connect(valueSlider_, &QSlider::valueChanged, this, [this](int val) {
        QColor c = wheel_->color();
        c.setHsvF(c.hueF(), c.saturationF(), val / 100.0);
        wheel_->setColor(c);
        updateHexDisplay(c);
    });

    connect(hexEdit_, &QLineEdit::editingFinished, this, [this]() {
        QString text = hexEdit_->text().trimmed();
        if (!text.startsWith('#')) {
            text.prepend('#');
        }
        const QColor parsed(text);
        if (!parsed.isValid()) {
            updateHexDisplay(wheel_->color());
            return;
        }
        wheel_->setColor(parsed);
        updateHexDisplay(parsed);
    });

    auto* btns = new QHBoxLayout();
    btns->addStretch();

    auto* btnCancel = new QPushButton(tr("Cancel"));
    auto* btnOk = new QPushButton(tr("OK"));

    btns->addWidget(btnCancel);
    btns->addWidget(btnOk);
    main->addLayout(btns);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
}

QColor ColorPickerDialog::color() const {
    return wheel_->color();
}

void ColorPickerDialog::updateHexDisplay(const QColor& c) {
    const QString hex = c.name(QColor::HexRgb).toUpper();
    const QColor text = idealTextColor(c);

    hexLabel_->setText(hex);
    hexEdit_->setText(hex);
    hexLabel_->setStyleSheet(QString(
                                   "QLabel {"
                                   " background-color: %1;"
                                   " color: %2;"
                                   " border: 1px solid #444;"
                                   " border-radius: 4px;"
                                   " font-family: monospace;"
                                   " font-size: 14px;"
                                   " font-weight: bold;"
                                   "}")
                               .arg(c.name(), text.name()));
}
