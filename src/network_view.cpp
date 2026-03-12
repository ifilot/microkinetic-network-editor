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
#include <QPen>
#include <QRectF>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

NetworkView::NetworkView(QWidget* parent)
    : QOpenGLWidget(parent) {
    setMinimumSize(900, 600);
    setMouseTracking(true);
}

void NetworkView::set_network(NetworkData data) {
    qInfo() << "Applying network data" << data.nodes.size() << "nodes" << data.edges.size() << "edges";
    network_data_ = std::move(data);
    fit_network_to_viewport();
    snap_all_nodes_to_grid();
    selected_node_ = -1;
    selected_edge_ = -1;
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

        const NodeData& from = network_data_.nodes[edge.from_index];
        const NodeData& to = network_data_.nodes[edge.to_index];
        const QPointF p1(from.x, from.y);
        const QPointF p2(to.x, to.y);
        const QColor effective_edge_color = edge.color.isEmpty() ? line_color_ : QColor(edge.color);
        edge_pen.setColor(effective_edge_color);
        painter.setPen(edge_pen);
        painter.drawLine(p1, p2);

        if (edge_index == selected_edge_) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 170, 0), 2.0, Qt::DashLine));
            const QRectF bounds = QRectF(p1, p2).normalized().adjusted(-8.0, -8.0, 8.0, 8.0);
            painter.drawRect(bounds);
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

            const bool edge_goes_left = p2.x() < p1.x();
            const bool edge_is_vertical = qAbs(p2.x() - p1.x()) < 1e-6;

            const QString forward_arrow = edge_goes_left ? "◀" : "▶";
            const QString backward_arrow = edge_goes_left ? "▶" : "◀";
            bool forward_prepend_arrow = false;
            if (edge_is_vertical) {
                // Vertical edge: always render forward as "value + arrow"
                // and backward as "arrow + value".
                forward_prepend_arrow = false;
            } else if (edge_goes_left) {
                forward_prepend_arrow = true;
            }

            auto with_arrow = [](const QString& value, const QString& arrow, bool prepend) {
                return prepend ? (arrow + " " + value) : (value + " " + arrow);
            };

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

            const QString edge_type = edge.type.trimmed().toLower();
            if (edge_type == "ads") {
                // Adsorption: keep directional cue and only display backward barrier.
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
                // Rearrangements: keep arrows and wrap values in parentheses.
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

            const QPointF center = (p1 + p2) / 2.0;
            double angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x()) * 180.0 / M_PI;
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
        dragged_node_ = pick_node(world);
        if (dragged_node_ >= 0) {
            selected_node_ = dragged_node_;
            selected_edge_ = -1;
            const NodeData& node = network_data_.nodes[selected_node_];
            drag_offset_ = world - QPointF(node.x, node.y);
        } else {
            selected_node_ = -1;
            selected_edge_ = pick_edge(world);
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
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void NetworkView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        panning_ = false;
    }
    if (event->button() == Qt::LeftButton) {
        dragged_node_ = -1;
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
        const NodeData& from = network_data_.nodes[edge.from_index];
        const NodeData& to = network_data_.nodes[edge.to_index];
        bounds = bounds.united(QRectF(QPointF(from.x, from.y), QPointF(to.x, to.y)).normalized().adjusted(-20, -20, 20, 20));
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
        if (edge.from_index < 0 || edge.to_index < 0 ||
            edge.from_index >= static_cast<int>(network_data_.nodes.size()) ||
            edge.to_index >= static_cast<int>(network_data_.nodes.size())) {
            continue;
        }
        const QPointF a(network_data_.nodes[edge.from_index].x, network_data_.nodes[edge.from_index].y);
        const QPointF b(network_data_.nodes[edge.to_index].x, network_data_.nodes[edge.to_index].y);
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
    return best_index;
}

QPointF NetworkView::to_world(const QPointF& screen_point) const {
    return (screen_point - pan_offset_) / zoom_scale_;
}
