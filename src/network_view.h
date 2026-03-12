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
#include <QOpenGLWidget>
#include <QPoint>

#include "network_io.h"

class NetworkView : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit NetworkView(QWidget* parent = nullptr);

    void set_network(NetworkData data);
    const NetworkData& network() const;

    void set_node_radius(float radius);
    float node_radius() const;

    void set_line_thickness(float thickness);
    float line_thickness() const;

    void set_background_color(const QColor& color);
    QColor background_color() const;

    void set_node_fill_color(const QColor& color);
    QColor node_fill_color() const;

    void set_node_outline_color(const QColor& color);
    QColor node_outline_color() const;

    void set_line_color(const QColor& color);
    QColor line_color() const;

    void set_label_color(const QColor& color);
    QColor label_color() const;

    void set_node_outline_thickness(float thickness);
    float node_outline_thickness() const;

    void set_font_family(const QString& family);
    QString font_family() const;

    void set_font_size(float size);
    float font_size() const;

    void set_label_angle_degrees(float degrees);
    float label_angle_degrees() const;

    void set_node_label_distance(float distance);
    float node_label_distance() const;

    void set_value_decimals(int decimals);
    int value_decimals() const;

    void set_value_unit(const QString& unit);
    QString value_unit() const;

    int selected_node_index() const;
    int selected_edge_index() const;
    bool has_node_selection() const;
    bool has_edge_selection() const;
    QString selected_node_name() const;
    void set_selected_node_name(const QString& label);
    void reset_selected_node_name();

    QString selected_item_type() const;
    QString selected_item_label() const;
    QColor selected_item_color() const;
    QColor selected_node_fill_color() const;
    QColor selected_node_outline_color() const;
    void set_selected_item_color(const QColor& color);
    void set_selected_node_fill_color(const QColor& color);
    void set_selected_node_outline_color(const QColor& color);

    ViewSettingsData current_settings() const;
    void apply_settings(const ViewSettingsData& settings);

    bool save_view_to_png(const QString& path, QString& error) const;

signals:
    void selection_changed();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void draw_scene(QPainter& painter, const QRectF& world_visible_rect) const;
    QRectF scene_bounds() const;
    void fit_network_to_viewport();
    void snap_node_to_grid(NodeData& node) const;
    void snap_all_nodes_to_grid();
    int pick_node(const QPointF& world_point) const;
    int pick_edge(const QPointF& world_point) const;
    QPointF to_world(const QPointF& screen_point) const;

    static constexpr float kGridSize = 100.0f;

    NetworkData network_data_;
    float node_radius_{10.0f};
    float line_thickness_{5.0f};
    QColor background_color_{253, 246, 227};
    QColor node_fill_color_{Qt::white};
    QColor node_outline_color_{Qt::black};
    QColor line_color_{220, 50, 47};
    QColor label_color_{Qt::black};
    float node_outline_thickness_{3.0f};
    QString font_family_{"Bahnschrift SemiBold"};
    float font_size_{12.0f};
    float label_angle_degrees_{90.0f};
    float node_label_distance_{14.0f};
    int value_decimals_{3};
    QString value_unit_{"eV"};
    int dragged_node_{-1};
    int selected_node_{-1};
    int selected_edge_{-1};
    QPointF drag_offset_;
    QPointF pan_offset_{0.0, 0.0};
    bool panning_{false};
    QPoint last_pan_pos_;
    float zoom_scale_{1.0f};
};
