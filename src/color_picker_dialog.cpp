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
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>

#include <yaml-cpp/yaml.h>

namespace {

struct NamedColor {
    QString theme;
    QString name;
    QColor color;
};

QColor idealTextColor(const QColor& bg) {
    const double luminance =
        0.299 * bg.redF() +
        0.587 * bg.greenF() +
        0.114 * bg.blueF();
    return luminance > 0.5 ? Qt::black : Qt::white;
}

std::vector<NamedColor> loadSolarizedSwatches() {
    std::vector<NamedColor> swatches;

    QFile file(QStringLiteral(":/palettes/solarized-swatches.yaml"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return swatches;
    }

    const QByteArray bytes = file.readAll();
    YAML::Node root = YAML::Load(bytes.toStdString());
    const YAML::Node themes = root["themes"];
    if (!themes || !themes.IsSequence()) {
        return swatches;
    }

    for (std::size_t i = 0; i < themes.size(); ++i) {
        const YAML::Node theme_node = themes[i];
        if (!theme_node || !theme_node.IsMap() || !theme_node["name"] || !theme_node["colors"]) {
            continue;
        }

        const QString theme_name = QString::fromStdString(theme_node["name"].as<std::string>());
        const YAML::Node colors = theme_node["colors"];
        if (!colors.IsSequence()) {
            continue;
        }

        for (std::size_t j = 0; j < colors.size(); ++j) {
            const YAML::Node color_node = colors[j];
            if (!color_node || !color_node.IsMap() || !color_node["name"] || !color_node["hex"]) {
                continue;
            }

            const QString color_name = QString::fromStdString(color_node["name"].as<std::string>());
            const QColor color(QString::fromStdString(color_node["hex"].as<std::string>()));
            if (!color.isValid()) {
                continue;
            }

            swatches.push_back(NamedColor{theme_name, color_name, color});
        }
    }

    return swatches;
}

}  // namespace

ColorPickerDialog::ColorPickerDialog(const QColor& initial, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Select color"));
    setModal(true);

    auto* main = new QVBoxLayout(this);

    auto* split = new QHBoxLayout();
    split->setAlignment(Qt::AlignTop);

    auto* left_panel = new QVBoxLayout();
    left_panel->setContentsMargins(0, 0, 0, 0);

    auto* center = new QHBoxLayout();
    center->setAlignment(Qt::AlignTop);

    wheel_ = new ColorWheelWidget(this);
    wheel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    center->addWidget(wheel_, 0, Qt::AlignTop);

    valueSlider_ = new QSlider(Qt::Vertical, this);
    valueSlider_->setRange(0, 100);
    valueSlider_->setFixedWidth(22);
    valueSlider_->setFixedHeight(wheel_->sizeHint().height());
    center->addWidget(valueSlider_, 0, Qt::AlignTop);
    left_panel->addLayout(center);

    hexLabel_ = new QLabel(this);
    hexLabel_->setAlignment(Qt::AlignCenter);
    hexLabel_->setMinimumHeight(36);
    left_panel->addWidget(hexLabel_);

    hexEdit_ = new QLineEdit(this);
    hexEdit_->setPlaceholderText("#RRGGBB");
    left_panel->addWidget(hexEdit_);

    split->addLayout(left_panel, 1);

    auto* swatches_container = new QVBoxLayout();
    swatches_container->setContentsMargins(0, 0, 0, 0);
    buildSwatches(swatches_container);
    split->addLayout(swatches_container, 0);

    main->addLayout(split, 1);

    applyColor(initial);

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
        applyColor(parsed);
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

    main->setSizeConstraint(QLayout::SetFixedSize);
    adjustSize();
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

void ColorPickerDialog::applyColor(const QColor& c) {
    wheel_->setColor(c);
    valueSlider_->setValue(static_cast<int>(c.valueF() * 100.0));
    updateHexDisplay(c);
}

void ColorPickerDialog::buildSwatches(QVBoxLayout* container_layout) {
    if (container_layout == nullptr) {
        return;
    }

    auto* group_box = new QGroupBox(tr("Swatches"), this);
    auto* group_layout = new QVBoxLayout(group_box);
    group_layout->setContentsMargins(8, 8, 8, 8);

    auto* swatches_widget = new QWidget(group_box);
    auto* swatches_layout = new QVBoxLayout(swatches_widget);
    swatches_layout->setContentsMargins(0, 0, 0, 0);
    swatches_layout->setSpacing(8);

    const std::vector<NamedColor> swatches = loadSolarizedSwatches();
    QString current_theme;
    QGridLayout* current_grid = nullptr;
    int swatch_index = 0;

    for (const NamedColor& named_color : swatches) {
        if (named_color.theme != current_theme) {
            current_theme = named_color.theme;
            auto* group = new QLabel(current_theme, swatches_widget);
            swatches_layout->addWidget(group);

            auto* grid = new QGridLayout();
            grid->setHorizontalSpacing(4);
            grid->setVerticalSpacing(4);
            swatches_layout->addLayout(grid);
            current_grid = grid;
            swatch_index = 0;
        }

        if (current_grid == nullptr) {
            continue;
        }

        auto* button = new QPushButton(swatches_widget);
        button->setFixedSize(20, 20);
        button->setToolTip(QString("%1 · %2\n%3")
                               .arg(named_color.theme, named_color.name, named_color.color.name(QColor::HexRgb).toUpper()));
        button->setStyleSheet(QString(
                                  "QPushButton {"
                                  " background-color: %1;"
                                  " border: 1px solid #666;"
                                  " border-radius: 3px;"
                                  "}"
                                  "QPushButton:hover {"
                                  " border: 2px solid #222;"
                                  "}")
                                  .arg(named_color.color.name()));

        connect(button, &QPushButton::clicked, this, [this, named_color]() {
            applyColor(named_color.color);
        });

        const int row = swatch_index / 8;
        const int col = swatch_index % 8;
        current_grid->addWidget(button, row, col);
        ++swatch_index;
    }

    auto* scroll = new QScrollArea(group_box);
    scroll->setWidget(swatches_widget);
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(220);
    scroll->setMaximumWidth(240);
    scroll->setFrameShape(QFrame::NoFrame);
    group_layout->addWidget(scroll);

    container_layout->addWidget(group_box, 0, Qt::AlignTop);
}
