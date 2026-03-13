// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>

#include "structure_loader.h"

#include "atom_settings.h"

#include <QDebug>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <yaml-cpp/yaml.h>

StructureLoader::StructureLoader() {}

std::vector<std::shared_ptr<Structure>> StructureLoader::load_file(const QString& path) {
    if (!path.endsWith(".yaml", Qt::CaseInsensitive) && !path.endsWith(".yml", Qt::CaseInsensitive)) {
        throw std::runtime_error("Unsupported structure file type (expected YAML): " + path.toStdString());
    }

    qDebug() << "Recognising file as PyMKMKit YAML type:" << path;
    if (!this->is_pymkmkit_yaml(path.toStdString())) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: missing 'pymkmkit' and/or 'structure' root elements.");
    }

    return this->load_pymkmkit_yaml(path.toStdString());
}

bool StructureLoader::is_pymkmkit_yaml(const std::string& filename) {
    try {
        YAML::Node yaml = YAML::LoadFile(filename);
        return yaml["pymkmkit"] && yaml["structure"];
    } catch (const YAML::Exception&) {
        return false;
    }
}

std::vector<std::shared_ptr<Structure>> StructureLoader::load_pymkmkit_yaml(const std::string& filename) {
    YAML::Node yaml;

    try {
        yaml = YAML::LoadFile(filename);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse YAML file '" + filename + "': " + e.what());
    }

    if (!yaml["pymkmkit"] || !yaml["structure"]) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: missing 'pymkmkit' and/or 'structure' root elements.");
    }

    const YAML::Node structure_node = yaml["structure"];
    const YAML::Node lattice_vectors = structure_node["lattice_vectors"];
    const YAML::Node coordinates_direct = structure_node["coordinates_direct"];

    if (!lattice_vectors || !lattice_vectors.IsSequence() || lattice_vectors.size() != 3) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: 'structure.lattice_vectors' must be a sequence of three vectors.");
    }

    if (!coordinates_direct || !coordinates_direct.IsSequence()) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: 'structure.coordinates_direct' must be a sequence.");
    }

    MatrixUnitcell unitcell = MatrixUnitcell::Zero(3, 3);
    for (std::size_t i = 0; i < 3; i++) {
        const YAML::Node row = lattice_vectors[i];
        if (!row.IsSequence() || row.size() != 3) {
            throw std::runtime_error("Invalid PyMKMKit YAML file: each lattice vector must contain exactly three values.");
        }

        unitcell(i, 0) = row[0].as<double>();
        unitcell(i, 1) = row[1].as<double>();
        unitcell(i, 2) = row[2].as<double>();
    }

    auto structure = std::make_shared<Structure>(unitcell);

    static const boost::regex regex_coord_direct("^\\s*([A-Za-z]+)\\s+([0-9eE.+-]+)\\s+([0-9eE.+-]+)\\s+([0-9eE.+-]+)\\s*$");

    for (std::size_t i = 0; i < coordinates_direct.size(); i++) {
        const std::string atom_line = coordinates_direct[i].as<std::string>();
        boost::smatch what;

        if (!boost::regex_match(atom_line, what, regex_coord_direct)) {
            throw std::runtime_error("Invalid PyMKMKit YAML coordinate line: " + atom_line);
        }

        const unsigned int elid = AtomSettings::get().get_atom_elnr(what[1]);
        const double fx = boost::lexical_cast<double>(what[2]);
        const double fy = boost::lexical_cast<double>(what[3]);
        const double fz = boost::lexical_cast<double>(what[4]);

        VectorPosition direct(fx, fy, fz);
        VectorPosition cartesian = unitcell.transpose() * direct;
        structure->add_atom(elid, cartesian(0), cartesian(1), cartesian(2));
    }

    return {structure};
}
