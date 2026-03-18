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

#include <QString>

#include <vector>

struct NodeData {
    QString label;
    QString name;
    QString structure;
    QString fill_color;
    QString outline_color;
    float label_angle_degrees{90.0f};
    float x{0.0f};
    float y{0.0f};
};

struct EdgeData {
    struct GuideNode {
        float x{0.0f};
        float y{0.0f};
    };

    enum class SegmentKind {
        Straight,
        BendClockwise,
        BendCounterClockwise,
        WiggleHorizontalFirst,
        WiggleVerticalFirst,
    };

    int from_index{-1};
    int to_index{-1};
    QString type;
    QString description;
    QString structure;
    QString color;
    std::vector<QString> values;
    bool swap_label_sides{false};
    int label_segment_index{0};
    std::vector<GuideNode> guide_nodes;
    std::vector<SegmentKind> segment_kinds;
};

struct ViewSettingsData {
    float node_radius{10.0f};
    float line_thickness{5.0f};
    float node_outline_thickness{3.0f};
    float font_size{12.0f};
    float label_angle_degrees{90.0f};
    float node_label_distance{14.0f};
    QString font_family{"Bahnschrift SemiBold"};
    QString background_color{"#fdf6e3"};
    QString node_fill_color{"#ffffff"};
    QString node_outline_color{"#000000"};
    QString line_color{"#dc322f"};
    QString label_color{"#000000"};
    int value_decimals{3};
    QString value_unit{"eV"};
};

struct NetworkData {
    std::vector<NodeData> nodes;
    std::vector<EdgeData> edges;
    ViewSettingsData settings;
    bool has_settings{false};
};

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
bool load_network_yaml(const QString& path, NetworkData& out_data, QString& error);
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
bool save_network_yaml(const QString& path, const NetworkData& data, QString& error);
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
bool network_yaml_to_string(const NetworkData& data, QString& yaml_text, QString& error);
