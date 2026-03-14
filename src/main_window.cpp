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

#include "main_window.h"

#include <QAction>
#include <QAbstractItemView>
#include <QDockWidget>
#include <QDir>
#include <QDialog>
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
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QHash>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <exception>

#include "color_picker_dialog.h"
#include "config.h"
#include "log_window.h"
#include "logging.h"
#include "network_io.h"
#include "structures/anaglyph_widget.h"
#include "structures/structure_loader.h"

namespace {
QHash<QString, std::shared_ptr<Structure>> g_loaded_structure_cache;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    update_window_title();
    setWindowIcon(QIcon(QStringLiteral(":/icons/mng-icon.png")));
    resize(1200, 760);

    view_ = new NetworkView(this);
    setCentralWidget(view_);
    connect(view_, &NetworkView::selection_changed, this, &MainWindow::on_selection_changed);

    QSettings settings;
    last_directory_path_ = settings.value("io/last_directory", QString()).toString();

    build_properties_widget();
    build_menus();

    log_window_ = new LogWindow(g_log_messages, this);

    statusBar()->showMessage("Use File > Load YAML to open a network file.");
}

void MainWindow::build_menus() {
    QMenu* file_menu = menuBar()->addMenu("&File");
    QAction* load_action = file_menu->addAction("&Load YAML...");
    load_action->setShortcut(QKeySequence::Open);
    connect(load_action, &QAction::triggered, this, &MainWindow::load_yaml);

    QAction* save_action = file_menu->addAction("&Save YAML");
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, &MainWindow::save_yaml);

    QAction* save_as_action = file_menu->addAction("Save YAML &As...");
    save_as_action->setShortcut(QKeySequence::SaveAs);
    connect(save_as_action, &QAction::triggered, this, &MainWindow::save_yaml_as);

    QAction* save_png_action = file_menu->addAction("Save &PNG...");
    save_png_action->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(save_png_action, &QAction::triggered, this, &MainWindow::save_png);

    file_menu->addSeparator();
    QAction* exit_action = file_menu->addAction("E&xit");
    exit_action->setShortcut(QKeySequence::Quit);
    connect(exit_action, &QAction::triggered, this, &QWidget::close);

    QMenu* properties_menu = menuBar()->addMenu("&Properties");
    if (properties_toggle_action_ != nullptr) {
        properties_menu->addAction(properties_toggle_action_);
    }
    if (yaml_source_toggle_action_ != nullptr) {
        properties_menu->addAction(yaml_source_toggle_action_);
    }

    QMenu* help_menu = menuBar()->addMenu("&Help");
    debug_log_action_ = help_menu->addAction("Show &Debug Log");
    connect(debug_log_action_, &QAction::triggered, this, &MainWindow::show_debug_log);

    QAction* about_action = help_menu->addAction("&About");
    connect(about_action, &QAction::triggered, this, &MainWindow::show_about);
}

void MainWindow::build_properties_widget() {
    QDockWidget* properties_dock = new QDockWidget("Properties", this);
    properties_dock->setObjectName("properties-dock");

    QWidget* properties_widget = new QWidget(properties_dock);
    QVBoxLayout* layout = new QVBoxLayout(properties_widget);

    auto* geometry_group = new QGroupBox("Geometry", properties_widget);
    auto* geometry_form = new QFormLayout(geometry_group);
    node_radius_spin_ = new QDoubleSpinBox(properties_widget);
    node_radius_spin_->setRange(6.0, 120.0);
    geometry_form->addRow("Node size", node_radius_spin_);
    connect(node_radius_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_node_size);

    line_thickness_spin_ = new QDoubleSpinBox(properties_widget);
    line_thickness_spin_->setRange(1.0, 30.0);
    line_thickness_spin_->setSingleStep(0.5);
    geometry_form->addRow("Line thickness", line_thickness_spin_);
    connect(line_thickness_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_line_thickness);

    node_outline_spin_ = new QDoubleSpinBox(properties_widget);
    node_outline_spin_->setRange(0.5, 12.0);
    node_outline_spin_->setSingleStep(0.25);
    geometry_form->addRow("Node outline thickness", node_outline_spin_);
    connect(node_outline_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_node_outline_thickness);

    auto* typography_group = new QGroupBox("Typography", properties_widget);
    auto* typography_form = new QFormLayout(typography_group);
    font_family_combo_ = new QFontComboBox(properties_widget);
    typography_form->addRow("Font", font_family_combo_);
    connect(font_family_combo_, &QFontComboBox::currentFontChanged, this, &MainWindow::update_font_family);

    font_size_spin_ = new QDoubleSpinBox(properties_widget);
    font_size_spin_->setRange(6.0, 64.0);
    typography_form->addRow("Font size", font_size_spin_);
    connect(font_size_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_font_size);

    label_angle_spin_ = new QDoubleSpinBox(properties_widget);
    label_angle_spin_->setRange(-180.0, 180.0);
    label_angle_spin_->setSingleStep(5.0);
    typography_form->addRow("Node label angle", label_angle_spin_);
    connect(label_angle_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_label_angle);

    label_distance_spin_ = new QDoubleSpinBox(properties_widget);
    label_distance_spin_->setRange(0.0, 100.0);
    label_distance_spin_->setSingleStep(1.0);
    typography_form->addRow("Node label distance", label_distance_spin_);
    connect(label_distance_spin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::update_node_label_distance);

    value_decimals_spin_ = new QSpinBox(properties_widget);
    value_decimals_spin_->setRange(0, 10);
    typography_form->addRow("Value decimals", value_decimals_spin_);
    connect(value_decimals_spin_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::update_value_decimals);

    value_unit_combo_ = new QComboBox(properties_widget);
    value_unit_combo_->addItem("eV");
    value_unit_combo_->addItem("kJ/mol");
    typography_form->addRow("Value unit", value_unit_combo_);
    connect(value_unit_combo_, &QComboBox::currentTextChanged, this, &MainWindow::update_value_unit);

    auto* colors_group = new QGroupBox("Colors", properties_widget);
    auto* colors_form = new QFormLayout(colors_group);

    auto add_color_row = [this, properties_widget, colors_form](const QString& label, QPushButton*& button, QLabel*& hex, const char* slot) {
        auto* row = new QWidget(properties_widget);
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
    add_color_row("Label color", label_color_button_, label_color_hex_, SLOT(pick_label_color()));

    auto* selection_group = new QGroupBox("Selection", properties_widget);
    selection_form_ = new QFormLayout(selection_group);

    selected_type_value_ = new QLabel("None", properties_widget);
    selection_form_->addRow("Selected", selected_type_value_);

    selected_node_edit_ = new QLineEdit(properties_widget);
    selected_node_name_row_ = new QWidget(properties_widget);
    {
        auto* row_layout = new QHBoxLayout(selected_node_name_row_);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->addWidget(selected_node_edit_);
        reset_node_name_button_ = new QPushButton("Revert to label", properties_widget);
        row_layout->addWidget(reset_node_name_button_);
    }
    selection_form_->addRow("Node name", selected_node_name_row_);
    connect(selected_node_edit_, &QLineEdit::textEdited, this, &MainWindow::update_selected_node_name);
    connect(reset_node_name_button_, &QPushButton::clicked, this, &MainWindow::reset_selected_node_name);

    auto add_selection_color_row = [this, properties_widget](const QString& label, QPushButton*& button, QLabel*& hex_label, QWidget*& row_widget, const char* slot) {
        row_widget = new QWidget(properties_widget);
        auto* row_layout = new QHBoxLayout(row_widget);
        row_layout->setContentsMargins(0, 0, 0, 0);
        hex_label = new QLabel("#000000", row_widget);
        button = new QPushButton(row_widget);
        button->setFixedSize(10, 10);
        row_layout->addWidget(hex_label);
        row_layout->addStretch();
        row_layout->addWidget(button);
        selection_form_->addRow(label, row_widget);
        connect(button, SIGNAL(clicked()), this, slot);
    };

    add_selection_color_row("Edge color", edge_color_button_, edge_color_hex_, edge_color_row_, SLOT(pick_edge_color()));
    add_selection_color_row("Node fill", selection_node_fill_color_button_, selection_node_fill_color_hex_, selected_node_fill_row_, SLOT(pick_selection_node_fill_color()));
    add_selection_color_row("Node outline", selection_node_outline_color_button_, selection_node_outline_color_hex_, selected_node_outline_row_, SLOT(pick_selection_node_outline_color()));

    edge_swap_labels_row_ = new QWidget(properties_widget);
    {
        auto* row_layout = new QHBoxLayout(edge_swap_labels_row_);
        row_layout->setContentsMargins(0, 0, 0, 0);
        swap_edge_labels_button_ = new QPushButton("Swap edge label sides", properties_widget);
        swap_edge_labels_button_->setCheckable(true);
        row_layout->addWidget(swap_edge_labels_button_);
    }
    selection_form_->addRow("", edge_swap_labels_row_);
    connect(swap_edge_labels_button_, &QPushButton::toggled, this, &MainWindow::toggle_selected_edge_swap_labels);

    edge_segments_table_ = new QTableWidget(properties_widget);
    edge_segments_table_->setColumnCount(2);
    edge_segments_table_->setHorizontalHeaderLabels(QStringList{"Segment", "Shape"});
    edge_segments_table_->horizontalHeader()->setStretchLastSection(true);
    edge_segments_table_->verticalHeader()->setVisible(false);
    edge_segments_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    edge_segments_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    edge_segments_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    edge_segments_table_->setEnabled(false);
    edge_segments_table_->setMinimumHeight(130);
    edge_segments_row_ = edge_segments_table_;
    selection_form_->addRow("Segments", edge_segments_table_);
    connect(edge_segments_table_, &QTableWidget::currentCellChanged, this, &MainWindow::on_edge_segment_selected);

    edge_label_segment_combo_ = new QComboBox(properties_widget);
    edge_label_segment_row_ = edge_label_segment_combo_;
    selection_form_->addRow("Label segment", edge_label_segment_combo_);
    connect(edge_label_segment_combo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::on_edge_label_segment_changed);

    add_guide_node_button_ = new QPushButton("Add guide node", properties_widget);
    add_guide_node_button_->setEnabled(false);
    add_guide_node_row_ = add_guide_node_button_;
    selection_form_->addRow("", add_guide_node_button_);
    connect(add_guide_node_button_, &QPushButton::clicked, this, &MainWindow::add_selected_edge_guide_node);

    remove_guide_node_button_ = new QPushButton("Remove guide node", properties_widget);
    remove_guide_node_button_->setEnabled(false);
    remove_guide_node_row_ = remove_guide_node_button_;
    selection_form_->addRow("", remove_guide_node_button_);
    connect(remove_guide_node_button_, &QPushButton::clicked, this, &MainWindow::remove_selected_edge_guide_node);

    selected_structure_row_ = new QGroupBox("Structure", properties_widget);
    {
        auto* structure_layout = new QVBoxLayout(selected_structure_row_);
        selected_structure_widget_ = new AnaglyphWidget(selected_structure_row_);
        selected_structure_widget_->setMinimumHeight(220);
        structure_layout->addWidget(selected_structure_widget_);
    }

    layout->addWidget(geometry_group);
    layout->addWidget(typography_group);
    layout->addWidget(colors_group);
    layout->addWidget(selection_group);
    layout->addWidget(selected_structure_row_);

    auto* design_errors_row = new QWidget(properties_widget);
    auto* design_errors_layout = new QHBoxLayout(design_errors_row);
    design_errors_layout->setContentsMargins(0, 0, 0, 0);
    design_errors_label_ = new QLabel("0 errors in design found.", design_errors_row);
    design_errors_button_ = new QPushButton("Show", design_errors_row);
    design_errors_layout->addWidget(design_errors_label_);
    design_errors_layout->addStretch();
    design_errors_layout->addWidget(design_errors_button_);
    layout->addWidget(design_errors_row);
    connect(design_errors_button_, &QPushButton::clicked, this, &MainWindow::show_design_errors_dialog);

    layout->addStretch();

    properties_widget->setLayout(layout);
    properties_dock->setWidget(properties_widget);
    addDockWidget(Qt::RightDockWidgetArea, properties_dock);

    properties_toggle_action_ = properties_dock->toggleViewAction();
    properties_toggle_action_->setText("Show &Properties");

    auto* yaml_source_dock = new QDockWidget("YAML Source", this);
    yaml_source_dock->setObjectName("yaml-source-dock");
    yaml_source_view_ = new QPlainTextEdit(yaml_source_dock);
    yaml_source_view_->setReadOnly(true);
    yaml_source_view_->setLineWrapMode(QPlainTextEdit::NoWrap);
    yaml_source_dock->setWidget(yaml_source_view_);
    addDockWidget(Qt::RightDockWidgetArea, yaml_source_dock);
    yaml_source_toggle_action_ = yaml_source_dock->toggleViewAction();
    yaml_source_toggle_action_->setText("Show &YAML Source");
    yaml_source_dock->hide();

    sync_controls_from_view();
    refresh_yaml_source_widget();
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
        if (error.contains("contains more than two nodes")) {
            QMessageBox unsupported_edge_modal(this);
            unsupported_edge_modal.setIcon(QMessageBox::Critical);
            unsupported_edge_modal.setWindowTitle("Unsupported edge in YAML");
            unsupported_edge_modal.setText("This YAML file contains an edge with more than two nodes.");
            unsupported_edge_modal.setInformativeText(error + "\nPlease split multi-node edges into pairwise edges.");
            unsupported_edge_modal.exec();
        } else {
            QMessageBox::critical(this, "Load failed", "Failed to load YAML:\n" + error);
        }
        return;
    }

    view_->set_network(data);
    if (data.has_settings) {
        view_->apply_settings(data.settings);
    }
    sync_controls_from_view();

    current_file_path_ = file_path;
    loaded_network_directory_ = QFileInfo(file_path).absolutePath();
    remember_dialog_path(file_path);
    statusBar()->showMessage("Loaded: " + file_path, 4000);
    update_window_title();
    refresh_yaml_source_widget();
}

void MainWindow::save_yaml() {
    if (current_file_path_.isEmpty()) {
        save_yaml_as();
        return;
    }

    QString error;
    NetworkData data = view_->network();
    data.settings = view_->current_settings();
    data.has_settings = true;
    if (!save_network_yaml(current_file_path_, data, error)) {
        QMessageBox::critical(this, "Save failed", "Failed to save YAML:\n" + error);
        return;
    }

    loaded_network_directory_ = QFileInfo(current_file_path_).absolutePath();
    remember_dialog_path(current_file_path_);
    statusBar()->showMessage("Saved: " + current_file_path_, 4000);
    update_window_title();
    refresh_yaml_source_widget();
}

void MainWindow::save_yaml_as() {
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
    loaded_network_directory_ = QFileInfo(file_path).absolutePath();
    remember_dialog_path(file_path);
    statusBar()->showMessage("Saved: " + file_path, 4000);
    update_window_title();
    refresh_yaml_source_widget();
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
    const QString license_html = QString::fromUtf8(kLicenseText).toHtmlEscaped().replace("\n", "<br>");

    QMessageBox::about(this,
                       QString("About %1").arg(kProgramName),
                       QString("<h3>%1</h3>"
                               "<p>Version %2</p>"
                               "<p>Author: Ivo Filot &lt;<a href='mailto:i.a.w.filot@tue.nl'>i.a.w.filot@tue.nl</a>&gt;<br>"
                               "Maintainer: Ivo Filot &lt;<a href='mailto:i.a.w.filot@tue.nl'>i.a.w.filot@tue.nl</a>&gt;</p>"
                               "<p><b>Acknowledgements:</b><br>"
                               "- María Presa Zubillaga<br>"
                               "- Bart Zijlstra<br>"
                               "- Min Zhang<br>"
                               "- Robin Broos<br>"
                               "- Xianxuan Ren<br>"
                               "- Joeri van Limpt</p>"
                               "<p><b>License:</b><br>%3</p>"
                               "<p><b>Third-party dependencies:</b><br>"
                               "- Qt (Qt Widgets / QOpenGLWidget). Qt is available under LGPLv3/GPL/commercial terms; "
                               "redistribution must comply with the selected Qt license terms. "
                               "Source code: <a href='https://code.qt.io/cgit/qt/'>https://code.qt.io/cgit/qt/</a><br>"
                               "- yaml-cpp (YAML parser library). "
                               "Source code: <a href='https://github.com/jbeder/yaml-cpp'>https://github.com/jbeder/yaml-cpp</a></p>"
                               "<p><b>Project repository:</b> "
                               "<a href='https://github.com/ifilot/microkinetic-network-editor'>"
                               "https://github.com/ifilot/microkinetic-network-editor</a></p>")
                           .arg(kProgramName, kProgramVersion, license_html));
}

void MainWindow::show_debug_log() {
    if (log_window_ == nullptr) {
        return;
    }

    log_window_->show();
    log_window_->raise();
    log_window_->activateWindow();
}

void MainWindow::update_node_size(double value) { view_->set_node_radius(static_cast<float>(value)); refresh_yaml_source_widget(); }

void MainWindow::update_line_thickness(double value) { view_->set_line_thickness(static_cast<float>(value)); refresh_yaml_source_widget(); }

void MainWindow::update_node_outline_thickness(double value) {
    view_->set_node_outline_thickness(static_cast<float>(value));
    refresh_yaml_source_widget();
}

void MainWindow::update_font_size(double value) { view_->set_font_size(static_cast<float>(value)); refresh_yaml_source_widget(); }

void MainWindow::update_label_angle(double value) { view_->set_label_angle_degrees(static_cast<float>(value)); refresh_yaml_source_widget(); }

void MainWindow::update_font_family(const QFont& font) { view_->set_font_family(font.family()); refresh_yaml_source_widget(); }

void MainWindow::update_node_label_distance(double value) { view_->set_node_label_distance(static_cast<float>(value)); refresh_yaml_source_widget(); }

void MainWindow::update_value_decimals(int value) { view_->set_value_decimals(value); refresh_yaml_source_widget(); }

void MainWindow::update_value_unit(const QString& unit) { view_->set_value_unit(unit); refresh_yaml_source_widget(); }

void MainWindow::update_selected_node_name(const QString& label) { view_->set_selected_node_name(label); refresh_yaml_source_widget(); }

void MainWindow::reset_selected_node_name() { view_->reset_selected_node_name(); refresh_yaml_source_widget(); }

void MainWindow::toggle_selected_edge_swap_labels(bool checked) { view_->set_selected_edge_swap_label_sides(checked); refresh_yaml_source_widget(); }

void MainWindow::on_edge_segment_selected(int current_row, int, int, int) {
    Q_UNUSED(current_row);
    // Segment row clicks should only control segment-shape editing focus,
    // not the label placement segment.
}

void MainWindow::on_edge_label_segment_changed(int index) {
    if (index < 0) {
        return;
    }
    view_->set_selected_edge_label_segment_index(index);
    refresh_yaml_source_widget();
}

void MainWindow::add_selected_edge_guide_node() { view_->add_selected_edge_guide_node(); refresh_yaml_source_widget(); }

void MainWindow::remove_selected_edge_guide_node() { view_->remove_selected_edge_guide_node(); refresh_yaml_source_widget(); }

void MainWindow::refresh_edge_segments_table() {
    const bool edge_selected = view_->has_edge_selection();
    const int segment_count = edge_selected ? view_->selected_edge_segment_count() : 0;

    edge_segments_table_->setEnabled(edge_selected && segment_count > 0);
    edge_label_segment_combo_->setEnabled(edge_selected && segment_count > 0);
    edge_segments_table_->setRowCount(segment_count);

    for (int row = 0; row < segment_count; ++row) {
        auto* segment_item = new QTableWidgetItem(QString("Segment %1").arg(row + 1));
        edge_segments_table_->setItem(row, 0, segment_item);

        auto* shape_combo = new QComboBox(edge_segments_table_);
        shape_combo->addItem("Straight");
        shape_combo->addItem("90° bend (clockwise)");
        shape_combo->addItem("90° bend (counterclockwise)");
        shape_combo->addItem("Wiggle (horizontal first)");
        shape_combo->addItem("Wiggle (vertical first)");
        shape_combo->setCurrentIndex(static_cast<int>(view_->selected_edge_segment_kind_at(row)));
        connect(shape_combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, row](int index) {
            if (index < 0) {
                return;
            }
            view_->set_selected_edge_segment_kind_at(row, static_cast<NetworkView::SegmentKindUi>(index));
            refresh_yaml_source_widget();
        });
        edge_segments_table_->setCellWidget(row, 1, shape_combo);
    }

    {
        const QSignalBlocker blocker(edge_label_segment_combo_);
        edge_label_segment_combo_->clear();
        for (int row = 0; row < segment_count; ++row) {
            edge_label_segment_combo_->addItem(QString("Segment %1").arg(row + 1));
        }
        if (segment_count > 0) {
            edge_label_segment_combo_->setCurrentIndex(std::clamp(view_->selected_edge_label_segment_index(), 0, segment_count - 1));
        }
    }

    if (segment_count > 0) {
        const int active_row = std::clamp(view_->selected_edge_label_segment_index(), 0, segment_count - 1);
        const QSignalBlocker blocker(edge_segments_table_);
        edge_segments_table_->setCurrentCell(active_row, 0);
    }
}

void MainWindow::on_selection_changed() {
    const bool node_selected = view_->has_node_selection();
    const bool edge_selected = view_->has_edge_selection();

    QWidget* const properties_root = selected_structure_row_ != nullptr ? selected_structure_row_->parentWidget() : nullptr;
    if (properties_root != nullptr) {
        properties_root->setUpdatesEnabled(false);
    }

    selected_type_value_->setText(view_->selected_item_type().isEmpty() ? "None" :
                                  QString("%1: %2").arg(view_->selected_item_type(), view_->selected_item_label()));

    {
        const QSignalBlocker blocker(selected_node_edit_);
        selected_node_edit_->setText(node_selected ? view_->selected_node_name() : QString());
    }
    selected_node_edit_->setEnabled(node_selected);
    reset_node_name_button_->setEnabled(node_selected);

    selected_node_name_row_->setVisible(node_selected);
    if (QWidget* label = selection_form_->labelForField(selected_node_name_row_)) {
        label->setVisible(node_selected);
    }

    edge_color_row_->setVisible(edge_selected);
    if (QWidget* label = selection_form_->labelForField(edge_color_row_)) {
        label->setVisible(edge_selected);
    }
    if (edge_selected) {
        set_color_chip(edge_color_button_, edge_color_hex_, view_->selected_item_color());
    }

    selected_node_fill_row_->setVisible(node_selected);
    if (QWidget* label = selection_form_->labelForField(selected_node_fill_row_)) {
        label->setVisible(node_selected);
    }
    if (node_selected) {
        set_color_chip(selection_node_fill_color_button_, selection_node_fill_color_hex_, view_->selected_node_fill_color());
    }

    selected_node_outline_row_->setVisible(node_selected);
    if (QWidget* label = selection_form_->labelForField(selected_node_outline_row_)) {
        label->setVisible(node_selected);
    }
    if (node_selected) {
        set_color_chip(selection_node_outline_color_button_, selection_node_outline_color_hex_, view_->selected_node_outline_color());
    }

    edge_swap_labels_row_->setVisible(edge_selected);
    if (QWidget* label = selection_form_->labelForField(edge_swap_labels_row_)) {
        label->setVisible(edge_selected);
    }
    {
        const QSignalBlocker blocker(swap_edge_labels_button_);
        swap_edge_labels_button_->setChecked(edge_selected ? view_->selected_edge_swap_label_sides() : false);
    }

    edge_segments_row_->setVisible(edge_selected);
    if (QWidget* label = selection_form_->labelForField(edge_segments_row_)) {
        label->setVisible(edge_selected);
    }
    edge_label_segment_row_->setVisible(edge_selected);
    if (QWidget* label = selection_form_->labelForField(edge_label_segment_row_)) {
        label->setVisible(edge_selected);
    }
    add_guide_node_row_->setVisible(edge_selected);
    if (QWidget* label = selection_form_->labelForField(add_guide_node_row_)) {
        label->setVisible(edge_selected);
    }
    remove_guide_node_row_->setVisible(edge_selected);
    if (QWidget* label = selection_form_->labelForField(remove_guide_node_row_)) {
        label->setVisible(edge_selected);
    }

    refresh_edge_segments_table();
    request_structure_preview_refresh();

    add_guide_node_button_->setEnabled(edge_selected && view_->selected_edge_can_add_guide_node());
    remove_guide_node_button_->setEnabled(edge_selected && view_->selected_edge_can_remove_guide_node());
    refresh_design_errors_summary();
    refresh_yaml_source_widget();
    request_network_view_refresh();

    if (properties_root != nullptr) {
        properties_root->setUpdatesEnabled(true);
        properties_root->update();
    }
}

void MainWindow::request_network_view_refresh() {
    if (network_view_refresh_pending_) {
        return;
    }

    network_view_refresh_pending_ = true;
    QTimer::singleShot(0, this, [this]() {
        network_view_refresh_pending_ = false;
        if (view_ != nullptr) {
            view_->update();
        }
    });
}

void MainWindow::request_structure_preview_refresh() {
    if (structure_preview_refresh_pending_) {
        return;
    }

    structure_preview_refresh_pending_ = true;
    QTimer::singleShot(0, this, [this]() {
        structure_preview_refresh_pending_ = false;
        refresh_selected_structure_preview();
    });
}

void MainWindow::refresh_selected_structure_preview() {
    if (selected_structure_widget_ == nullptr) {
        return;
    }

    const bool node_selected = view_->has_node_selection();
    const QString structure_path = node_selected ? view_->selected_node_structure().trimmed() : QString();
    const bool has_structure = !structure_path.isEmpty();

    selected_structure_row_->setVisible(has_structure);

    if (!has_structure) {
        if (!last_loaded_structure_path_.isEmpty()) {
            selected_structure_widget_->set_structure(nullptr);
            last_loaded_structure_path_.clear();
        }
        request_network_view_refresh();
        return;
    }

    QString resolved_path = structure_path;
    if (QDir::isRelativePath(structure_path)) {
        resolved_path = QDir(loaded_network_directory_).filePath(structure_path);
    }
    resolved_path = QFileInfo(resolved_path).canonicalFilePath().isEmpty() ? QFileInfo(resolved_path).absoluteFilePath()
                                                                          : QFileInfo(resolved_path).canonicalFilePath();

    if (!QFileInfo::exists(resolved_path)) {
        qWarning() << "Structure file does not exist:" << resolved_path;
        if (!last_loaded_structure_path_.isEmpty()) {
            selected_structure_widget_->set_structure(nullptr);
            last_loaded_structure_path_.clear();
        }
        request_network_view_refresh();
        return;
    }

    if (resolved_path == last_loaded_structure_path_ && selected_structure_widget_->get_structure() != nullptr) {
        request_network_view_refresh();
        return;
    }

    try {
        if (!g_loaded_structure_cache.contains(resolved_path)) {
            StructureLoader loader;
            const auto structures = loader.load_file(resolved_path);
            if (structures.empty()) {
                throw std::runtime_error("No structures loaded from file.");
            }
            g_loaded_structure_cache.insert(resolved_path, structures.back());
        }

        selected_structure_widget_->set_structure(g_loaded_structure_cache.value(resolved_path));
        last_loaded_structure_path_ = resolved_path;
    } catch (const std::exception& ex) {
        qWarning() << "Failed to load structure for selected node:" << ex.what();
        selected_structure_widget_->set_structure(nullptr);
        last_loaded_structure_path_.clear();
    }

    request_network_view_refresh();
}

void MainWindow::refresh_design_errors_summary() {
    design_errors_cache_ = view_->design_errors();
    const int count = design_errors_cache_.size();
    design_errors_label_->setText(QString("%1 error%2 in design found.").arg(count).arg(count == 1 ? "" : "s"));
    if (count > 0) {
        design_errors_label_->setStyleSheet("QLabel { color: #dc322f; font-weight: 600; }");
    } else {
        design_errors_label_->setStyleSheet(QString());
    }
    design_errors_button_->setEnabled(count > 0);
}

void MainWindow::show_design_errors_dialog() {
    if (design_errors_cache_.isEmpty()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Design errors");
    dialog.resize(680, 360);
    auto* layout = new QVBoxLayout(&dialog);
    auto* list = new QListWidget(&dialog);
    list->addItems(design_errors_cache_);
    layout->addWidget(list);

    auto* close_button = new QPushButton("Close", &dialog);
    connect(close_button, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(close_button);

    dialog.exec();
}

void MainWindow::pick_background_color() {
    ColorPickerDialog dialog(view_->background_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_background_color(dialog.color());
    set_color_chip(bg_color_button_, bg_color_hex_, dialog.color());
    refresh_yaml_source_widget();
}

void MainWindow::pick_edge_color() {
    if (!view_->has_edge_selection()) {
        return;
    }
    ColorPickerDialog dialog(view_->selected_item_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_selected_item_color(dialog.color());
    on_selection_changed();
    refresh_yaml_source_widget();
}

void MainWindow::pick_selection_node_fill_color() {
    if (!view_->has_node_selection()) {
        return;
    }
    ColorPickerDialog dialog(view_->selected_node_fill_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_selected_node_fill_color(dialog.color());
    on_selection_changed();
    refresh_yaml_source_widget();
}

void MainWindow::pick_selection_node_outline_color() {
    if (!view_->has_node_selection()) {
        return;
    }
    ColorPickerDialog dialog(view_->selected_node_outline_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_selected_node_outline_color(dialog.color());
    on_selection_changed();
    refresh_yaml_source_widget();
}

void MainWindow::pick_label_color() {
    ColorPickerDialog dialog(view_->label_color(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    view_->set_label_color(dialog.color());
    set_color_chip(label_color_button_, label_color_hex_, dialog.color());
    refresh_yaml_source_widget();
}

void MainWindow::update_window_title() {
    const QString base_title = QString("%1 %2").arg(kProgramName, kProgramVersion);
    if (current_file_path_.isEmpty()) {
        setWindowTitle(base_title);
        return;
    }

    setWindowTitle(QString("%1 - %2").arg(QFileInfo(current_file_path_).fileName(), base_title));
}

void MainWindow::refresh_yaml_source_widget() {
    if (yaml_source_view_ == nullptr || view_ == nullptr) {
        return;
    }

    NetworkData data = view_->network();
    data.settings = view_->current_settings();
    data.has_settings = true;

    QString yaml_text;
    QString error;
    if (!network_yaml_to_string(data, yaml_text, error)) {
        yaml_source_view_->setPlainText(QString("# Failed to generate YAML source:\n%1").arg(error));
        return;
    }

    yaml_source_view_->setPlainText(yaml_text);
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
    set_color_chip(label_color_button_, label_color_hex_, view_->label_color());
    on_selection_changed();
    update_window_title();
}
