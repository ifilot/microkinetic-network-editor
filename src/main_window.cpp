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

#include "main_window.h"

#include <QAction>
#include <QDockWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "color_picker_dialog.h"
#include "config.h"
#include "log_window.h"
#include "logging.h"
#include "network_io.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QString("%1 %2").arg(kProgramName, kProgramVersion));
    setWindowIcon(QIcon(QStringLiteral(":/icons/mng-icon.png")));
    resize(1200, 760);

    view_ = new NetworkView(this);
    setCentralWidget(view_);
    connect(view_, &NetworkView::selected_node_changed, this, &MainWindow::on_selected_node_changed);

    QSettings settings;
    last_directory_path_ = settings.value("io/last_directory", QString()).toString();

    build_settings_widget();
    build_menus();

    log_window_ = new LogWindow(g_log_messages, this);

    statusBar()->showMessage("Use File > Load YAML to open a network file.");
}

void MainWindow::build_menus() {
    QMenu* file_menu = menuBar()->addMenu("&File");
    QAction* load_action = file_menu->addAction("&Load YAML...");
    load_action->setShortcut(QKeySequence::Open);
    connect(load_action, &QAction::triggered, this, &MainWindow::load_yaml);

    QAction* save_action = file_menu->addAction("&Save YAML...");
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, &MainWindow::save_yaml);

    QAction* save_png_action = file_menu->addAction("Save &PNG...");
    save_png_action->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(save_png_action, &QAction::triggered, this, &MainWindow::save_png);

    file_menu->addSeparator();
    QAction* exit_action = file_menu->addAction("E&xit");
    exit_action->setShortcut(QKeySequence::Quit);
    connect(exit_action, &QAction::triggered, this, &QWidget::close);

    QMenu* settings_menu = menuBar()->addMenu("&Settings");
    if (settings_toggle_action_ != nullptr) {
        settings_menu->addAction(settings_toggle_action_);
    }

    QMenu* help_menu = menuBar()->addMenu("&Help");
    debug_log_action_ = help_menu->addAction("Show &Debug Log");
    connect(debug_log_action_, &QAction::triggered, this, &MainWindow::show_debug_log);

    QAction* about_action = help_menu->addAction("&About");
    connect(about_action, &QAction::triggered, this, &MainWindow::show_about);
}

void MainWindow::build_settings_widget() {
    QDockWidget* settings_dock = new QDockWidget("Settings", this);
    settings_dock->setObjectName("settings-dock");

    QWidget* settings_widget = new QWidget(settings_dock);
    QVBoxLayout* layout = new QVBoxLayout(settings_widget);

    auto* geometry_group = new QGroupBox("Geometry", settings_widget);
    auto* geometry_form = new QFormLayout(geometry_group);
    node_radius_spin_ = new QDoubleSpinBox(settings_widget);
    node_radius_spin_->setRange(6.0, 120.0);
    geometry_form->addRow("Node size", node_radius_spin_);
    connect(node_radius_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_node_size);

    line_thickness_spin_ = new QDoubleSpinBox(settings_widget);
    line_thickness_spin_->setRange(1.0, 30.0);
    line_thickness_spin_->setSingleStep(0.5);
    geometry_form->addRow("Line thickness", line_thickness_spin_);
    connect(line_thickness_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_line_thickness);

    node_outline_spin_ = new QDoubleSpinBox(settings_widget);
    node_outline_spin_->setRange(0.5, 12.0);
    node_outline_spin_->setSingleStep(0.25);
    geometry_form->addRow("Node outline thickness", node_outline_spin_);
    connect(node_outline_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_node_outline_thickness);

    auto* typography_group = new QGroupBox("Typography", settings_widget);
    auto* typography_form = new QFormLayout(typography_group);
    font_family_combo_ = new QFontComboBox(settings_widget);
    typography_form->addRow("Font", font_family_combo_);
    connect(font_family_combo_, &QFontComboBox::currentFontChanged, this, &MainWindow::update_font_family);

    font_size_spin_ = new QDoubleSpinBox(settings_widget);
    font_size_spin_->setRange(6.0, 64.0);
    typography_form->addRow("Font size", font_size_spin_);
    connect(font_size_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_font_size);

    label_angle_spin_ = new QDoubleSpinBox(settings_widget);
    label_angle_spin_->setRange(-180.0, 180.0);
    label_angle_spin_->setSingleStep(5.0);
    typography_form->addRow("Node label angle", label_angle_spin_);
    connect(label_angle_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_label_angle);

    label_distance_spin_ = new QDoubleSpinBox(settings_widget);
    label_distance_spin_->setRange(0.0, 100.0);
    label_distance_spin_->setSingleStep(1.0);
    typography_form->addRow("Node label distance", label_distance_spin_);
    connect(label_distance_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_node_label_distance);

    value_decimals_spin_ = new QSpinBox(settings_widget);
    value_decimals_spin_->setRange(0, 10);
    typography_form->addRow("Value decimals", value_decimals_spin_);
    connect(value_decimals_spin_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::update_value_decimals);

    value_unit_combo_ = new QComboBox(settings_widget);
    value_unit_combo_->addItem("eV");
    value_unit_combo_->addItem("kJ/mol");
    typography_form->addRow("Value unit", value_unit_combo_);
    connect(value_unit_combo_, &QComboBox::currentTextChanged, this, &MainWindow::update_value_unit);

    auto* colors_group = new QGroupBox("Colors", settings_widget);
    auto* colors_form = new QFormLayout(colors_group);

    auto add_color_row = [this, settings_widget, colors_form](const QString& label, QPushButton*& button, QLabel*& hex, const char* slot) {
        auto* row = new QWidget(settings_widget);
        auto* row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        hex = new QLabel("#000000", row);
        button = new QPushButton(row);
        button->setFixedSize(10, 10);
        row_layout->addWidget(hex);
        row_layout->addStretch();
        row_layout->addWidget(button);
        colors_form->addRow(label, row);
        connect(button, SIGNAL(clicked()), this, slot);
    };

    add_color_row("Background", bg_color_button_, bg_color_hex_, SLOT(pick_background_color()));
    add_color_row("Node fill", node_fill_color_button_, node_fill_color_hex_, SLOT(pick_node_fill_color()));
    add_color_row("Node outline", node_outline_color_button_, node_outline_color_hex_, SLOT(pick_node_outline_color()));
    add_color_row("Line color", line_color_button_, line_color_hex_, SLOT(pick_line_color()));
    add_color_row("Label color", label_color_button_, label_color_hex_, SLOT(pick_label_color()));

    auto* selection_group = new QGroupBox("Selection", settings_widget);
    auto* selection_form = new QFormLayout(selection_group);
    selected_node_edit_ = new QLineEdit(settings_widget);
    selected_node_edit_->setEnabled(false);
    selection_form->addRow("Node name", selected_node_edit_);
    connect(selected_node_edit_, &QLineEdit::textEdited, this, &MainWindow::update_selected_node_name);

    reset_node_name_button_ = new QPushButton("Revert to label", settings_widget);
    reset_node_name_button_->setEnabled(false);
    selection_form->addRow("", reset_node_name_button_);
    connect(reset_node_name_button_, &QPushButton::clicked, this, &MainWindow::reset_selected_node_name);

    layout->addWidget(geometry_group);
    layout->addWidget(typography_group);
    layout->addWidget(colors_group);
    layout->addWidget(selection_group);
    layout->addStretch();

    settings_widget->setLayout(layout);
    settings_dock->setWidget(settings_widget);
    addDockWidget(Qt::RightDockWidgetArea, settings_dock);

    settings_toggle_action_ = settings_dock->toggleViewAction();
    settings_toggle_action_->setText("Show &Settings");

    sync_controls_from_view();
}

void MainWindow::load_yaml() {
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Load network YAML",
        initial_dialog_directory(),
        "YAML files (*.yaml *.yml)");

    if (file_path.isEmpty()) {
        return;
    }

    NetworkData data;
    QString error;
    if (!load_network_yaml(file_path, data, error)) {
        QMessageBox::critical(this, "Load failed", "Failed to load YAML:\n" + error);
        return;
    }

    view_->set_network(data);
    if (data.has_settings) {
        view_->apply_settings(data.settings);
    }
    sync_controls_from_view();

    current_file_path_ = file_path;
    remember_dialog_path(file_path);
    statusBar()->showMessage("Loaded: " + file_path, 4000);
}

void MainWindow::save_yaml() {
    QString starting_path = current_file_path_;
    if (starting_path.isEmpty()) {
        const QString base_dir = initial_dialog_directory();
        starting_path = base_dir.isEmpty() ? QStringLiteral("network.yaml") : base_dir + "/network.yaml";
    }
    const QString file_path = QFileDialog::getSaveFileName(
        this,
        "Save network YAML",
        starting_path,
        "YAML files (*.yaml *.yml)");

    if (file_path.isEmpty()) {
        return;
    }

    QString error;
    NetworkData data = view_->network();
    data.settings = view_->current_settings();
    data.has_settings = true;
    if (!save_network_yaml(file_path, data, error)) {
        QMessageBox::critical(this, "Save failed", "Failed to save YAML:\n" + error);
        return;
    }

    current_file_path_ = file_path;
    remember_dialog_path(file_path);
    statusBar()->showMessage("Saved: " + file_path, 4000);
}

void MainWindow::save_png() {
    const QString file_path = QFileDialog::getSaveFileName(this,
                                                           "Save PNG",
                                                           initial_dialog_directory().isEmpty() ? "network.png" : initial_dialog_directory() + "/network.png",
                                                           "PNG files (*.png)");
    if (file_path.isEmpty()) {
        return;
    }

    QString error;
    if (!view_->save_view_to_png(file_path, error)) {
        QMessageBox::critical(this, "Save PNG failed", error);
        return;
    }
    remember_dialog_path(file_path);
    statusBar()->showMessage("PNG saved: " + file_path, 4000);
}

void MainWindow::show_about() {
    QMessageBox::about(this,
                       QString("About %1").arg(kProgramName),
                       QString("%1\nVersion %2\n\n"
                               "Author: Ivo Filot <i.a.w.filot@tue.nl>\n"
                               "Maintainer: Ivo Filot <i.a.w.filot@tue.nl>\n\n"
                               "Acknowledgements:\n"
                               "- María Presa Zubillaga\n"
                               "- Bart Zijlstra\n"
                               "- Min Zhang\n"
                               "- Robin Broos\n"
                               "- Xianxuan Ren\n\n"
                               "License:\n%3\n\n"
                               "Third-party dependencies:\n"
                               "- Qt (Qt Widgets / QOpenGLWidget). Qt is available under LGPLv3/GPL/commercial terms; "
                               "redistribution must comply with the selected Qt license terms.\n"
                               "- yaml-cpp (YAML parser library).")
                           .arg(kProgramName, kProgramVersion, kLicenseText));
}

void MainWindow::show_debug_log() {
    if (log_window_ == nullptr) {
        return;
    }

    log_window_->show();
    log_window_->raise();
    log_window_->activateWindow();
}

void MainWindow::update_node_size(double value) { view_->set_node_radius(static_cast<float>(value)); }

void MainWindow::update_line_thickness(double value) { view_->set_line_thickness(static_cast<float>(value)); }

void MainWindow::update_node_outline_thickness(double value) {
    view_->set_node_outline_thickness(static_cast<float>(value));
}

void MainWindow::update_font_size(double value) { view_->set_font_size(static_cast<float>(value)); }

void MainWindow::update_label_angle(double value) { view_->set_label_angle_degrees(static_cast<float>(value)); }

void MainWindow::update_font_family(const QFont& font) { view_->set_font_family(font.family()); }

void MainWindow::update_node_label_distance(double value) { view_->set_node_label_distance(static_cast<float>(value)); }

void MainWindow::update_value_decimals(int value) { view_->set_value_decimals(value); }

void MainWindow::update_value_unit(const QString& unit) { view_->set_value_unit(unit); }

void MainWindow::update_selected_node_name(const QString& label) { view_->set_selected_node_name(label); }

void MainWindow::reset_selected_node_name() { view_->reset_selected_node_name(); }

void MainWindow::on_selected_node_changed(int index, const QString& label) {
    const QSignalBlocker blocker(selected_node_edit_);
    selected_node_edit_->setEnabled(index >= 0);
    reset_node_name_button_->setEnabled(index >= 0);
    selected_node_edit_->setText(label);
}

void MainWindow::pick_background_color() {
    ColorPickerDialog dialog(view_->background_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_background_color(dialog.color());
    set_color_chip(bg_color_button_, bg_color_hex_, dialog.color());
}

void MainWindow::pick_node_fill_color() {
    ColorPickerDialog dialog(view_->node_fill_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_node_fill_color(dialog.color());
    set_color_chip(node_fill_color_button_, node_fill_color_hex_, dialog.color());
}

void MainWindow::pick_node_outline_color() {
    ColorPickerDialog dialog(view_->node_outline_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_node_outline_color(dialog.color());
    set_color_chip(node_outline_color_button_, node_outline_color_hex_, dialog.color());
}

void MainWindow::pick_line_color() {
    ColorPickerDialog dialog(view_->line_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_line_color(dialog.color());
    set_color_chip(line_color_button_, line_color_hex_, dialog.color());
}

void MainWindow::pick_label_color() {
    ColorPickerDialog dialog(view_->label_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_label_color(dialog.color());
    set_color_chip(label_color_button_, label_color_hex_, dialog.color());
}

QString MainWindow::initial_dialog_directory() const {
    if (!current_file_path_.isEmpty()) {
        return QFileInfo(current_file_path_).absolutePath();
    }
    if (!last_directory_path_.isEmpty()) {
        return last_directory_path_;
    }
    return QString();
}

void MainWindow::remember_dialog_path(const QString& file_path) {
    QFileInfo info(file_path);
    last_directory_path_ = info.absolutePath();
    QSettings settings;
    settings.setValue("io/last_directory", last_directory_path_);
}

void MainWindow::set_color_chip(QPushButton* button, QLabel* hex_label, const QColor& color) const {
    if (button == nullptr || hex_label == nullptr) {
        return;
    }
    hex_label->setText(color.name(QColor::HexRgb).toUpper());
    button->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #555; }").arg(color.name()));
}

void MainWindow::sync_controls_from_view() {
    if (!node_radius_spin_) {
        return;
    }

    node_radius_spin_->setValue(view_->node_radius());
    line_thickness_spin_->setValue(view_->line_thickness());
    node_outline_spin_->setValue(view_->node_outline_thickness());
    font_size_spin_->setValue(view_->font_size());
    label_angle_spin_->setValue(view_->label_angle_degrees());
    label_distance_spin_->setValue(view_->node_label_distance());
    font_family_combo_->setCurrentFont(QFont(view_->font_family()));
    value_decimals_spin_->setValue(view_->value_decimals());
    const int unit_index = value_unit_combo_->findText(view_->value_unit());
    value_unit_combo_->setCurrentIndex(unit_index >= 0 ? unit_index : 0);

    set_color_chip(bg_color_button_, bg_color_hex_, view_->background_color());
    set_color_chip(node_fill_color_button_, node_fill_color_hex_, view_->node_fill_color());
    set_color_chip(node_outline_color_button_, node_outline_color_hex_, view_->node_outline_color());
    set_color_chip(line_color_button_, line_color_hex_, view_->line_color());
    set_color_chip(label_color_button_, label_color_hex_, view_->label_color());
}
