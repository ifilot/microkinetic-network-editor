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

#include <QString>

#include <vector>

struct NodeData {
    QString label;
    QString name;
    QString fill_color;
    QString outline_color;
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

bool load_network_yaml(const QString& path, NetworkData& out_data, QString& error);
bool save_network_yaml(const QString& path, const NetworkData& data, QString& error);
