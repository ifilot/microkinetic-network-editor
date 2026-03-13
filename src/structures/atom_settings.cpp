// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>

#include "atom_settings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

/**
 * @brief      Constructs a new instance.
 */
AtomSettings::AtomSettings() {
    // load default settings JSON from resources into memory
    QFile file(":/assets/configuration/atoms.json");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not read atoms.json file from assets");
    }

    const QByteArray raw = file.readAll();
    this->settings_data = raw.toStdString();
    this->reset();
}

/**
 * @brief reset.
 */
void AtomSettings::reset() {
    this->load();
    this->atom_color_rules.clear();
    this->atom_radius_rules.clear();
    this->bond_distance_rules.clear();

    // set all bonds by default to 3.0
    this->bond_distances.resize(121);
    for(unsigned int i=0; i<121; i++) {
        this->bond_distances[i].resize(121, 2.5);
    }

    // loop over all atoms
    for(unsigned int i=0; i<121; i++) {
        if(i > 20) {
            for(unsigned int j=2; j<=20; j++) {
                // bonds for hydrogen
                this->bond_distances[i][1] = 2.0;
                this->bond_distances[1][i] = 2.0;

                // other atoms
                this->bond_distances[i][j] = 2.2;
                this->bond_distances[j][i] = 2.2;
            }
        } else {
            for(unsigned int j=2; j<=20; j++) {
                // bonds for hydrogen
                this->bond_distances[i][1] = 1.2;
                this->bond_distances[1][i] = 1.2;

                // other atoms
                this->bond_distances[i][j] = 2.0;
                this->bond_distances[j][i] = 2.0;
            }
        }
    }

    // add some special cases on the basis of user input
    this->bond_distances[6][13] = 3.5; // Al-C
    this->bond_distances[13][6] = 3.5;

    this->radii.resize(119);
    this->colors.resize(119);
    for(unsigned int i=1; i<=118; i++) {
        this->radii[i] = this->get_atom_radius(this->get_name_from_elnr(i));
        this->colors[i] = this->hexcode_to_vector3d(this->get_atom_color(this->get_name_from_elnr(i)).substr(1,6));
    }
}

/**
 * @brief overwrite data using json
 * @param data
 */
void AtomSettings::overwrite(const std::string& data) {
    qDebug() << "Reconfiguring AtomSettings data";

    // update custom rule sets only from provided JSON object
    this->atom_color_rules.clear();
    this->atom_radius_rules.clear();
    this->bond_distance_rules.clear();

    if(data.empty()) {
        return;
    }

    try {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(data), &err);
        if(err.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "Error encountered in parsing JSON string: " << err.errorString();
            return;
        }

        const QJsonObject root = doc.object();

        if(root.contains("bond_distances") && root["bond_distances"].isArray()) {
            const QJsonArray arr = root["bond_distances"].toArray();
            for(const auto& value : arr) {
                std::string item = value.toString().toStdString();
                std::vector<std::string> pieces;
                boost::split(pieces, item, boost::is_any_of("/"));
                if(pieces.size() != 3) {
                    qWarning() << "Skipping malformed bond_distances rule:" << item.c_str();
                    continue;
                }

                std::string atom0 = pieces[0];
                std::string atom1 = pieces[1];
                float dist = boost::lexical_cast<float>(pieces[2]);

                unsigned int atom_id0 = this->get_atom_elnr(atom0);
                unsigned int atom_id1 = this->get_atom_elnr(atom1);

                this->bond_distances[atom_id0][atom_id1] = dist;
                this->bond_distances[atom_id1][atom_id0] = dist;

                BondDistanceRule rule;
                rule.element_a = atom0;
                rule.element_b = atom1;
                rule.max_distance = dist;
                this->bond_distance_rules.push_back(rule);

                qDebug() << "Overwring bond distances " << atom0.c_str() << "-"
                         << atom1.c_str() << ": " << pieces[2].c_str() << " angstrom.";
            }
        }

        if(root.contains("atom_colors") && root["atom_colors"].isArray()) {
            const QJsonArray arr = root["atom_colors"].toArray();
            for(const auto& value : arr) {
                std::string item = value.toString().toStdString();
                std::vector<std::string> pieces;
                boost::split(pieces, item, boost::is_any_of("/"));
                if(pieces.size() != 4) {
                    qWarning() << "Skipping malformed atom_colors rule:" << item.c_str();
                    continue;
                }

                IndexedColorRule rule;
                rule.element = pieces[0];
                rule.from = boost::lexical_cast<int>(pieces[1]);
                rule.to = boost::lexical_cast<int>(pieces[2]);
                rule.color = pieces[3];

                if(rule.from > rule.to) {
                    std::swap(rule.from, rule.to);
                }

                this->atom_color_rules.push_back(rule);
            }
        }

        if(root.contains("atom_radii") && root["atom_radii"].isArray()) {
            const QJsonArray arr = root["atom_radii"].toArray();
            for(const auto& value : arr) {
                std::string item = value.toString().toStdString();
                std::vector<std::string> pieces;
                boost::split(pieces, item, boost::is_any_of("/"));
                if(pieces.size() != 4) {
                    qWarning() << "Skipping malformed atom_radii rule:" << item.c_str();
                    continue;
                }

                IndexedRadiusRule rule;
                rule.element = pieces[0];
                rule.from = boost::lexical_cast<int>(pieces[1]);
                rule.to = boost::lexical_cast<int>(pieces[2]);
                rule.radius = boost::lexical_cast<float>(pieces[3]);

                if(rule.from > rule.to) {
                    std::swap(rule.from, rule.to);
                }

                this->atom_radius_rules.push_back(rule);
            }
        }
    }  catch (const std::exception& e) {
        qDebug() << "Error encountered in parsing JSON string: " << e.what();
    }
}

/**
 * @brief      Load the JSON file and parse its contents
 */
void AtomSettings::load() {
    try {
        std::stringstream ss(this->settings_data);
        boost::property_tree::read_json(ss, this->root);
    } catch(std::exception const& ex) {
        std::cerr << "[ERROR] There was an error parsing the JSON tree" << std::endl;
        std::cerr << ex.what() << std::endl;
        std::cerr << "[ERROR] Terminating program" << std::endl;
        exit(-1);
    }
}

/**
 * @brief      Get the atomic radius of an element
 *
 * @param[in]  elname  Element name
 *
 * @return     atomic radius
 */
float AtomSettings::get_atom_radius(const std::string& elname, unsigned int atom_index) const {
    std::string radius = this->root.get<std::string>("atoms.radii." + elname);
    float result = boost::lexical_cast<float>(radius);

    auto custom_rule = this->find_atom_radius_rule(elname, atom_index);
    if(custom_rule) {
        result = custom_rule->radius;
    }

    return result;
}

/**
 * @brief      Get the color of an element
 *
 * @param[in]  elname  Element name
 *
 * @return     atomic radius
 */
std::string AtomSettings::get_atom_color(const std::string& elname, unsigned int atom_index) const {
    std::string result = this->root.get<std::string>("atoms.colors." + elname);

    auto custom_rule = this->find_atom_color_rule(elname, atom_index);
    if(custom_rule) {
        result = custom_rule->color;
    }

    return result;
}

/**
 * @brief      Get the atomic radius of an element
 *
 * @param[in]  elname  Element name
 *
 * @return     atomic radius
 */
float AtomSettings::get_atom_radius_from_elnr(unsigned int elnr, unsigned int atom_index) const {
    return this->get_atom_radius(this->get_name_from_elnr(elnr), atom_index);
}

/**
 * @brief      Get element number of an element
 *
 * @param[in]  elname  Element name
 *
 * @return     The atom elnr.
 */
unsigned int AtomSettings::get_atom_elnr(const std::string& elname) const {
    std::string elnr = this->root.get<std::string>("atoms.elnr." + elname);
    return boost::lexical_cast<unsigned int>(elnr);
}

/**
 * @brief      Get the maximum bond distance between two atoms
 *
 * @param[in]  atoma  The atoma
 * @param[in]  atomb  The atomb
 *
 * @return     The bond distance.
 */
double AtomSettings::get_bond_distance(int atoma, int atomb) const {
    return this->bond_distances[atoma][atomb];
}

/**
 * @brief      Gets the name from element number.
 *
 * @param[in]  elnr  The elnr
 *
 * @return     The name from elnr.
 */
std::string AtomSettings::get_name_from_elnr(unsigned int elnr) const {
    return this->root.get<std::string>("atoms.nr2element." + boost::lexical_cast<std::string>(elnr));
}

/**
 * @brief get_atom_color_from_elnr.
 */
QVector3D AtomSettings::get_atom_color_from_elnr(unsigned int elnr, unsigned int atom_index) const {
    return this->hexcode_to_vector3d(this->get_atom_color(this->get_name_from_elnr(elnr), atom_index).substr(1,6));
}

std::optional<AtomSettings::IndexedColorRule> AtomSettings::find_atom_color_rule(const std::string& elname, unsigned int atom_index) const {
    std::optional<IndexedColorRule> match;
    for(const auto& rule : this->atom_color_rules) {
        if(rule.element != elname) {
            continue;
        }

        if(rule.from == 0 || rule.to == 0 ||
           (static_cast<int>(atom_index) >= rule.from && static_cast<int>(atom_index) <= rule.to)) {
            match = rule;
        }
    }

    return match;
}

std::optional<AtomSettings::IndexedRadiusRule> AtomSettings::find_atom_radius_rule(const std::string& elname, unsigned int atom_index) const {
    std::optional<IndexedRadiusRule> match;
    for(const auto& rule : this->atom_radius_rules) {
        if(rule.element != elname) {
            continue;
        }

        if(rule.from == 0 || rule.to == 0 ||
           (static_cast<int>(atom_index) >= rule.from && static_cast<int>(atom_index) <= rule.to)) {
            match = rule;
        }
    }

    return match;
}


/**
 * @brief hexcode_to_vector3d.
 */
QVector3D AtomSettings::hexcode_to_vector3d(const std::string& hexcode) const {
    if(hexcode.size() != 6) {
        throw std::runtime_error("Invalid hexcode received: " + hexcode);
    }

    float r = strtoul(hexcode.substr(0, 2).c_str(), NULL, 16) / 255.f;
    float g = strtoul(hexcode.substr(2, 2).c_str(), NULL, 16) / 255.f;
    float b = strtoul(hexcode.substr(4, 2).c_str(), NULL, 16) / 255.f;

    return QVector3D(r,g,b);
}
