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

#include <QFile>
#include <QDebug>

// boost headers
#include <boost/filesystem.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

// stl headers
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

#include <QVector3D>

/**
 * @brief      Class holding information about atoms in the periodic table
 */
class AtomSettings {

private:
    struct IndexedColorRule {
        std::string element;
        int from = 0;
        int to = 0;
        std::string color;
    };

    struct IndexedRadiusRule {
        std::string element;
        int from = 0;
        int to = 0;
        float radius = 1.0f;
    };

    struct BondDistanceRule {
        std::string element_a;
        std::string element_b;
        float max_distance = 2.0f;
    };

private:
    std::string settings_data;
    boost::property_tree::ptree root;

    std::vector<std::vector<double>> bond_distances;
    std::vector<float> radii;
    std::vector<QVector3D> colors;
    std::vector<IndexedColorRule> atom_color_rules;
    std::vector<IndexedRadiusRule> atom_radius_rules;
    std::vector<BondDistanceRule> bond_distance_rules;

public:
    /**
     * @brief      Get AtomSettings Class
     *
     * Default singleton pattern
     *
     * @return     return instance of the atomsettings class
     */
    static AtomSettings& get() {
        static AtomSettings settings_instance;
        return settings_instance;
    }

    /**
     * @brief Rebuild AtomSettings data
     */
    void reset();

    /**
     * @brief overwrite data using json
     * @param data
     */
    void overwrite(const std::string& data);

    /**
     * @brief      Get the atomic radius of an element
     *
     * @param[in]  elname  Element name
     *
     * @return     atomic radius
     */
    float get_atom_radius(const std::string& elname, unsigned int atom_index = 0) const;

    /**
     * @brief      Get the color of an element
     *
     * @param[in]  elname  Element name
     *
     * @return     atomic radius
     */
    std::string get_atom_color(const std::string& elname, unsigned int atom_index = 0) const;

    /**
     * @brief      Get the atomic radius of an element
     *
     * @param[in]  elname  Element name
     *
     * @return     atomic radius
     */
    float get_atom_radius_from_elnr(unsigned int elnr, unsigned int atom_index = 0) const;

    /**
     * @brief      Get element number of an element
     *
     * @param[in]  elname  Element name
     *
     * @return     The atom elnr.
     */
    unsigned int get_atom_elnr(const std::string& elname) const;

    /**
     * @brief      Get the maximum bond distance between two atoms
     *
     * @param[in]  atoma  The atoma
     * @param[in]  atomb  The atomb
     *
     * @return     The bond distance.
     */
    double get_bond_distance(int atoma, int atomb) const;

    /**
     * @brief      Gets the name from element number.
     *
     * @param[in]  elnr  The elnr
     *
     * @return     The name from elnr.
     */
    std::string get_name_from_elnr(unsigned int elnr) const;

    /**
     * @brief      Gets the color from element number.
     *
     * @param[in]  elnr  The elnr
     *
     * @return     The name from elnr.
     */
    QVector3D get_atom_color_from_elnr(unsigned int elnr, unsigned int atom_index = 0) const;

private:
    /**
     * @brief      Constructs a new instance.
     */
    AtomSettings();

    /**
     * @brief      Load the JSON file and parse its contents
     */
    void load();

    /**
     * @brief hexcode_to_vector3d.
     */
    QVector3D hexcode_to_vector3d(const std::string& hexcode) const;

    std::optional<IndexedColorRule> find_atom_color_rule(const std::string& elname, unsigned int atom_index) const;
    std::optional<IndexedRadiusRule> find_atom_radius_rule(const std::string& elname, unsigned int atom_index) const;

    // delete copy constructor
    AtomSettings(AtomSettings const&)          = delete;
    void operator=(AtomSettings const&)    = delete;
};
