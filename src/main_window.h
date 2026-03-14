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

#include <QMainWindow>
#include <QStringList>

#include "network_view.h"

class QAction;
class QDoubleSpinBox;
class QFont;
class QComboBox;
class QFontComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QPlainTextEdit;
class QTableWidget;
class LogWindow;
class AnaglyphWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void load_yaml();
    void save_yaml();
    void save_yaml_as();
    void save_png();
    void show_about();
    void show_debug_log();
    void update_node_size(double value);
    void update_line_thickness(double value);
    void update_node_outline_thickness(double value);
    void update_font_size(double value);
    void update_label_angle(double value);
    void update_font_family(const QFont& font);
    void update_node_label_distance(double value);
    void update_value_decimals(int value);
    void update_value_unit(const QString& unit);
    void update_selected_node_name(const QString& label);
    void reset_selected_node_name();
    void on_selection_changed();
    void pick_background_color();
    void pick_label_color();
    void pick_edge_color();
    void pick_selection_node_fill_color();
    void pick_selection_node_outline_color();
    void toggle_selected_edge_swap_labels(bool checked);
    void on_edge_segment_selected(int current_row, int current_column, int previous_row, int previous_column);
    void on_edge_label_segment_changed(int index);
    void add_selected_edge_guide_node();
    void remove_selected_edge_guide_node();
    void refresh_edge_segments_table();
    void show_design_errors_dialog();

private:
    QString initial_dialog_directory() const;
    void remember_dialog_path(const QString& file_path);
    void set_color_chip(QPushButton* button, QLabel* hex_label, const QColor& color) const;
    void build_menus();
    void build_properties_widget();
    void sync_controls_from_view();
    void update_window_title();
    void refresh_yaml_source_widget();
    void refresh_design_errors_summary();
    void refresh_selected_structure_preview();
    void request_structure_preview_refresh();
    void request_network_view_refresh();

    NetworkView* view_{nullptr};
    QString current_file_path_;
    QString last_directory_path_;
    QString loaded_network_directory_;
    QString last_loaded_structure_path_;
    bool structure_preview_refresh_pending_{false};
    bool network_view_refresh_pending_{false};

    QDoubleSpinBox* node_radius_spin_{nullptr};
    QDoubleSpinBox* line_thickness_spin_{nullptr};
    QDoubleSpinBox* node_outline_spin_{nullptr};
    QDoubleSpinBox* font_size_spin_{nullptr};
    QDoubleSpinBox* label_angle_spin_{nullptr};
    QDoubleSpinBox* label_distance_spin_{nullptr};
    QFontComboBox* font_family_combo_{nullptr};
    QSpinBox* value_decimals_spin_{nullptr};
    QComboBox* value_unit_combo_{nullptr};
    QLabel* selected_type_value_{nullptr};
    QLineEdit* selected_node_edit_{nullptr};
    QPushButton* reset_node_name_button_{nullptr};
    QPushButton* edge_color_button_{nullptr};
    QLabel* edge_color_hex_{nullptr};
    QPushButton* selection_node_fill_color_button_{nullptr};
    QLabel* selection_node_fill_color_hex_{nullptr};
    QPushButton* selection_node_outline_color_button_{nullptr};
    QLabel* selection_node_outline_color_hex_{nullptr};
    QPushButton* swap_edge_labels_button_{nullptr};
    QTableWidget* edge_segments_table_{nullptr};
    QComboBox* edge_label_segment_combo_{nullptr};
    QWidget* selected_node_name_row_{nullptr};
    QWidget* edge_color_row_{nullptr};
    QWidget* selected_node_fill_row_{nullptr};
    QWidget* selected_node_outline_row_{nullptr};
    QWidget* edge_swap_labels_row_{nullptr};
    QWidget* edge_segments_row_{nullptr};
    QWidget* edge_label_segment_row_{nullptr};
    QWidget* add_guide_node_row_{nullptr};
    QWidget* remove_guide_node_row_{nullptr};
    QFormLayout* selection_form_{nullptr};
    AnaglyphWidget* selected_structure_widget_{nullptr};
    QWidget* selected_structure_row_{nullptr};
    QPushButton* add_guide_node_button_{nullptr};
    QPushButton* remove_guide_node_button_{nullptr};
    QLabel* design_errors_label_{nullptr};
    QPushButton* design_errors_button_{nullptr};
    QStringList design_errors_cache_;

    QPushButton* bg_color_button_{nullptr};
    QLabel* bg_color_hex_{nullptr};
    QPushButton* label_color_button_{nullptr};
    QLabel* label_color_hex_{nullptr};

    QAction* properties_toggle_action_{nullptr};
    QAction* yaml_source_toggle_action_{nullptr};
    QAction* debug_log_action_{nullptr};
    QPlainTextEdit* yaml_source_view_{nullptr};
    LogWindow* log_window_{nullptr};
};
