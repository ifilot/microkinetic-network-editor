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

#include <QMainWindow>

#include "network_view.h"

class QAction;
class QDoubleSpinBox;
class QFont;
class QComboBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class LogWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void load_yaml();
    void save_yaml();
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
    void pick_selection_color();
    void pick_selection_node_fill_color();
    void pick_selection_node_outline_color();

private:
    QString initial_dialog_directory() const;
    void remember_dialog_path(const QString& file_path);
    void set_color_chip(QPushButton* button, QLabel* hex_label, const QColor& color) const;
    void build_menus();
    void build_settings_widget();
    void sync_controls_from_view();

    NetworkView* view_{nullptr};
    QString current_file_path_;
    QString last_directory_path_;

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
    QPushButton* selection_color_button_{nullptr};
    QLabel* selection_color_hex_{nullptr};
    QPushButton* selection_node_fill_color_button_{nullptr};
    QLabel* selection_node_fill_color_hex_{nullptr};
    QPushButton* selection_node_outline_color_button_{nullptr};
    QLabel* selection_node_outline_color_hex_{nullptr};

    QPushButton* bg_color_button_{nullptr};
    QLabel* bg_color_hex_{nullptr};
    QPushButton* label_color_button_{nullptr};
    QLabel* label_color_hex_{nullptr};

    QAction* settings_toggle_action_{nullptr};
    QAction* debug_log_action_{nullptr};
    LogWindow* log_window_{nullptr};
};
