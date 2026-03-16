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

#include "network_io.h"

#include <QDebug>

#include <yaml-cpp/yaml.h>

#include <QString>

#include <cmath>
#include <algorithm>
#include <exception>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

namespace {

constexpr float kGridSpacing = 140.0f;
constexpr int kGridColumns = 4;

/**
 * @brief Test whether a candidate position is too close to any already placed node.
 *
 * The function computes squared Euclidean distances against each node and compares them
 * with the provided minimum threshold, avoiding square-root work during repeated checks.
 *
 * @param x Candidate x-coordinate for a node center.
 * @param y Candidate y-coordinate for a node center.
 * @param nodes Nodes that are already considered placed.
 * @param minimum_distance_squared Squared minimum allowed center-to-center distance.
 * @return True when the candidate overlaps an existing node distance constraint.
 */
bool overlaps_existing_node(float x, float y, const std::vector<NodeData>& nodes, float minimum_distance_squared) {
    for (const NodeData& node : nodes) {
        const float dx = x - node.x;
        const float dy = y - node.y;
        const float distance_squared = dx * dx + dy * dy;
        if (distance_squared < minimum_distance_squared) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Assign grid-based fallback positions to nodes missing explicit coordinates.
 *
 * Pre-positioned nodes are copied first, then unpositioned nodes are placed on a fixed
 * column grid while skipping slots that violate a radius-derived spacing threshold. This
 * mutates `data.nodes` in place so all nodes have usable coordinates after loading.
 *
 * @param data Network data whose node coordinates are completed in place.
 * @param has_position Flags indicating which node entries already had coordinates in YAML.
 * @param node_radius Radius used to derive a minimum spacing between generated positions.
 */
void assign_default_positions(NetworkData& data, const std::vector<bool>& has_position, float node_radius) {
    std::vector<NodeData> placed_nodes;
    placed_nodes.reserve(data.nodes.size());

    for (size_t i = 0; i < data.nodes.size(); ++i) {
        if (has_position[i]) {
            placed_nodes.push_back(data.nodes[i]);
        }
    }

    const float minimum_distance = std::max(2.0f * node_radius, 1.0f);
    const float minimum_distance_squared = minimum_distance * minimum_distance;

    size_t next_slot = 0;
    for (size_t i = 0; i < data.nodes.size(); ++i) {
        if (has_position[i]) {
            continue;
        }

        while (true) {
            const int col = static_cast<int>(next_slot % kGridColumns);
            const int row = static_cast<int>(next_slot / kGridColumns);
            const float x = 100.0f + static_cast<float>(col) * kGridSpacing;
            const float y = 100.0f + static_cast<float>(row) * kGridSpacing;
            ++next_slot;

            if (overlaps_existing_node(x, y, placed_nodes, minimum_distance_squared)) {
                continue;
            }

            data.nodes[i].x = x;
            data.nodes[i].y = y;
            placed_nodes.push_back(data.nodes[i]);
            break;
        }
    }
}

/**
 * @brief Read a color-like scalar field from a YAML map with fallback behavior.
 *
 * If the key exists it is converted to a QString; otherwise the provided fallback is
 * returned unchanged.
 *
 * @param node YAML map node to inspect.
 * @param key Key name to retrieve.
 * @param fallback Value returned when the key is absent.
 * @return Parsed string value or fallback.
 */
QString color_or_default(const YAML::Node& node, const char* key, const QString& fallback) {
    if (node[key]) {
        return QString::fromStdString(node[key].as<std::string>());
    }
    return fallback;
}

/**
 * @brief Fetch a child entry from a YAML map only when the parent is a valid map node.
 *
 * This helper guards key access by returning an empty node for null or non-map parents,
 * allowing callers to chain optional lookups safely.
 *
 * @param node Parent YAML node expected to be a map.
 * @param key Child key to retrieve.
 * @return Requested child node or an empty node when unavailable.
 */
YAML::Node map_child(const YAML::Node& node, const char* key) {
    if (!node || !node.IsMap()) {
        return YAML::Node();
    }
    return node[key];
}

/**
 * @brief Convert a scalar YAML node to QString with a tolerant fallback path.
 *
 * The function returns an empty string for non-scalars, first attempts typed conversion to
 * `std::string`, and falls back to raw scalar text if conversion throws.
 *
 * @param node YAML node expected to hold scalar text.
 * @return Converted QString, or empty when the node is missing/non-scalar.
 */
QString scalar_to_qstring(const YAML::Node& node) {
    if (!node || !node.IsScalar()) {
        return QString();
    }

    try {
        return QString::fromStdString(node.as<std::string>());
    } catch (...) {
        return QString::fromStdString(node.Scalar());
    }
}

/**
 * @brief Flatten scalar or nested-sequence YAML values into a QString list.
 *
 * Sequences are traversed recursively and scalar entries are appended when non-empty after
 * conversion. This lets edge value fields accept either single values or arbitrarily nested
 * arrays in input YAML.
 *
 * @param value YAML node containing scalar or sequence values.
 * @param out_values Destination list that receives parsed scalar strings.
 */
void append_value_node(const YAML::Node& value, std::vector<QString>& out_values) {
    if (!value) {
        return;
    }

    if (value.IsSequence()) {
        for (size_t i = 0; i < value.size(); ++i) {
            append_value_node(value[i], out_values);
        }
        return;
    }

    const QString parsed = scalar_to_qstring(value);
    if (!parsed.isEmpty()) {
        out_values.push_back(parsed);
    }
}


/**
 * @brief Map an edge segment kind enum to its YAML string token.
 *
 * The mapping keeps serialization stable for bend/wiggle variants and defaults unknown
 * values to `straight`.
 *
 * @param kind Segment routing mode to encode.
 * @return Canonical YAML token for the segment kind.
 */
QString segment_kind_to_string(EdgeData::SegmentKind kind) {
    switch (kind) {
    case EdgeData::SegmentKind::BendClockwise:
        return "bend_cw";
    case EdgeData::SegmentKind::BendCounterClockwise:
        return "bend_ccw";
    case EdgeData::SegmentKind::WiggleHorizontalFirst:
        return "wiggle_horizontal_first";
    case EdgeData::SegmentKind::WiggleVerticalFirst:
        return "wiggle_vertical_first";
    case EdgeData::SegmentKind::Straight:
    default:
        return "straight";
    }
}

/**
 * @brief Parse a YAML segment type token into the internal segment-kind enum.
 *
 * Input text is trimmed and lowercased, supports legacy aliases for clockwise and
 * counterclockwise bends, and falls back to straight segments for unknown values.
 *
 * @param kind_raw Raw segment token read from YAML.
 * @return Parsed segment kind, or Straight when unrecognized.
 */
EdgeData::SegmentKind segment_kind_from_string(const QString& kind_raw) {
    const QString kind = kind_raw.trimmed().toLower();
    if (kind == "bend_cw" || kind == "clockwise") {
        return EdgeData::SegmentKind::BendClockwise;
    }
    if (kind == "bend_ccw" || kind == "counterclockwise") {
        return EdgeData::SegmentKind::BendCounterClockwise;
    }
    if (kind == "wiggle_horizontal_first") {
        return EdgeData::SegmentKind::WiggleHorizontalFirst;
    }
    if (kind == "wiggle_vertical_first") {
        return EdgeData::SegmentKind::WiggleVerticalFirst;
    }
    return EdgeData::SegmentKind::Straight;
}

/**
 * @brief Expand a scalar adsorption energy into forward/backward edge values.
 *
 * For adsorption-style edges, this helper appends a zero forward barrier and the negative
 * adsorption energy as the backward value to match the editor's expected two-value format.
 *
 * @param ads_node YAML scalar containing an adsorption energy value.
 * @param out_values Destination list receiving generated forward/backward entries.
 */
void append_forward_backward_from_ads(const YAML::Node& ads_node, std::vector<QString>& out_values) {
    if (!ads_node || !ads_node.IsScalar()) {
        return;
    }

    const double ads = ads_node.as<double>();
    out_values.push_back(QString::number(0.0, 'g', 16));
    out_values.push_back(QString::number(-ads, 'g', 16));
}

}  // namespace

/**
 * @brief Parse a network YAML file and populate node, edge, and view-setting data.
 *
 * The loader validates required fields, resolves edge labels to node indices, normalizes
 * optional edge/value encodings, and assigns fallback node coordinates when positions are
 * missing. Parsing and validation errors are returned through `error` and a false result.
 *
 * @param path Path to the YAML file to read from disk.
 * @param out_data Destination structure that receives parsed network content on success.
 * @param error Output string receiving a human-readable error when loading fails.
 * @return True when parsing and conversion succeed; false otherwise.
 */
bool load_network_yaml(const QString& path, NetworkData& out_data, QString& error) {
    qInfo() << "Loading network YAML from" << path;
    try {
        YAML::Node root = YAML::LoadFile(path.toStdString());
        YAML::Node node_list = root["nodes"];
        YAML::Node edge_list = root["edges"];

        if (!node_list || !node_list.IsSequence()) {
            error = "YAML field 'nodes' is missing or invalid.";
            return false;
        }
        if (!edge_list || !edge_list.IsSequence()) {
            error = "YAML field 'edges' is missing or invalid.";
            return false;
        }

        NetworkData data;
        std::unordered_map<std::string, int> index_by_label;
        std::vector<bool> has_position;
        has_position.reserve(node_list.size());

        std::unordered_set<std::string> labels_in_edges;
        labels_in_edges.reserve(edge_list.size() * 2);
        for (size_t i = 0; i < edge_list.size(); ++i) {
            const YAML::Node edge = edge_list[i];
            const YAML::Node edge_nodes = map_child(edge, "nodes");
            if (!edge_nodes || !edge_nodes.IsSequence()) {
                error = "An edge has an invalid 'nodes' entry (expected a sequence of two labels).";
                return false;
            }
            if (edge_nodes.size() > 2) {
                error = QString("Edge %1 contains more than two nodes (%2). Only pairwise edges are supported.")
                    .arg(i + 1)
                    .arg(static_cast<int>(edge_nodes.size()));
                return false;
            }
            if (edge_nodes.size() < 2) {
                error = QString("Edge %1 has fewer than two nodes.").arg(i + 1);
                return false;
            }

            labels_in_edges.insert(edge_nodes[0].as<std::string>());
            labels_in_edges.insert(edge_nodes[1].as<std::string>());
        }

        YAML::Node settings = root["settings"];
        const float node_radius = (settings && settings.IsMap() && settings["node_radius"]) ?
            settings["node_radius"].as<float>() : data.settings.node_radius;

        for (size_t i = 0; i < node_list.size(); ++i) {
            const YAML::Node yaml_node = node_list[i];
            const YAML::Node label_node = map_child(yaml_node, "label");
            if (!label_node || !label_node.IsScalar()) {
                error = "A node is missing the required 'label' field.";
                return false;
            }

            NodeData node;
            node.label = scalar_to_qstring(label_node);

            if (labels_in_edges.find(node.label.toStdString()) == labels_in_edges.end()) {
                qWarning() << "Ignoring unconnected node with label:" << node.label;
                continue;
            }

            const QString maybe_name = scalar_to_qstring(map_child(yaml_node, "name"));
            node.name = maybe_name.isEmpty() ? node.label : maybe_name;
            node.structure = scalar_to_qstring(map_child(yaml_node, "structure")).trimmed();
            node.fill_color = scalar_to_qstring(map_child(yaml_node, "fill_color")).trimmed();
            node.outline_color = scalar_to_qstring(map_child(yaml_node, "outline_color")).trimmed();

            if (map_child(yaml_node, "position") && map_child(yaml_node, "position").IsMap() &&
                map_child(map_child(yaml_node, "position"), "x") && map_child(map_child(yaml_node, "position"), "y")) {
                node.x = map_child(map_child(yaml_node, "position"), "x").as<float>();
                node.y = map_child(map_child(yaml_node, "position"), "y").as<float>();
                has_position.push_back(true);
            } else {
                has_position.push_back(false);
            }

            index_by_label[node.label.toStdString()] = static_cast<int>(data.nodes.size());
            data.nodes.push_back(node);
        }

        for (size_t i = 0; i < edge_list.size(); ++i) {
            const YAML::Node edge = edge_list[i];
            const YAML::Node edge_nodes = map_child(edge, "nodes");
            const std::string from_label = edge_nodes[0].as<std::string>();
            const std::string to_label = edge_nodes[1].as<std::string>();

            const auto from_it = index_by_label.find(from_label);
            const auto to_it = index_by_label.find(to_label);
            if (from_it == index_by_label.end() || to_it == index_by_label.end()) {
                error = "An edge references an unknown node label.";
                return false;
            }

            EdgeData edge_data;
            edge_data.from_index = from_it->second;
            edge_data.to_index = to_it->second;
            edge_data.type = scalar_to_qstring(map_child(edge, "type")).trimmed();
            edge_data.description = scalar_to_qstring(map_child(edge, "name")).trimmed();
            edge_data.structure = scalar_to_qstring(map_child(edge, "structure")).trimmed();
            edge_data.color = scalar_to_qstring(map_child(edge, "color")).trimmed();

            append_value_node(map_child(edge, "values"), edge_data.values);

            if (edge_data.values.empty()) {
                append_value_node(map_child(edge, "forward"), edge_data.values);
                append_value_node(map_child(edge, "backward"), edge_data.values);
            }

            if (edge_data.values.empty()) {
                const QString edge_type = edge_data.type.toLower();
                if (edge_type == "ads") {
                    append_forward_backward_from_ads(map_child(edge, "ads"), edge_data.values);
                } else {
                    append_value_node(map_child(edge, "ads"), edge_data.values);
                }
            }

            edge_data.swap_label_sides = map_child(edge, "swap_label_sides") ? map_child(edge, "swap_label_sides").as<bool>() : false;
            edge_data.label_segment_index = map_child(edge, "label_segment") ? map_child(edge, "label_segment").as<int>() : 0;

            const YAML::Node guide_nodes = map_child(edge, "guide_nodes");
            if (guide_nodes && guide_nodes.IsSequence()) {
                for (size_t guide_index = 0; guide_index < guide_nodes.size(); ++guide_index) {
                    const YAML::Node guide_node = guide_nodes[guide_index];
                    if (!guide_node.IsMap() || !guide_node["x"] || !guide_node["y"]) {
                        continue;
                    }
                    EdgeData::GuideNode parsed_guide_node;
                    parsed_guide_node.x = guide_node["x"].as<float>();
                    parsed_guide_node.y = guide_node["y"].as<float>();
                    edge_data.guide_nodes.push_back(parsed_guide_node);
                }
            }

            const YAML::Node segment_types = map_child(edge, "segment_types");
            if (segment_types && segment_types.IsSequence()) {
                for (size_t segment_index = 0; segment_index < segment_types.size(); ++segment_index) {
                    edge_data.segment_kinds.push_back(segment_kind_from_string(scalar_to_qstring(segment_types[segment_index])));
                }
            }
            edge_data.segment_kinds.resize(edge_data.guide_nodes.size() + 1, EdgeData::SegmentKind::Straight);
            edge_data.label_segment_index = std::clamp(edge_data.label_segment_index, 0, std::max(0, static_cast<int>(edge_data.segment_kinds.size()) - 1));

            data.edges.push_back(edge_data);
        }

        assign_default_positions(data, has_position, node_radius);

        if (settings && settings.IsMap()) {
            data.settings.node_radius = settings["node_radius"] ? settings["node_radius"].as<float>() : data.settings.node_radius;
            data.settings.line_thickness = settings["line_thickness"] ? settings["line_thickness"].as<float>() : data.settings.line_thickness;
            data.settings.node_outline_thickness = settings["node_outline_thickness"] ? settings["node_outline_thickness"].as<float>() : data.settings.node_outline_thickness;
            data.settings.font_size = settings["font_size"] ? settings["font_size"].as<float>() : data.settings.font_size;
            data.settings.label_angle_degrees = settings["label_angle_degrees"] ? settings["label_angle_degrees"].as<float>() : data.settings.label_angle_degrees;
            data.settings.node_label_distance = settings["node_label_distance"] ? settings["node_label_distance"].as<float>() : data.settings.node_label_distance;
            data.settings.font_family = settings["font_family"] ? QString::fromStdString(settings["font_family"].as<std::string>()) : data.settings.font_family;
            data.settings.background_color = color_or_default(settings, "background_color", data.settings.background_color);
            data.settings.node_fill_color = color_or_default(settings, "node_fill_color", data.settings.node_fill_color);
            data.settings.node_outline_color = color_or_default(settings, "node_outline_color", data.settings.node_outline_color);
            data.settings.line_color = color_or_default(settings, "line_color", data.settings.line_color);
            data.settings.label_color = color_or_default(settings, "label_color", data.settings.label_color);
            data.settings.value_decimals = settings["value_decimals"] ? settings["value_decimals"].as<int>() : data.settings.value_decimals;
            data.settings.value_unit = settings["value_unit"] ? QString::fromStdString(settings["value_unit"].as<std::string>()) : data.settings.value_unit;
            data.has_settings = true;
        }

        out_data = std::move(data);
        qInfo() << "Successfully loaded network:" << out_data.nodes.size() << "nodes," << out_data.edges.size() << "edges";
        return true;
    } catch (const std::exception& ex) {
        error = QString::fromUtf8(ex.what());
        qWarning() << "Exception while loading YAML" << error;
        return false;
    }
}

/**
 * @brief Convert network data structures into a YAML document string.
 *
 * The serializer emits nodes, pairwise edges, guide-node geometry, segment types, and view
 * settings while applying effective fallback colors when per-element colors are empty. It
 * skips edges that reference invalid node indices and reports emitter errors via `error`.
 *
 * @param data Source network data to serialize.
 * @param yaml_text Output string that receives the generated YAML text on success.
 * @param error Output string receiving serialization errors.
 * @return True when YAML emission succeeds; false otherwise.
 */
bool network_yaml_to_string(const NetworkData& data, QString& yaml_text, QString& error) {
    try {
        YAML::Node root;
        YAML::Node nodes(YAML::NodeType::Sequence);
        for (const NodeData& node : data.nodes) {
            YAML::Node node_yaml;
            node_yaml["label"] = node.label.toStdString();
            if (!node.name.isEmpty() && node.name != node.label) {
                node_yaml["name"] = node.name.toStdString();
            }
            if (!node.structure.trimmed().isEmpty()) {
                node_yaml["structure"] = node.structure.trimmed().toStdString();
            }
            const QString effective_fill_color = node.fill_color.trimmed().isEmpty()
                ? data.settings.node_fill_color
                : node.fill_color.trimmed();
            const QString effective_outline_color = node.outline_color.trimmed().isEmpty()
                ? data.settings.node_outline_color
                : node.outline_color.trimmed();
            node_yaml["fill_color"] = effective_fill_color.toStdString();
            node_yaml["outline_color"] = effective_outline_color.toStdString();
            node_yaml["position"]["x"] = node.x;
            node_yaml["position"]["y"] = node.y;
            nodes.push_back(node_yaml);
        }

        YAML::Node edges(YAML::NodeType::Sequence);
        for (const EdgeData& edge : data.edges) {
            if (edge.from_index < 0 || edge.to_index < 0 ||
                edge.from_index >= static_cast<int>(data.nodes.size()) ||
                edge.to_index >= static_cast<int>(data.nodes.size())) {
                continue;
            }

            YAML::Node edge_yaml;
            YAML::Node pair(YAML::NodeType::Sequence);
            pair.push_back(data.nodes[edge.from_index].label.toStdString());
            pair.push_back(data.nodes[edge.to_index].label.toStdString());
            edge_yaml["nodes"] = pair;
            if (!edge.type.trimmed().isEmpty()) {
                edge_yaml["type"] = edge.type.toStdString();
            }
            if (!edge.description.trimmed().isEmpty()) {
                edge_yaml["name"] = edge.description.toStdString();
            }
            if (!edge.structure.trimmed().isEmpty()) {
                edge_yaml["structure"] = edge.structure.trimmed().toStdString();
            }
            const QString effective_edge_color = edge.color.trimmed().isEmpty()
                ? data.settings.line_color
                : edge.color.trimmed();
            edge_yaml["color"] = effective_edge_color.toStdString();
            if (!edge.values.empty()) {
                YAML::Node values(YAML::NodeType::Sequence);
                for (const QString& value : edge.values) {
                    values.push_back(value.toStdString());
                }
                edge_yaml["values"] = values;
            }

            edge_yaml["swap_label_sides"] = edge.swap_label_sides;
            edge_yaml["label_segment"] = edge.label_segment_index;

            if (!edge.guide_nodes.empty()) {
                YAML::Node guide_nodes(YAML::NodeType::Sequence);
                for (const EdgeData::GuideNode& guide_node : edge.guide_nodes) {
                    YAML::Node guide;
                    guide["x"] = guide_node.x;
                    guide["y"] = guide_node.y;
                    guide_nodes.push_back(guide);
                }
                edge_yaml["guide_nodes"] = guide_nodes;
            }

            if (!edge.segment_kinds.empty()) {
                YAML::Node segment_types(YAML::NodeType::Sequence);
                for (EdgeData::SegmentKind segment_kind : edge.segment_kinds) {
                    segment_types.push_back(segment_kind_to_string(segment_kind).toStdString());
                }
                edge_yaml["segment_types"] = segment_types;
            }

            edges.push_back(edge_yaml);
        }

        root["nodes"] = nodes;
        root["edges"] = edges;

        YAML::Node settings;
        settings["node_radius"] = data.settings.node_radius;
        settings["line_thickness"] = data.settings.line_thickness;
        settings["node_outline_thickness"] = data.settings.node_outline_thickness;
        settings["font_size"] = data.settings.font_size;
        settings["label_angle_degrees"] = data.settings.label_angle_degrees;
        settings["node_label_distance"] = data.settings.node_label_distance;
        settings["font_family"] = data.settings.font_family.toStdString();
        settings["background_color"] = data.settings.background_color.toStdString();
        settings["node_fill_color"] = data.settings.node_fill_color.toStdString();
        settings["node_outline_color"] = data.settings.node_outline_color.toStdString();
        settings["line_color"] = data.settings.line_color.toStdString();
        settings["label_color"] = data.settings.label_color.toStdString();
        settings["value_decimals"] = data.settings.value_decimals;
        settings["value_unit"] = data.settings.value_unit.toStdString();
        root["settings"] = settings;

        YAML::Emitter emitter;
        emitter << root;
        if (!emitter.good()) {
            error = QString::fromUtf8(emitter.GetLastError().c_str());
            return false;
        }

        yaml_text = QString::fromUtf8(emitter.c_str());
        return true;
    } catch (const std::exception& ex) {
        error = QString::fromUtf8(ex.what());
        qWarning() << "Exception while serializing YAML" << error;
        return false;
    }
}

/**
 * @brief Serialize network data to YAML and write it to a file path.
 *
 * This function first converts in-memory network structures to YAML text and then writes
 * the result with truncation semantics. If file creation or serialization fails, it stores
 * the failure reason in `error`.
 *
 * @param path Destination file path for the YAML document.
 * @param data Network content to persist.
 * @param error Output string receiving serialization or I/O errors.
 * @return True when the file is written successfully; false otherwise.
 */
bool save_network_yaml(const QString& path, const NetworkData& data, QString& error) {
    qInfo() << "Saving network YAML to" << path;
    try {
        QString yaml_text;
        if (!network_yaml_to_string(data, yaml_text, error)) {
            return false;
        }

        std::ofstream out(path.toStdString(), std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            error = "Could not open file for writing.";
            return false;
        }

        out << yaml_text.toStdString();
        qInfo() << "Network saved";
        return true;
    } catch (const std::exception& ex) {
        error = QString::fromUtf8(ex.what());
        qWarning() << "Exception while saving YAML" << error;
        return false;
    }
}
