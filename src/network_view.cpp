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

#include "network_view.h"

#include <QDebug>
#include <QImage>
#include <QLocale>
#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <cmath>

NetworkView::NetworkView(QWidget* parent)
    : QOpenGLWidget(parent) {
    setMinimumSize(900, 600);
    setMouseTracking(true);
}

void NetworkView::set_network(NetworkData data) {
    qInfo() << "Applying network data" << data.nodes.size() << "nodes" << data.edges.size() << "edges";
    network_data_ = std::move(data);
    for (EdgeData& edge : network_data_.edges) {
        const int segment_count = static_cast<int>(edge.guide_nodes.size()) + 1;
        edge.label_segment_index = std::clamp(edge.label_segment_index, 0, std::max(0, segment_count - 1));
        edge.segment_kinds.resize(static_cast<size_t>(segment_count), EdgeData::SegmentKind::Straight);
    }
    fit_network_to_viewport();
    snap_all_nodes_to_grid();
    selected_node_ = -1;
    selected_edge_ = -1;
    dragged_guide_node_ = -1;
    emit selection_changed();
    update();
}

const NetworkData& NetworkView::network() const { return network_data_; }

void NetworkView::set_node_radius(float radius) {
    node_radius_ = std::max(6.0f, radius);
    update();
}

float NetworkView::node_radius() const { return node_radius_; }

void NetworkView::set_line_thickness(float thickness) {
    line_thickness_ = std::max(1.0f, thickness);
    update();
}

float NetworkView::line_thickness() const { return line_thickness_; }

void NetworkView::set_background_color(const QColor& color) {
    background_color_ = color;
    update();
}

QColor NetworkView::background_color() const { return background_color_; }

void NetworkView::set_node_fill_color(const QColor& color) {
    node_fill_color_ = color;
    update();
}

QColor NetworkView::node_fill_color() const { return node_fill_color_; }

void NetworkView::set_node_outline_color(const QColor& color) {
    node_outline_color_ = color;
    update();
}

QColor NetworkView::node_outline_color() const { return node_outline_color_; }

void NetworkView::set_line_color(const QColor& color) {
    line_color_ = color;
    update();
}

QColor NetworkView::line_color() const { return line_color_; }

void NetworkView::set_label_color(const QColor& color) {
    label_color_ = color;
    update();
}

QColor NetworkView::label_color() const { return label_color_; }

void NetworkView::set_node_outline_thickness(float thickness) {
    node_outline_thickness_ = std::max(0.5f, thickness);
    update();
}

float NetworkView::node_outline_thickness() const { return node_outline_thickness_; }

void NetworkView::set_font_family(const QString& family) {
    if (!family.isEmpty()) {
        font_family_ = family;
        update();
    }
}

QString NetworkView::font_family() const { return font_family_; }

void NetworkView::set_font_size(float size) {
    font_size_ = std::max(6.0f, size);
    update();
}

float NetworkView::font_size() const { return font_size_; }

void NetworkView::set_label_angle_degrees(float degrees) {
    label_angle_degrees_ = degrees;
    update();
}

float NetworkView::label_angle_degrees() const { return label_angle_degrees_; }

void NetworkView::set_node_label_distance(float distance) {
    node_label_distance_ = std::max(0.0f, distance);
    update();
}

float NetworkView::node_label_distance() const { return node_label_distance_; }

void NetworkView::set_value_decimals(int decimals) {
    value_decimals_ = std::clamp(decimals, 0, 10);
    update();
}

int NetworkView::value_decimals() const { return value_decimals_; }

void NetworkView::set_value_unit(const QString& unit) {
    if (unit == "eV" || unit == "kJ/mol") {
        value_unit_ = unit;
        update();
    }
}

QString NetworkView::value_unit() const { return value_unit_; }
int NetworkView::selected_node_index() const { return selected_node_; }

int NetworkView::selected_edge_index() const { return selected_edge_; }

bool NetworkView::has_node_selection() const { return selected_node_ >= 0; }

bool NetworkView::has_edge_selection() const { return selected_edge_ >= 0; }

QString NetworkView::selected_node_name() const {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return QString();
    }
    return network_data_.nodes[selected_node_].name;
}

void NetworkView::set_selected_node_name(const QString& name) {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return;
    }
    network_data_.nodes[selected_node_].name = name;
    emit selection_changed();
    update();
}

void NetworkView::reset_selected_node_name() {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return;
    }
    NodeData& node = network_data_.nodes[selected_node_];
    node.name = node.label;
    emit selection_changed();
    update();
}


QString NetworkView::selected_item_type() const {
    if (has_node_selection()) {
        return "Node";
    }
    if (has_edge_selection()) {
        return "Edge";
    }
    return QString();
}

QString NetworkView::selected_item_label() const {
    if (has_node_selection() && selected_node_ < static_cast<int>(network_data_.nodes.size())) {
        const NodeData& node = network_data_.nodes[selected_node_];
        return node.name.isEmpty() ? node.label : node.name;
    }
    if (has_edge_selection() && selected_edge_ < static_cast<int>(network_data_.edges.size())) {
        const EdgeData& edge = network_data_.edges[selected_edge_];
        if (edge.from_index >= 0 && edge.to_index >= 0 &&
            edge.from_index < static_cast<int>(network_data_.nodes.size()) &&
            edge.to_index < static_cast<int>(network_data_.nodes.size())) {
            return QString("%1 → %2").arg(network_data_.nodes[edge.from_index].label, network_data_.nodes[edge.to_index].label);
        }
    }
    return QString();
}

QColor NetworkView::selected_item_color() const {
    if (has_edge_selection() && selected_edge_ < static_cast<int>(network_data_.edges.size())) {
        const QString& color = network_data_.edges[selected_edge_].color;
        return color.isEmpty() ? line_color_ : QColor(color);
    }
    if (has_node_selection() && selected_node_ < static_cast<int>(network_data_.nodes.size())) {
        return selected_node_outline_color();
    }
    return QColor();
}

QColor NetworkView::selected_node_fill_color() const {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return QColor();
    }
    const QString& color = network_data_.nodes[selected_node_].fill_color;
    return color.isEmpty() ? node_fill_color_ : QColor(color);
}

QColor NetworkView::selected_node_outline_color() const {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return QColor();
    }
    const QString& color = network_data_.nodes[selected_node_].outline_color;
    return color.isEmpty() ? node_outline_color_ : QColor(color);
}

void NetworkView::set_selected_item_color(const QColor& color) {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return;
    }
    network_data_.edges[selected_edge_].color = color.name();
    update();
}

void NetworkView::set_selected_node_fill_color(const QColor& color) {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return;
    }
    network_data_.nodes[selected_node_].fill_color = color.name();
    update();
}

void NetworkView::set_selected_node_outline_color(const QColor& color) {
    if (!has_node_selection() || selected_node_ >= static_cast<int>(network_data_.nodes.size())) {
        return;
    }
    network_data_.nodes[selected_node_].outline_color = color.name();
    update();
}

bool NetworkView::selected_edge_swap_label_sides() const {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return false;
    }
    return network_data_.edges[selected_edge_].swap_label_sides;
}

void NetworkView::set_selected_edge_swap_label_sides(bool swap) {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return;
    }
    network_data_.edges[selected_edge_].swap_label_sides = swap;
    update();
}

bool NetworkView::can_select_edge_label_segment() const { return selected_edge_segment_count() > 1; }

int NetworkView::selected_edge_label_segment_index() const {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return 0;
    }
    return network_data_.edges[selected_edge_].label_segment_index;
}

int NetworkView::selected_edge_segment_count() const {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return 0;
    }
    const EdgeData& edge = network_data_.edges[selected_edge_];
    return static_cast<int>(edge.guide_nodes.size()) + 1;
}

void NetworkView::set_selected_edge_label_segment_index(int index) {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return;
    }
    EdgeData& edge = network_data_.edges[selected_edge_];
    const int segment_count = static_cast<int>(edge.guide_nodes.size()) + 1;
    edge.label_segment_index = std::clamp(index, 0, std::max(0, segment_count - 1));
    update();
}

bool NetworkView::selected_edge_has_segments() const { return selected_edge_segment_count() > 0; }

bool NetworkView::selected_edge_can_add_guide_node() const { return has_edge_selection(); }

void NetworkView::add_selected_edge_guide_node() {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return;
    }
    EdgeData& edge = network_data_.edges[selected_edge_];
    const std::vector<QPointF> points = edge_polyline_points(edge);
    if (points.size() < 2) {
        return;
    }

    const int segment_count = static_cast<int>(points.size()) - 1;
    const int insert_after = std::clamp(edge.label_segment_index, 0, std::max(0, segment_count - 1));
    const QPointF midpoint = (points[insert_after] + points[insert_after + 1]) * 0.5;

    EdgeData::GuideNode guide_node;
    guide_node.x = static_cast<float>(std::round(midpoint.x() / kGridSize) * kGridSize);
    guide_node.y = static_cast<float>(std::round(midpoint.y() / kGridSize) * kGridSize);

    edge.guide_nodes.insert(edge.guide_nodes.begin() + insert_after, guide_node);
    edge.segment_kinds.resize(edge.guide_nodes.size() + 1, EdgeData::SegmentKind::Straight);
    edge.segment_kinds[insert_after] = EdgeData::SegmentKind::Straight;
    if (insert_after + 1 < static_cast<int>(edge.segment_kinds.size())) {
        edge.segment_kinds[insert_after + 1] = EdgeData::SegmentKind::Straight;
    }
    edge.label_segment_index = insert_after;
    emit selection_changed();
    update();
}

bool NetworkView::selected_edge_can_remove_guide_node() const {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return false;
    }
    return !network_data_.edges[selected_edge_].guide_nodes.empty();
}

void NetworkView::remove_selected_edge_guide_node() {
    if (!selected_edge_can_remove_guide_node()) {
        return;
    }
    EdgeData& edge = network_data_.edges[selected_edge_];
    const int segment_count = static_cast<int>(edge.guide_nodes.size()) + 1;
    if (segment_count < 2) {
        return;
    }

    const int remove_at = std::clamp(edge.label_segment_index, 0, static_cast<int>(edge.guide_nodes.size()) - 1);
    edge.guide_nodes.erase(edge.guide_nodes.begin() + remove_at);
    edge.segment_kinds.resize(edge.guide_nodes.size() + 1, EdgeData::SegmentKind::Straight);

    const int new_segment_count = static_cast<int>(edge.guide_nodes.size()) + 1;
    edge.label_segment_index = std::clamp(edge.label_segment_index, 0, std::max(0, new_segment_count - 1));
    emit selection_changed();
    update();
}

NetworkView::SegmentKindUi NetworkView::selected_edge_segment_kind() const {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return SegmentKindUi::Straight;
    }
    const EdgeData& edge = network_data_.edges[selected_edge_];
    if (edge.segment_kinds.empty()) {
        return SegmentKindUi::Straight;
    }
    const int idx = std::clamp(edge.label_segment_index, 0, static_cast<int>(edge.segment_kinds.size()) - 1);
    return to_ui_segment_kind(edge.segment_kinds[idx]);
}

void NetworkView::set_selected_edge_segment_kind(SegmentKindUi kind) {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return;
    }
    const EdgeData& edge = network_data_.edges[selected_edge_];
    const int idx = std::clamp(edge.label_segment_index, 0, std::max(0, static_cast<int>(edge.guide_nodes.size())));
    set_selected_edge_segment_kind_at(idx, kind);
}

NetworkView::SegmentKindUi NetworkView::selected_edge_segment_kind_at(int segment_index) const {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return SegmentKindUi::Straight;
    }
    const EdgeData& edge = network_data_.edges[selected_edge_];
    if (edge.segment_kinds.empty()) {
        return SegmentKindUi::Straight;
    }
    const int idx = std::clamp(segment_index, 0, static_cast<int>(edge.segment_kinds.size()) - 1);
    return to_ui_segment_kind(edge.segment_kinds[idx]);
}

void NetworkView::set_selected_edge_segment_kind_at(int segment_index, SegmentKindUi kind) {
    if (!has_edge_selection() || selected_edge_ >= static_cast<int>(network_data_.edges.size())) {
        return;
    }
    EdgeData& edge = network_data_.edges[selected_edge_];
    if (edge.segment_kinds.empty()) {
        edge.segment_kinds.resize(static_cast<size_t>(std::max(1, static_cast<int>(edge.guide_nodes.size()) + 1)), EdgeData::SegmentKind::Straight);
    }
    const int idx = std::clamp(segment_index, 0, static_cast<int>(edge.segment_kinds.size()) - 1);
    const EdgeData::SegmentKind target_kind = from_ui_segment_kind(kind);

    if (target_kind != EdgeData::SegmentKind::Straight) {
        const std::vector<QPointF> points = edge_polyline_points(edge);
        if (idx + 1 < static_cast<int>(points.size())) {
            const QPointF a = points[idx];
            const QPointF b = points[idx + 1];
            const double adx = std::abs(b.x() - a.x());
            const double ady = std::abs(b.y() - a.y());
            const double tol = 1e-3;
            const bool near_diagonal = std::abs(adx - kGridSize) <= tol && std::abs(ady - kGridSize) <= tol;
            if (!near_diagonal) {
                return;
            }
        }
    }

    edge.segment_kinds[idx] = target_kind;
    emit selection_changed();
    update();
}

ViewSettingsData NetworkView::current_settings() const {
    ViewSettingsData settings;
    settings.node_radius = node_radius_;
    settings.line_thickness = line_thickness_;
    settings.node_outline_thickness = node_outline_thickness_;
    settings.font_size = font_size_;
    settings.label_angle_degrees = label_angle_degrees_;
    settings.node_label_distance = node_label_distance_;
    settings.font_family = font_family_;
    settings.background_color = background_color_.name();
    settings.node_fill_color = node_fill_color_.name();
    settings.node_outline_color = node_outline_color_.name();
    settings.line_color = line_color_.name();
    settings.label_color = label_color_.name();
    settings.value_decimals = value_decimals_;
    settings.value_unit = value_unit_;
    return settings;
}

void NetworkView::apply_settings(const ViewSettingsData& settings) {
    set_node_radius(settings.node_radius);
    set_line_thickness(settings.line_thickness);
    set_node_outline_thickness(settings.node_outline_thickness);
    set_font_size(settings.font_size);
    set_label_angle_degrees(settings.label_angle_degrees);
    set_node_label_distance(settings.node_label_distance);
    set_font_family(settings.font_family);
    set_background_color(QColor(settings.background_color));
    set_node_fill_color(QColor(settings.node_fill_color));
    set_node_outline_color(QColor(settings.node_outline_color));
    set_line_color(QColor(settings.line_color));
    set_label_color(QColor(settings.label_color));
    set_value_decimals(settings.value_decimals);
    set_value_unit(settings.value_unit);
}

QStringList NetworkView::design_errors() const {
    QStringList errors;
    const double tol = 1e-3;

    for (int edge_index = 0; edge_index < static_cast<int>(network_data_.edges.size()); ++edge_index) {
        const EdgeData& edge = network_data_.edges[edge_index];
        const std::vector<QPointF> points = edge_polyline_points(edge);
        if (points.size() < 2) {
            continue;
        }

        for (int seg = 0; seg + 1 < static_cast<int>(points.size()); ++seg) {
            const EdgeData::SegmentKind kind = (seg < static_cast<int>(edge.segment_kinds.size())) ? edge.segment_kinds[seg] : EdgeData::SegmentKind::Straight;
            if (kind == EdgeData::SegmentKind::Straight) {
                continue;
            }
            const double adx = std::abs(points[seg + 1].x() - points[seg].x());
            const double ady = std::abs(points[seg + 1].y() - points[seg].y());
            const bool near_diagonal = std::abs(adx - kGridSize) <= tol && std::abs(ady - kGridSize) <= tol;
            if (near_diagonal) {
                continue;
            }

            QString edge_name = QString("Edge %1").arg(edge_index + 1);
            if (edge.from_index >= 0 && edge.from_index < static_cast<int>(network_data_.nodes.size()) &&
                edge.to_index >= 0 && edge.to_index < static_cast<int>(network_data_.nodes.size())) {
                edge_name = QString("%1 → %2").arg(network_data_.nodes[edge.from_index].label,
                                                   network_data_.nodes[edge.to_index].label);
            }
            errors.push_back(QString("%1, segment %2: bend/wiggle requires one diagonal grid-step separation.")
                                 .arg(edge_name)
                                 .arg(seg + 1));
        }
    }

    return errors;
}

bool NetworkView::save_view_to_png(const QString& path, QString& error) const {
    const QRectF bounds = scene_bounds();
    if (bounds.isEmpty()) {
        error = "Nothing to export.";
        return false;
    }

    const int padding = 40;
    const QSize image_size(static_cast<int>(std::ceil(bounds.width())) + 2 * padding,
                           static_cast<int>(std::ceil(bounds.height())) + 2 * padding);
    QImage image(image_size, QImage::Format_ARGB32);
    image.fill(background_color_);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(padding - bounds.left(), padding - bounds.top());
    draw_scene(painter, bounds);
    painter.end();

    if (!image.save(path, "PNG")) {
        error = "Failed to write PNG file.";
        return false;
    }
    return true;
}

void NetworkView::initializeGL() { qInfo() << "OpenGL initialized for NetworkView"; }

void NetworkView::paintGL() {
    QOpenGLFunctions* gl = context()->functions();
    gl->glClearColor(background_color_.redF(), background_color_.greenF(), background_color_.blueF(), 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF world_visible_rect(-pan_offset_.x() / zoom_scale_,
                                    -pan_offset_.y() / zoom_scale_,
                                    width() / zoom_scale_,
                                    height() / zoom_scale_);

    painter.translate(pan_offset_);
    painter.scale(zoom_scale_, zoom_scale_);
    draw_scene(painter, world_visible_rect);
}

void NetworkView::draw_scene(QPainter& painter, const QRectF& world_visible_rect) const {
    const QPen grid_pen(QColor(85, 85, 85), 1.0 / zoom_scale_, Qt::DashLine);
    painter.setPen(grid_pen);

    const float left = static_cast<float>(world_visible_rect.left()) - kGridSize;
    const float right = static_cast<float>(world_visible_rect.right()) + kGridSize;
    const float top = static_cast<float>(world_visible_rect.top()) - kGridSize;
    const float bottom = static_cast<float>(world_visible_rect.bottom()) + kGridSize;

    const float start_x = std::floor(left / kGridSize) * kGridSize;
    const float start_y = std::floor(top / kGridSize) * kGridSize;
    for (float x = start_x; x <= right; x += kGridSize) {
        painter.drawLine(QPointF(x, top), QPointF(x, bottom));
    }
    for (float y = start_y; y <= bottom; y += kGridSize) {
        painter.drawLine(QPointF(left, y), QPointF(right, y));
    }

    QPen edge_pen(line_color_);
    edge_pen.setWidthF(line_thickness_);
    edge_pen.setCapStyle(Qt::RoundCap);

    QFont text_font(font_family_, static_cast<int>(std::round(font_size_)));
    painter.setFont(text_font);
    const QFontMetrics metrics(text_font);

    for (int edge_index = 0; edge_index < static_cast<int>(network_data_.edges.size()); ++edge_index) {
        const EdgeData& edge = network_data_.edges[edge_index];
        if (edge.from_index < 0 || edge.to_index < 0 ||
            edge.from_index >= static_cast<int>(network_data_.nodes.size()) ||
            edge.to_index >= static_cast<int>(network_data_.nodes.size())) {
            continue;
        }

        const std::vector<QPointF> points = edge_polyline_points(edge);
        if (points.size() < 2) {
            continue;
        }

        const QColor effective_edge_color = edge.color.isEmpty() ? line_color_ : QColor(edge.color);
        edge_pen.setColor(effective_edge_color);
        painter.setPen(edge_pen);

        QPainterPath edge_path(points.front());
        for (int seg = 0; seg + 1 < static_cast<int>(points.size()); ++seg) {
            const EdgeData::SegmentKind kind = (seg < static_cast<int>(edge.segment_kinds.size())) ? edge.segment_kinds[seg] : EdgeData::SegmentKind::Straight;
            append_segment_path(edge_path, points[seg], points[seg + 1], kind);
        }
        painter.drawPath(edge_path);

        if (edge_index == selected_edge_) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 170, 0), 2.0, Qt::DashLine));
            painter.drawRect(edge_path.boundingRect().adjusted(-8.0, -8.0, 8.0, 8.0));
            painter.setPen(QPen(QColor(Qt::black), 2.0));
            painter.setBrush(Qt::NoBrush);
            for (const EdgeData::GuideNode& guide_node : edge.guide_nodes) {
                painter.drawEllipse(QPointF(guide_node.x, guide_node.y), 5.0, 5.0);
            }
            painter.restore();
            painter.setPen(edge_pen);
        }

        if (!edge.values.empty()) {
            auto parse_value = [](const QString& raw, double& out) -> bool {
                bool ok = false;
                out = QLocale::c().toDouble(raw.trimmed(), &ok);
                return ok;
            };

            auto format_value = [this](double value) -> QString {
                const double displayed = (value_unit_ == "kJ/mol") ? value * 96.48533212 : value;
                return QLocale::c().toString(displayed, 'f', value_decimals_);
            };

            const int segment_count = static_cast<int>(points.size()) - 1;
            const int label_segment_index = std::clamp(edge.label_segment_index, 0, std::max(0, segment_count - 1));
            const EdgeData::SegmentKind label_segment_kind = (label_segment_index < static_cast<int>(edge.segment_kinds.size())) ? edge.segment_kinds[label_segment_index] : EdgeData::SegmentKind::Straight;

            QPointF tangent;
            if (!edge_segment_tangent(points, label_segment_index, label_segment_kind, tangent)) {
                continue;
            }

            QString forward_text = edge.values[0].trimmed();
            double numeric_forward = 0.0;
            if (parse_value(edge.values[0], numeric_forward)) {
                forward_text = format_value(numeric_forward);
            }

            QString backward_text;
            if (edge.values.size() >= 2) {
                backward_text = edge.values[1].trimmed();
                double numeric_backward = 0.0;
                if (parse_value(edge.values[1], numeric_backward)) {
                    backward_text = format_value(numeric_backward);
                }
            }

            const bool edge_goes_left = tangent.x() < 0.0;
            const bool edge_is_vertical = std::abs(tangent.x()) < 1e-6;
            const QString forward_arrow = edge_goes_left ? "◀" : "▶";
            const QString backward_arrow = edge_goes_left ? "▶" : "◀";
            bool forward_prepend_arrow = false;
            if (!edge_is_vertical && edge_goes_left) {
                forward_prepend_arrow = true;
            }

            auto with_arrow = [](const QString& value, const QString& arrow, bool prepend) {
                return prepend ? (arrow + " " + value) : (value + " " + arrow);
            };

            const QString edge_type = edge.type.trimmed().toLower();
            if (edge_type == "ads") {
                forward_text.clear();
                if (!backward_text.isEmpty()) {
                    if (edge.values.size() >= 2) {
                        double numeric_backward = 0.0;
                        if (parse_value(edge.values[1], numeric_backward)) {
                            const QString sign = (numeric_backward >= 0.0) ? "+" : "";
                            backward_text = sign + backward_text;
                        }
                    }
                    backward_text = "|" + backward_text + "|";
                    backward_text = with_arrow(backward_text, backward_arrow, !forward_prepend_arrow);
                }
            } else if (edge_type == "rearrangement") {
                if (!forward_text.isEmpty() && !forward_text.startsWith('+') && !forward_text.startsWith('-')) {
                    forward_text = "+" + forward_text;
                }
                if (!backward_text.isEmpty() && !backward_text.startsWith('+') && !backward_text.startsWith('-')) {
                    backward_text = "+" + backward_text;
                }
                forward_text = with_arrow("(" + forward_text + ")", forward_arrow, forward_prepend_arrow);
                if (!backward_text.isEmpty()) {
                    backward_text = with_arrow("(" + backward_text + ")", backward_arrow, !forward_prepend_arrow);
                }
            } else {
                forward_text = with_arrow(forward_text, forward_arrow, forward_prepend_arrow);
                if (!backward_text.isEmpty()) {
                    backward_text = with_arrow(backward_text, backward_arrow, !forward_prepend_arrow);
                }
            }

            if (edge.swap_label_sides) {
                std::swap(forward_text, backward_text);
            }

            const QPointF center = edge_segment_midpoint(points, label_segment_index, label_segment_kind);
            double angle = std::atan2(tangent.y(), tangent.x()) * 180.0 / M_PI;
            if (angle > 90.0 || angle < -90.0) {
                angle += 180.0;
            }

            painter.save();
            painter.translate(center);
            painter.rotate(angle);
            painter.setPen(QPen(label_color_));

            const int text_gap = 4;
            const int text_height = metrics.height();

            if (!forward_text.isEmpty()) {
                const QRect forward_rect(-metrics.horizontalAdvance(forward_text) / 2,
                                        -text_gap - text_height,
                                        metrics.horizontalAdvance(forward_text),
                                        text_height);
                painter.drawText(forward_rect, Qt::AlignCenter, forward_text);
            }

            if (!backward_text.isEmpty()) {
                const QRect backward_rect(-metrics.horizontalAdvance(backward_text) / 2,
                                          text_gap,
                                          metrics.horizontalAdvance(backward_text),
                                          text_height);
                painter.drawText(backward_rect, Qt::AlignCenter, backward_text);
            }

            painter.restore();
            painter.setPen(edge_pen);
        }
    }

    for (int i = 0; i < static_cast<int>(network_data_.nodes.size()); ++i) {
        const NodeData& node = network_data_.nodes[i];
        const QPointF center(node.x, node.y);
        const QColor effective_fill = node.fill_color.isEmpty() ? node_fill_color_ : QColor(node.fill_color);
        const QColor effective_outline = node.outline_color.isEmpty() ? node_outline_color_ : QColor(node.outline_color);
        painter.setPen(QPen(effective_outline, node_outline_thickness_));
        painter.setBrush(QBrush(effective_fill));
        painter.drawEllipse(center, node_radius_, node_radius_);

        if (i == selected_node_) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 170, 0), 2.0, Qt::DashLine));
            painter.drawEllipse(center, node_radius_ + 5.0, node_radius_ + 5.0);
            painter.restore();
        }

        const double angle_radians = label_angle_degrees_ * M_PI / 180.0;
        const QPointF direction(std::cos(angle_radians), -std::sin(angle_radians));
        const QPointF label_anchor = center + direction * (node_radius_ + node_label_distance_);
        const QString display_name = node.name.isEmpty() ? node.label : node.name;

        painter.save();
        painter.translate(label_anchor);
        painter.rotate(-label_angle_degrees_);
        painter.setPen(QPen(label_color_));
        const QRectF text_rect(0.0f, -metrics.height() / 2.0f, 200.0f, metrics.height() + 4.0f);
        painter.drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, display_name);
        painter.restore();
    }
}


NetworkView::SegmentKindUi NetworkView::to_ui_segment_kind(EdgeData::SegmentKind kind) {
    switch (kind) {
    case EdgeData::SegmentKind::BendClockwise:
        return SegmentKindUi::BendClockwise;
    case EdgeData::SegmentKind::BendCounterClockwise:
        return SegmentKindUi::BendCounterClockwise;
    case EdgeData::SegmentKind::WiggleHorizontalFirst:
        return SegmentKindUi::WiggleHorizontalFirst;
    case EdgeData::SegmentKind::WiggleVerticalFirst:
        return SegmentKindUi::WiggleVerticalFirst;
    case EdgeData::SegmentKind::Straight:
    default:
        return SegmentKindUi::Straight;
    }
}

EdgeData::SegmentKind NetworkView::from_ui_segment_kind(SegmentKindUi kind) {
    switch (kind) {
    case SegmentKindUi::BendClockwise:
        return EdgeData::SegmentKind::BendClockwise;
    case SegmentKindUi::BendCounterClockwise:
        return EdgeData::SegmentKind::BendCounterClockwise;
    case SegmentKindUi::WiggleHorizontalFirst:
        return EdgeData::SegmentKind::WiggleHorizontalFirst;
    case SegmentKindUi::WiggleVerticalFirst:
        return EdgeData::SegmentKind::WiggleVerticalFirst;
    case SegmentKindUi::Straight:
    default:
        return EdgeData::SegmentKind::Straight;
    }
}

std::vector<QPointF> NetworkView::edge_polyline_points(const EdgeData& edge) const {
    std::vector<QPointF> points;
    if (edge.from_index < 0 || edge.to_index < 0 ||
        edge.from_index >= static_cast<int>(network_data_.nodes.size()) ||
        edge.to_index >= static_cast<int>(network_data_.nodes.size())) {
        return points;
    }
    points.emplace_back(network_data_.nodes[edge.from_index].x, network_data_.nodes[edge.from_index].y);
    for (const EdgeData::GuideNode& guide_node : edge.guide_nodes) {
        points.emplace_back(guide_node.x, guide_node.y);
    }
    points.emplace_back(network_data_.nodes[edge.to_index].x, network_data_.nodes[edge.to_index].y);
    return points;
}

void NetworkView::append_segment_path(QPainterPath& path, const QPointF& a, const QPointF& b, EdgeData::SegmentKind kind) const {
    const QPointF delta = b - a;
    const double length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (length < 1e-6 || kind == EdgeData::SegmentKind::Straight) {
        path.lineTo(b);
        return;
    }

    if (kind == EdgeData::SegmentKind::BendClockwise || kind == EdgeData::SegmentKind::BendCounterClockwise) {
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        const double adx = std::abs(dx);
        const double ady = std::abs(dy);
        const double tol = 1e-3;

        // If points are axis-aligned, keep the segment straight.
        if (adx < tol || ady < tol) {
            path.lineTo(b);
            return;
        }

        // Only draw quarter-circle bends for immediate diagonal grid neighbors.
        if (std::abs(adx - kGridSize) > tol || std::abs(ady - kGridSize) > tol) {
            path.lineTo(b);
            return;
        }

        auto to_qt_angle = [](const QPointF& v) {
            return std::atan2(-v.y(), v.x()) * 180.0 / M_PI;
        };

        auto normalize_delta = [](double delta) {
            while (delta <= -180.0) {
                delta += 360.0;
            }
            while (delta > 180.0) {
                delta -= 360.0;
            }
            return delta;
        };

        const bool want_clockwise = (kind == EdgeData::SegmentKind::BendClockwise);
        const std::array<QPointF, 2> candidate_centers{QPointF(a.x(), b.y()), QPointF(b.x(), a.y())};

        bool found = false;
        QPointF chosen_center;
        double chosen_start_angle = 0.0;
        double chosen_sweep = 0.0;

        for (const QPointF& center : candidate_centers) {
            const QPointF va = a - center;
            const QPointF vb = b - center;
            const double ra = std::sqrt(va.x() * va.x() + va.y() * va.y());
            const double rb = std::sqrt(vb.x() * vb.x() + vb.y() * vb.y());
            if (std::abs(ra - kGridSize) > tol || std::abs(rb - kGridSize) > tol) {
                continue;
            }

            const double start_angle = to_qt_angle(va);
            const double end_angle = to_qt_angle(vb);
            const double delta_angle = normalize_delta(end_angle - start_angle);

            if (std::abs(std::abs(delta_angle) - 90.0) > 1e-2) {
                continue;
            }

            if ((want_clockwise && delta_angle < 0.0) || (!want_clockwise && delta_angle > 0.0)) {
                chosen_center = center;
                chosen_start_angle = start_angle;
                chosen_sweep = want_clockwise ? -90.0 : 90.0;
                found = true;
                break;
            }
        }

        if (!found) {
            path.lineTo(b);
            return;
        }

        if (path.currentPosition() != a) {
            path.lineTo(a);
        }

        const QRectF arc_rect(chosen_center.x() - kGridSize,
                              chosen_center.y() - kGridSize,
                              2.0 * kGridSize,
                              2.0 * kGridSize);
        path.arcTo(arc_rect, chosen_start_angle, chosen_sweep);
        return;
    }

    if (kind == EdgeData::SegmentKind::WiggleHorizontalFirst || kind == EdgeData::SegmentKind::WiggleVerticalFirst) {
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        const double adx = std::abs(dx);
        const double ady = std::abs(dy);
        const double tol = 1e-3;

        // Wiggles are only defined for immediate diagonal neighbors.
        if (adx < tol || ady < tol || std::abs(adx - kGridSize) > tol || std::abs(ady - kGridSize) > tol) {
            path.lineTo(b);
            return;
        }

        auto to_qt_angle = [](const QPointF& v) {
            return std::atan2(-v.y(), v.x()) * 180.0 / M_PI;
        };

        auto normalize_delta = [](double delta) {
            while (delta <= -180.0) {
                delta += 360.0;
            }
            while (delta > 180.0) {
                delta -= 360.0;
            }
            return delta;
        };

        auto append_quarter_arc = [&](const QPointF& center, const QPointF& from, const QPointF& to, double radius) {
            const QPointF v_from = from - center;
            const QPointF v_to = to - center;
            const double start_angle = to_qt_angle(v_from);
            const double sweep = normalize_delta(to_qt_angle(v_to) - start_angle);
            if (std::abs(std::abs(sweep) - 90.0) > 1e-2) {
                return false;
            }
            const QRectF rect(center.x() - radius, center.y() - radius, 2.0 * radius, 2.0 * radius);
            path.arcTo(rect, start_angle, sweep);
            return true;
        };

        const double sx = (dx > 0.0) ? 1.0 : -1.0;
        const double sy = (dy > 0.0) ? 1.0 : -1.0;
        const double radius = 0.5 * kGridSize;
        const QPointF mid(a.x() + sx * radius, a.y() + sy * radius);

        QPointF c1;
        QPointF c2;
        if (kind == EdgeData::SegmentKind::WiggleHorizontalFirst) {
            c1 = QPointF(a.x(), a.y() + sy * radius);
            c2 = QPointF(b.x(), b.y() - sy * radius);
        } else {
            c1 = QPointF(a.x() + sx * radius, a.y());
            c2 = QPointF(b.x() - sx * radius, b.y());
        }

        if (path.currentPosition() != a) {
            path.lineTo(a);
        }

        if (!append_quarter_arc(c1, a, mid, radius) || !append_quarter_arc(c2, mid, b, radius)) {
            // Fallback in degenerate cases.
            path.lineTo(b);
        }
        return;
    }

    path.lineTo(b);
}

QPointF NetworkView::edge_segment_midpoint(const std::vector<QPointF>& points, int segment_index, EdgeData::SegmentKind kind) const {
    if (segment_index < 0 || segment_index + 1 >= static_cast<int>(points.size())) {
        return QPointF();
    }
    const QPointF a = points[segment_index];
    const QPointF b = points[segment_index + 1];
    const QPointF d = b - a;
    const double len = std::sqrt(d.x() * d.x() + d.y() * d.y());
    if (len < 1e-6 || kind == EdgeData::SegmentKind::Straight) {
        return (a + b) * 0.5;
    }

    const QPointF t = d / len;
    const QPointF n(-t.y(), t.x());

    if (kind == EdgeData::SegmentKind::BendClockwise || kind == EdgeData::SegmentKind::BendCounterClockwise) {
        const double sign = (kind == EdgeData::SegmentKind::BendClockwise) ? -1.0 : 1.0;
        return (a + b) * 0.5 + n * (kGridSize * sign);
    }

    const double first_sign = (kind == EdgeData::SegmentKind::WiggleHorizontalFirst) ? 1.0 : -1.0;
    return (a + b) * 0.5 + n * (kGridSize * 0.25 * first_sign);
}

bool NetworkView::edge_segment_tangent(const std::vector<QPointF>& points, int segment_index, EdgeData::SegmentKind kind, QPointF& out_tangent) const {
    if (segment_index < 0 || segment_index + 1 >= static_cast<int>(points.size())) {
        return false;
    }
    const QPointF d = points[segment_index + 1] - points[segment_index];
    const double len = std::sqrt(d.x() * d.x() + d.y() * d.y());
    if (len < 1e-6) {
        return false;
    }
    QPointF t = d / len;
    const QPointF n(-t.y(), t.x());

    if (kind == EdgeData::SegmentKind::BendClockwise || kind == EdgeData::SegmentKind::BendCounterClockwise) {
        const double sign = (kind == EdgeData::SegmentKind::BendClockwise) ? -1.0 : 1.0;
        t = t + n * (0.8 * sign);
    } else if (kind == EdgeData::SegmentKind::WiggleHorizontalFirst || kind == EdgeData::SegmentKind::WiggleVerticalFirst) {
        const double first_sign = (kind == EdgeData::SegmentKind::WiggleHorizontalFirst) ? 1.0 : -1.0;
        t = t + n * (0.5 * first_sign);
    }

    const double tl = std::sqrt(t.x() * t.x() + t.y() * t.y());
    if (tl > 1e-6) {
        t /= tl;
    }
    out_tangent = t;
    return true;
}

void NetworkView::resizeGL(int width, int height) {
    qInfo() << "NetworkView resized" << width << "x" << height;
    fit_network_to_viewport();
}

void NetworkView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        panning_ = true;
        last_pan_pos_ = event->pos();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const QPointF world = to_world(event->pos());
        dragged_guide_node_ = -1;
        dragged_node_ = pick_node(world);
        if (dragged_node_ >= 0) {
            selected_node_ = dragged_node_;
            selected_edge_ = -1;
            const NodeData& node = network_data_.nodes[selected_node_];
            drag_offset_ = world - QPointF(node.x, node.y);
        } else {
            selected_node_ = -1;
            selected_edge_ = pick_edge(world);
            if (selected_edge_ >= 0 && selected_edge_ < static_cast<int>(network_data_.edges.size())) {
                const EdgeData& edge = network_data_.edges[selected_edge_];
                double best = 1e9;
                for (int i = 0; i < static_cast<int>(edge.guide_nodes.size()); ++i) {
                    const QPointF guide_pos(edge.guide_nodes[i].x, edge.guide_nodes[i].y);
                    const QPointF d = world - guide_pos;
                    const double dist = std::sqrt(d.x() * d.x() + d.y() * d.y());
                    if (dist <= std::max(10.0, line_thickness_ * 1.5) && dist < best) {
                        best = dist;
                        dragged_guide_node_ = i;
                    }
                }
            }
        }
        emit selection_changed();
        update();
    }
    QOpenGLWidget::mousePressEvent(event);
}

void NetworkView::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        const QPoint delta = event->pos() - last_pan_pos_;
        pan_offset_ += QPointF(delta.x(), delta.y());
        last_pan_pos_ = event->pos();
        update();
        return;
    }

    if (dragged_node_ >= 0 && dragged_node_ < static_cast<int>(network_data_.nodes.size())) {
        const QPointF world = to_world(event->pos());
        NodeData& node = network_data_.nodes[dragged_node_];
        node.x = static_cast<float>(world.x() - drag_offset_.x());
        node.y = static_cast<float>(world.y() - drag_offset_.y());
        snap_node_to_grid(node);
        update();
    } else if (selected_edge_ >= 0 && selected_edge_ < static_cast<int>(network_data_.edges.size()) &&
               dragged_guide_node_ >= 0 && dragged_guide_node_ < static_cast<int>(network_data_.edges[selected_edge_].guide_nodes.size())) {
        const QPointF world = to_world(event->pos());
        EdgeData::GuideNode& guide_node = network_data_.edges[selected_edge_].guide_nodes[dragged_guide_node_];
        guide_node.x = static_cast<float>(std::round(world.x() / kGridSize) * kGridSize);
        guide_node.y = static_cast<float>(std::round(world.y() / kGridSize) * kGridSize);
        update();
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void NetworkView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        panning_ = false;
    }
    if (event->button() == Qt::LeftButton) {
        const bool had_drag = (dragged_node_ >= 0 || dragged_guide_node_ >= 0);
        dragged_node_ = -1;
        dragged_guide_node_ = -1;
        if (had_drag) {
            emit selection_changed();
        }
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void NetworkView::wheelEvent(QWheelEvent* event) {
    const float factor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    const float new_zoom = std::clamp(zoom_scale_ * factor, 0.25f, 4.0f);
    if (std::abs(new_zoom - zoom_scale_) < 0.0001f) {
        return;
    }

    const QPointF cursor = event->position();
    const QPointF before = to_world(cursor);
    zoom_scale_ = new_zoom;
    const QPointF after = to_world(cursor);
    pan_offset_ += (after - before) * zoom_scale_;
    update();
}

QRectF NetworkView::scene_bounds() const {
    if (network_data_.nodes.empty()) {
        return QRectF();
    }

    QRectF bounds;
    bool initialized = false;
    for (const NodeData& node : network_data_.nodes) {
        QRectF node_rect(node.x - node_radius_ - 100.0, node.y - node_radius_ - 40.0,
                         220.0 + 2 * node_radius_, 140.0 + 2 * node_radius_);
        if (!initialized) {
            bounds = node_rect;
            initialized = true;
        } else {
            bounds = bounds.united(node_rect);
        }
    }

    for (int edge_index = 0; edge_index < static_cast<int>(network_data_.edges.size()); ++edge_index) {
        const EdgeData& edge = network_data_.edges[edge_index];
        if (edge.from_index < 0 || edge.to_index < 0 ||
            edge.from_index >= static_cast<int>(network_data_.nodes.size()) ||
            edge.to_index >= static_cast<int>(network_data_.nodes.size())) {
            continue;
        }
        const std::vector<QPointF> points = edge_polyline_points(edge);
        for (int seg = 0; seg + 1 < static_cast<int>(points.size()); ++seg) {
            bounds = bounds.united(QRectF(points[seg], points[seg + 1]).normalized().adjusted(-20, -20, 20, 20));
        }
        for (const EdgeData::GuideNode& guide_node : edge.guide_nodes) {
            bounds = bounds.united(QRectF(guide_node.x - 10.0, guide_node.y - 10.0, 20.0, 20.0));
        }
    }
    return bounds;
}

void NetworkView::fit_network_to_viewport() {
    if (network_data_.nodes.empty() || width() <= 0 || height() <= 0) {
        return;
    }

    float min_x = network_data_.nodes.front().x;
    float max_x = network_data_.nodes.front().x;
    float min_y = network_data_.nodes.front().y;
    float max_y = network_data_.nodes.front().y;

    for (const NodeData& node : network_data_.nodes) {
        min_x = std::min(min_x, node.x);
        max_x = std::max(max_x, node.x);
        min_y = std::min(min_y, node.y);
        max_y = std::max(max_y, node.y);
    }

    const float bounds_w = std::max(1.0f, max_x - min_x);
    const float bounds_h = std::max(1.0f, max_y - min_y);
    const float margin = node_radius_ + 30.0f;
    const float target_w = std::max(1.0f, static_cast<float>(width()) - 2.0f * margin);
    const float target_h = std::max(1.0f, static_cast<float>(height()) - 2.0f * margin);

    const bool likely_normalized = max_x <= 2.0f && max_y <= 2.0f;
    const bool outside_viewport = min_x < 0.0f || min_y < 0.0f || max_x > width() || max_y > height();

    if (!likely_normalized && !outside_viewport) {
        return;
    }

    const float scale = std::min(target_w / bounds_w, target_h / bounds_h);
    for (NodeData& node : network_data_.nodes) {
        node.x = margin + (node.x - min_x) * scale;
        node.y = margin + (node.y - min_y) * scale;
        snap_node_to_grid(node);
    }

    pan_offset_ = QPointF(0.0, 0.0);
    zoom_scale_ = 1.0f;
    qInfo() << "Network positions fitted into viewport with scale" << scale;
}

void NetworkView::snap_node_to_grid(NodeData& node) const {
    node.x = std::round(node.x / kGridSize) * kGridSize;
    node.y = std::round(node.y / kGridSize) * kGridSize;
}

void NetworkView::snap_all_nodes_to_grid() {
    for (NodeData& node : network_data_.nodes) {
        snap_node_to_grid(node);
    }
}

int NetworkView::pick_node(const QPointF& world_point) const {
    for (int i = static_cast<int>(network_data_.nodes.size()) - 1; i >= 0; --i) {
        const NodeData& node = network_data_.nodes[i];
        const double dx = static_cast<double>(node.x) - world_point.x();
        const double dy = static_cast<double>(node.y) - world_point.y();
        if ((dx * dx + dy * dy) <= static_cast<double>(node_radius_ * node_radius_)) {
            return i;
        }
    }
    return -1;
}


int NetworkView::pick_edge(const QPointF& world_point) const {
    int best_index = -1;
    double best_distance = 1e9;
    const double max_distance = std::max(10.0, line_thickness_ * 1.5);

    for (int i = 0; i < static_cast<int>(network_data_.edges.size()); ++i) {
        const EdgeData& edge = network_data_.edges[i];
        const std::vector<QPointF> points = edge_polyline_points(edge);
        if (points.size() < 2) {
            continue;
        }

        for (int seg = 0; seg + 1 < static_cast<int>(points.size()); ++seg) {
            const QPointF a = points[seg];
            const QPointF b = points[seg + 1];
            const QPointF ab = b - a;
            const double ab_len2 = ab.x() * ab.x() + ab.y() * ab.y();
            if (ab_len2 <= 1e-9) {
                continue;
            }
            const QPointF ap = world_point - a;
            const double t = std::clamp((ap.x() * ab.x() + ap.y() * ab.y()) / ab_len2, 0.0, 1.0);
            const QPointF closest = a + t * ab;
            const QPointF diff = world_point - closest;
            const double dist = std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
            if (dist <= max_distance && dist < best_distance) {
                best_distance = dist;
                best_index = i;
            }
        }
    }
    return best_index;
}

QPointF NetworkView::to_world(const QPointF& screen_point) const {
    return (screen_point - pan_offset_) / zoom_scale_;
}
