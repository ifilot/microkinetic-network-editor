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

#include "network_io.h"

#include <QDebug>

#include <yaml-cpp/yaml.h>

#include <QString>

#include <exception>
#include <fstream>
#include <unordered_map>

namespace {

constexpr float kGridSpacing = 140.0f;
constexpr int kGridColumns = 4;

void assign_default_positions(NetworkData& data) {
    for (size_t i = 0; i < data.nodes.size(); ++i) {
        const int col = static_cast<int>(i % kGridColumns);
        const int row = static_cast<int>(i / kGridColumns);
        data.nodes[i].x = 100.0f + static_cast<float>(col) * kGridSpacing;
        data.nodes[i].y = 100.0f + static_cast<float>(row) * kGridSpacing;
    }
}

QString color_or_default(const YAML::Node& node, const char* key, const QString& fallback) {
    if (node[key]) {
        return QString::fromStdString(node[key].as<std::string>());
    }
    return fallback;
}

YAML::Node map_child(const YAML::Node& node, const char* key) {
    if (!node || !node.IsMap()) {
        return YAML::Node();
    }
    return node[key];
}

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

void append_forward_backward_from_ads(const YAML::Node& ads_node, std::vector<QString>& out_values) {
    if (!ads_node || !ads_node.IsScalar()) {
        return;
    }

    const double ads = ads_node.as<double>();
    out_values.push_back(QString::number(0.0, 'g', 16));
    out_values.push_back(QString::number(-ads, 'g', 16));
}

}  // namespace

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
        bool has_all_positions = true;

        for (size_t i = 0; i < node_list.size(); ++i) {
            const YAML::Node yaml_node = node_list[i];
            const YAML::Node label_node = map_child(yaml_node, "label");
            if (!label_node || !label_node.IsScalar()) {
                error = "A node is missing the required 'label' field.";
                return false;
            }

            NodeData node;
            node.label = scalar_to_qstring(label_node);
            const QString maybe_name = scalar_to_qstring(map_child(yaml_node, "name"));
            node.name = maybe_name.isEmpty() ? node.label : maybe_name;

            if (map_child(yaml_node, "position") && map_child(yaml_node, "position").IsMap() &&
                map_child(map_child(yaml_node, "position"), "x") && map_child(map_child(yaml_node, "position"), "y")) {
                node.x = map_child(map_child(yaml_node, "position"), "x").as<float>();
                node.y = map_child(map_child(yaml_node, "position"), "y").as<float>();
            } else {
                has_all_positions = false;
            }

            index_by_label[node.label.toStdString()] = static_cast<int>(data.nodes.size());
            data.nodes.push_back(node);
        }

        for (size_t i = 0; i < edge_list.size(); ++i) {
            const YAML::Node edge = edge_list[i];
            const YAML::Node edge_nodes = map_child(edge, "nodes");
            if (!edge_nodes || !edge_nodes.IsSequence() || edge_nodes.size() != 2) {
                error = "An edge has an invalid 'nodes' entry (expected two labels).";
                return false;
            }

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

            const QString edge_type = scalar_to_qstring(map_child(edge, "type")).trimmed().toLower();
            if (edge_type == "ads") {
                append_forward_backward_from_ads(map_child(edge, "ads"), edge_data.values);
            } else if (edge_type == "surf") {
                append_value_node(map_child(edge, "forward"), edge_data.values);
                append_value_node(map_child(edge, "backward"), edge_data.values);
            } else {
                append_value_node(map_child(edge, "values"), edge_data.values);
                if (edge_data.values.empty()) {
                    append_value_node(map_child(edge, "forward"), edge_data.values);
                    append_value_node(map_child(edge, "backward"), edge_data.values);
                    if (edge_data.values.empty()) {
                        append_value_node(map_child(edge, "ads"), edge_data.values);
                    }
                }
            }

            data.edges.push_back(edge_data);
        }

        if (!has_all_positions) {
            assign_default_positions(data);
        }

        YAML::Node settings = root["settings"];
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

bool save_network_yaml(const QString& path, const NetworkData& data, QString& error) {
    qInfo() << "Saving network YAML to" << path;
    try {
        YAML::Node root;
        YAML::Node nodes(YAML::NodeType::Sequence);
        for (const NodeData& node : data.nodes) {
            YAML::Node node_yaml;
            node_yaml["label"] = node.label.toStdString();
            if (!node.name.isEmpty() && node.name != node.label) {
                node_yaml["name"] = node.name.toStdString();
            }
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
            if (!edge.values.empty()) {
                YAML::Node values(YAML::NodeType::Sequence);
                for (const QString& value : edge.values) {
                    values.push_back(value.toStdString());
                }
                edge_yaml["values"] = values;
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

        std::ofstream out(path.toStdString(), std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            error = "Could not open file for writing.";
            return false;
        }

        out << emitter.c_str();
        qInfo() << "Network saved";
        return true;
    } catch (const std::exception& ex) {
        error = QString::fromUtf8(ex.what());
        qWarning() << "Exception while saving YAML" << error;
        return false;
    }
}
