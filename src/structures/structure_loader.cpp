// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>

#include "structure_loader.h"
#include <QFileInfo>
#include <yaml-cpp/yaml.h>

/**
 * @brief      Constructs a new instance.
 */
StructureLoader::StructureLoader() {

}

/**
 * @brief load_file.
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_file(const QString& path) {
    QFileInfo file_info(path);
    QString filename = file_info.fileName();

    if(filename.contains("CONTCAR") || filename.contains("POSCAR")) {
        qDebug() << "Recognising file as POSCAR/CONTCAR type: " << path;
        return this->load_poscar(path.toStdString());
    } else if(filename == "logfile") {
        qDebug() << "Recognising file as ADF logfile type: " << path;
        return this->load_adf_logfile(path.toStdString());
    } else if(filename.endsWith(".log") || filename.endsWith(".LOG")) {
        qDebug() << "Recognising file as Gaussian log file type: " << path;
        return this->load_gaussian_logfile(path.toStdString());
    } else if (filename.endsWith(".mks")) {
        qDebug() << "Recognising file as MicroKinetic State (.mks) type:" << path;
        return this->load_mks(path.toStdString());
    } else if (filename.endsWith(".yaml") || filename.endsWith(".yml")) {
        qDebug() << "Recognising file as PyMKMKit YAML type:" << path;
        if(!this->is_pymkmkit_yaml(path.toStdString())) {
            throw std::runtime_error("Invalid PyMKMKit YAML file: missing 'pymkmkit' and/or 'structure' root elements.");
        }
        return this->load_pymkmkit_yaml(path.toStdString());
    } else {
        throw std::runtime_error("Unknown file type: " + filename.toStdString());
    }
}

/**
 * @brief      Load structure from OUTCAR file
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_outcar(const std::string& filename) {
    std::ifstream infile(filename);

    if(!infile.is_open()) {
        throw std::runtime_error("Could not open " + filename);
    }

    // vasp version
    unsigned int vasp_version = 0;

    // current reading state
    unsigned int readstate = 0;
    readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ELEMENTS);
    readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_IONS_PER_ELEMENT);
    readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_OPEN);

    // number of atoms and number of states
    unsigned int nr_atoms = 0;
    unsigned int nr_states = 0;

    MatrixUnitcell unitcell = MatrixUnitcell::Zero(3,3);
    std::vector<double> energies;
    std::vector<std::string> elements;
    std::vector<unsigned int> nr_atoms_per_elm;

    /*
    * Define all the regex patterns
    */
    boost::regex regex_vasp_version("^\\s*vasp.([0-9]).([0-9]+).([0-9]+).*$");
    boost::regex regex_element("^\\s*(VRHFIN\\s+=)([A-Za-z]+)\\s*:.*$");
    boost::regex regex_ions_per_element("^\\s*(ions per type =\\s+)([0-9 ]+)\\s*$");
    boost::regex regex_lattice_vectors("^\\s*direct lattice vectors.*$");
    boost::regex regex_atoms("^\\s*POSITION.*$");
    boost::regex regex_grab_numbers("^\\s+([0-9.-]+)\\s+([0-9.-]+)\\s+([0-9.-]+)\\s+([0-9.-]+)\\s+([0-9.-]+)\\s+([0-9.-]+).*$");
    boost::regex regex_grab_energy("^\\s+energy  without entropy=\\s+([0-9.-]+)\\s+energy\\(sigma->0\\) =\\s+([0-9.-]+).*$");

    std::string line;

    std::vector<std::shared_ptr<Structure>> structures;

    while(std::getline(infile, line)) { // loop over all the lines in the file
        boost::smatch what1;

        /*
         * Collect the vasp version (4 or 5)
         */
        if(readstate & (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ELEMENTS) ) {
          // get the elements and put these in an array
            if (boost::regex_match(line, what1, regex_vasp_version)) {

                vasp_version = boost::lexical_cast<unsigned int>(what1[1]);
                unsigned int version_major = boost::lexical_cast<unsigned int>(what1[2]);
                unsigned int version_minor = boost::lexical_cast<unsigned int>(what1[3]);

                continue;
            }
        }

        /*
         * Collect the elements
         */
        if(readstate & (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ELEMENTS) ) {
            if (boost::regex_match(line, what1, regex_element)) {
                elements.push_back(what1[2]);
                continue;
            }
        }

        /*
         * Collect the number of ions of each element type
         */
        if(readstate & (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_IONS_PER_ELEMENT) ) {
            if (boost::regex_match(line, what1, regex_ions_per_element)) {
                std::vector<std::string> pieces;
                const std::string subline = what1[2];
                boost::split(pieces, subline, boost::is_any_of("\t "), boost::token_compress_on);
                for(unsigned int i=0; i<pieces.size(); i++) {
                    nr_atoms_per_elm.push_back(boost::lexical_cast<unsigned int>(pieces[i]));
                    nr_atoms += boost::lexical_cast<unsigned int>(pieces[i]);

                }

                // remove ions state and elements state
                readstate &= ~(1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ELEMENTS);
                readstate &= ~(1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_IONS_PER_ELEMENT);
                readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_LATTICE_VECTORS);

                // check if a vasp version has been identified, if not, terminate
                if(!(vasp_version == 4 || vasp_version == 5)) {
                    throw std::runtime_error("Invalid VASP version encountered: " + boost::lexical_cast<std::string>(vasp_version));
                }

                continue;
            }
        }

        /*
         * Collect the dimensions of the unit cell.
         */
        if(readstate & (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_LATTICE_VECTORS) ) {
            // get the dimensionality of the unit cell
            if (boost::regex_match(line, what1, regex_lattice_vectors)) {
                boost::smatch what2;

                // grab next tree lines
                for(unsigned int i=0; i<3; i++) {
                    std::getline(infile, line);
                    if (boost::regex_match(line, what2, regex_grab_numbers)) {
                        unitcell(i,0) = boost::lexical_cast<double>(what2[1]);
                        unitcell(i,1) = boost::lexical_cast<double>(what2[2]);
                        unitcell(i,2) = boost::lexical_cast<double>(what2[3]);
                    }
                }

                readstate &= ~(1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_LATTICE_VECTORS);
                readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ATOMS);

                continue;
            }
        }

        /*
         * Collect the energy of the state
         */
        if(readstate & (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ATOMS) ) {
            if (boost::regex_match(line, what1, regex_grab_energy)) {

                energies.push_back(boost::lexical_cast<double>(what1[2]));

                if(vasp_version == 5) {
                    nr_states++;

                    readstate &= ~(1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ATOMS);
                    readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_LATTICE_VECTORS);
                }

                continue;
            }
        }

        /*
         * Collect the atomic positions and forces for this state
         */
        if(readstate & (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ATOMS) ) {
            if (boost::regex_match(line, what1, regex_atoms)) {
                std::getline(infile, line); // skip dashed line

                // create a new structure
                structures.push_back(std::make_shared<Structure>(unitcell));

                for(unsigned i=0; i<nr_atoms_per_elm.size(); i++) {
                    for(unsigned int j=0; j<nr_atoms_per_elm[i]; j++) {
                        std::getline(infile, line);
                        boost::smatch what2;
                        if (boost::regex_match(line, what2, regex_grab_numbers)) {
                            double x = boost::lexical_cast<double>(what2[1]);
                            double y = boost::lexical_cast<double>(what2[2]);
                            double z = boost::lexical_cast<double>(what2[3]);
                            double fx = boost::lexical_cast<double>(what2[4]);
                            double fy = boost::lexical_cast<double>(what2[5]);
                            double fz = boost::lexical_cast<double>(what2[6]);
                            unsigned int atnr = AtomSettings::get().get_atom_elnr(elements[i]);
                            structures.back()->add_atom(atnr, x, y, z, fx, fy, fz);
                        }
                    }
                }

                if(vasp_version == 4) {
                    nr_states++;

                    readstate &= ~(1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_ATOMS);
                    readstate |= (1 << OutcarReadStatus::VASP_OUTCAR_READ_STATE_LATTICE_VECTORS);
                }

                continue;
            }
        }
    }

    // energies are sometimes given either before or after the coordinates, hence, only
    // set the energies after everything has been parsed
    if(energies.size() != structures.size()) {
        throw std::runtime_error("Number of energies does not match number of structures.");
    }

    for(unsigned int i=0; i<energies.size(); i++) {
        structures[i]->set_energy(energies[i]);
    }

    return structures;
}

/**
 * @brief      Loads a structure from a binary structure pack file
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_structurepack(const std::string& filename) {
    std::ifstream infile(filename);

    // read number of atoms
    uint32_t datatype;
    uint32_t nr_atoms;
    uint32_t nr_images;
    infile.read((char*)&datatype, sizeof(uint32_t));
    infile.read((char*)&nr_images, sizeof(uint32_t));
    infile.read((char*)&nr_atoms, sizeof(uint32_t));

    // print some info
    std::cout << "Parsing " << nr_images << " images." << std::endl;
    std::cout << "Each structure contains " << nr_atoms << " atoms." << std::endl;

    if(nr_images > 10000) {
        throw std::runtime_error("Excessively large number of images encountered. Assuming incorrect input file.");
    }

    // build structure
    std::vector<std::shared_ptr<Structure>> structures;

    for(unsigned int k=0; k<nr_images; k++) {
        // read unit cell
        MatrixUnitcell unitcell = MatrixUnitcell::Zero(3,3);
        for(unsigned int i=0; i<3; i++) {
            for(unsigned int j=0; j<3; j++) {
                infile.read((char*)&unitcell(i,j), sizeof(double));
            }
        }

        // read energy
        double energy = 0;
        infile.read((char*)&energy, sizeof(double));

        // set structure
        structures.push_back(std::make_shared<Structure>(unitcell));
        structures.back()->set_energy(energy);

        // add atoms to structure
        for(uint32_t i=0; i<nr_atoms; i++) {
            uint8_t elid = 0;
            double x = 0;
            double y = 0;
            double z = 0;
            double fx = 0;
            double fy = 0;
            double fz = 0;

            infile.read((char*)&elid, sizeof(uint8_t));
            infile.read((char*)&x, sizeof(double));
            infile.read((char*)&y, sizeof(double));
            infile.read((char*)&z, sizeof(double));
            infile.read((char*)&fx, sizeof(double));
            infile.read((char*)&fy, sizeof(double));
            infile.read((char*)&fz, sizeof(double));

            structures.back()->add_atom(elid, x, y, z);
        }
    }

    infile.close();

    return structures;
}

/**
 * @brief      Load structure from POSCAR file
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_poscar(const std::string& filename) {
    std::ifstream infile(filename);

    if(!infile.is_open()) {
        throw std::runtime_error("Could not open " + filename);
    }

    std::string line;

    // skip first line (name of system)
    std::getline(infile, line);

    // read scaling factor
    std::getline(infile, line);
    boost::trim(line);
    double scalar = boost::lexical_cast<double>(line);

    // read matrix
    std::vector<std::string> pieces;
    MatrixUnitcell unitcell = MatrixUnitcell::Zero(3,3);
    for(unsigned int j=0; j<3; j++) {
        std::getline(infile, line);
        boost::trim(line);
        boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);
        for(unsigned int i=0; i<3; i++) {
            unitcell(j,i) = boost::lexical_cast<double>(pieces[i]);
        }
    }
    unitcell *= scalar;
    auto structure = std::make_shared<Structure>(unitcell);

    // assume that POSCARS are POSCAR5
    std::getline(infile, line);
    const boost::regex regex_els("^.*[A-Za-z]+.*$"); // if the line contains even a single non-numeric character, it is a line containing elements
    boost::smatch what;
    if(!boost::regex_match(line, what, regex_els)) {
        throw std::runtime_error("This file is probably a VASP4 POSCAR file. You can only load VASP5+ POSCAR files");
    }
    std::vector<std::string> elements;
    boost::trim(line);
    boost::split(elements, line, boost::is_any_of(" \t"), boost::token_compress_on);

    // get the number for each element
    std::getline(infile, line);
    boost::trim(line);
    std::vector<unsigned int> nr_elements;
    boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);
    for(unsigned int i=0; i<pieces.size(); i++) {
        nr_elements.push_back(boost::lexical_cast<unsigned int>(pieces[i]));
    }
    if(nr_elements.size() != elements.size()) {
        throw std::runtime_error("Array size for element types does not match array size for number for each element type.");
    }

    // check if next line is selective dynamics, if so, skip
    bool selective_dynamics = false;
    std::getline(infile, line);
    const boost::regex regex_sd("^\\s*[Ss].*$");
    if(boost::regex_match(line, what, regex_sd)) {
        selective_dynamics = true;
        std::getline(infile, line);
    }

    // direct or cartesian
    bool direct = (line[0] == 'D' || line[0] == 'd') ? true : false;

    // collect atoms
    static const boost::regex regex_double3("^\\s*([0-9e.-]+)\\s+([0-9e.-]+)\\s+([0-9e.-]+)\\s*(.*)$");
    static const boost::regex regex_double3_bool3("^\\s*([0-9e.-]+)\\s+([0-9e.-]+)\\s+([0-9e.-]+)\\s+([TF])\\s+([TF])\\s+([TF])\\s*(.*)$");

    for(unsigned int i=0; i<elements.size(); i++) {
        unsigned int elid = AtomSettings::get().get_atom_elnr(elements[i]);
        for(unsigned int j=0; j<nr_elements[i]; j++) {
            std::getline(infile, line);
            boost::smatch what;

            if(selective_dynamics) {
                if(boost::regex_match(line, what, regex_double3_bool3)) {
                    double x = boost::lexical_cast<double>(what[1]);
                    double y = boost::lexical_cast<double>(what[2]);
                    double z = boost::lexical_cast<double>(what[3]);
                    bool sx = (what[4] == "F" ? false : true);
                    bool sy = (what[5] == "F" ? false : true);
                    bool sz = (what[6] == "F" ? false : true);

                    VectorPosition position(x,y,z);

                    // build coordinates
                    if(direct) {
                        VectorPosition cartesian = unitcell.transpose() * position;
                        structure->add_atom(elid, cartesian(0), cartesian(1), cartesian(2), sx, sy, sz);
                    } else {
                        structure->add_atom(elid, position(0), position(1), position(2), sx, sy, sz);
                    }
                }
            } else {
                if(boost::regex_match(line, what, regex_double3)) {
                    double x = boost::lexical_cast<double>(what[1]);
                    double y = boost::lexical_cast<double>(what[2]);
                    double z = boost::lexical_cast<double>(what[3]);
                    VectorPosition position(x,y,z);

                    // build coordinates
                    if(direct) {
                        VectorPosition cartesian = unitcell.transpose() * position;
                        structure->add_atom(elid, cartesian(0), cartesian(1), cartesian(2));
                    } else {
                        structure->add_atom(elid, position(0), position(1), position(2));
                    }
                }
            }
        }
    }

    // package inside vector
    std::vector<std::shared_ptr<Structure>> structures;
    structures.push_back(structure);

    return structures;
}

/**
 * @brief      Load structure from ADF logfile
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_adf_logfile(const std::string& filename) {
    std::ifstream infile(filename);

    if(!infile.is_open()) {
        throw std::runtime_error("Could not open " + filename);
    }

    // create container
    std::vector<std::shared_ptr<Structure>> structures;

    std::string line;

    while(std::getline(infile, line)) {
        if(line.substr(0, 30) == " Coordinates in Geometry Cycle") {
            std::getline(infile, line); // skip line

            auto structure = std::make_shared<Structure>(MatrixUnitcell(3,3), true);

            static const boost::regex regex_atoms("^\\s*[0-9]+\\.([A-Za-z]+)\\s+([0-9e.-]+)\\s+([0-9e.-]+)\\s+([0-9e.-]+)\\s*$");
            double xmin = 1000, xmax = 0;
            double ymin = 1000, ymax = 0;
            double zmin = 1000, zmax = 0;
            while(true) {
                std::getline(infile, line);
                boost::smatch what;
                if(boost::regex_match(line, what, regex_atoms)) {
                    double x = boost::lexical_cast<double>(what[2]);
                    double y = boost::lexical_cast<double>(what[3]);
                    double z = boost::lexical_cast<double>(what[4]);

                    xmin = std::min(xmin, x);
                    ymin = std::min(ymin, y);
                    zmin = std::min(zmin, z);

                    xmax = std::max(xmax, x);
                    ymax = std::max(ymax, y);
                    zmax = std::max(zmax, z);

                    unsigned int elid = AtomSettings::get().get_atom_elnr(what[1]);

                    structure->add_atom(elid, x, y, z);
                } else {
                    break;
                }
            }

            // build unitcell
            double dx = (xmax - xmin) + 3;
            double dy = (ymax - ymin) + 3;
            double dz = (zmax - zmin) + 3;
            MatrixUnitcell unitcell(3,3);
            unitcell(0,0) = dx;
            unitcell(1,1) = dy;
            unitcell(2,2) = dz;
            structure->set_unitcell(unitcell);
            structures.push_back(structure);
        }
    }

    return structures;
}

/**
 * @brief      Load structure from Gaussian logfile
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_gaussian_logfile(const std::string& filename) {

    // open file
    std::ifstream infile(filename);
    if(!infile.is_open()) {
        throw std::runtime_error("Could not open " + filename);
    }

    // create container
    std::vector<std::shared_ptr<Structure>> structures;

    std::string line;

    // build regular expressionw
    boost::smatch what;
    const boost::regex re_orientation("^\\s+(?:Standard|Input) orientation:\\s+$");

    while(std::getline(infile, line)) {
        if(boost::regex_match(line, what, re_orientation)) {
            // skip four lines
            for(unsigned int i=0; i<4; i++) {
                std::getline(infile, line);
            }

            // create structure container
            auto structure = std::make_shared<Structure>(MatrixUnitcell(3,3), true);

            // keep track of molecular dimensions
            double xmin = 1000, xmax = 0;
            double ymin = 1000, ymax = 0;
            double zmin = 1000, zmax = 0;

            // start reading lines until "---" is encountered
            while(std::getline(infile, line)) {
                if(line.find(" ----------") == 0) {
                    break;
                }

                std::vector<std::string> pieces;
                boost::trim(line);
                boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);

                const unsigned int atomid = boost::lexical_cast<unsigned int>(pieces[0]);
                const unsigned int elementid = boost::lexical_cast<unsigned int>(pieces[1]);
                const double x = boost::lexical_cast<double>(pieces[3]);
                const double y = boost::lexical_cast<double>(pieces[4]);
                const double z = boost::lexical_cast<double>(pieces[5]);

                xmin = std::min(xmin, x);
                ymin = std::min(ymin, y);
                zmin = std::min(zmin, z);

                xmax = std::max(xmax, x);
                ymax = std::max(ymax, y);
                zmax = std::max(zmax, z);

                structure->add_atom(elementid, x, y, z);
            }

            // adjust unitcell such that molecule has at least 10A of vacuum space in each
            // cartesian direction
            double dx = (xmax - xmin) + 3;
            double dy = (ymax - ymin) + 3;
            double dz = (zmax - zmin) + 3;
            MatrixUnitcell unitcell(3,3);
            unitcell(0,0) = dx;
            unitcell(1,1) = dy;
            unitcell(2,2) = dz;
            structure->set_unitcell(unitcell);
            structures.push_back(structure);
        }
    }

    return structures;
}

/**
 * @brief      Load structure from ANNP DATA file
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_data(const std::string& filename) {
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        throw std::runtime_error("Could not open " + filename);
    }

    std::string line;
    std::vector<std::string> pieces;  // temporary container for boost::split

    // skip first line (comment)
    std::getline(infile, line);

    // skip until we get to lattice matrix
    const boost::regex regex_lattice("Lattice:");
    boost::smatch what;
    while (!boost::regex_match(line, what, regex_lattice)) {
        std::getline(infile, line);
    }

    // read lattice matrix
    MatrixUnitcell unitcell = MatrixUnitcell::Zero(3, 3);
    for (unsigned int j = 0; j < 3; j++) {
        std::getline(infile, line);
        boost::trim(line);
        boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);
        for (unsigned int i = 0; i < 3; i++) {
            unitcell(j, i) = boost::lexical_cast<double>(pieces[i]);
        }
    }
    auto structure = std::make_shared<Structure>(unitcell);

    // skip until we get to atom list
    const boost::regex regex_atom_list("Atomlist:");
    while (!boost::regex_match(line, what, regex_atom_list)) {
        std::getline(infile, line);
    }

    // read atom list
    std::getline(infile, line);
    boost::trim(line);
    boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);

    std::vector<unsigned int> atom_indices;
    unsigned int highest_atom_index = 0;
    unsigned int this_atom_index;
    for (unsigned int i = 0; i < pieces.size(); i++) {
        this_atom_index = boost::lexical_cast<unsigned int>(pieces[i]);
        atom_indices.push_back(this_atom_index);
        if (this_atom_index > highest_atom_index) {
            highest_atom_index = this_atom_index;
        }
    }

    // skip until we get to elements
    const boost::regex regex_elements("Chemical Symbols:");
    while (!boost::regex_match(line, what, regex_elements)) {
        std::getline(infile, line);
    }

    // read elements
    std::getline(infile, line);
    boost::trim(line);
    boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);
    std::vector<unsigned int> elements;
    for (unsigned int i = 0; i < pieces.size(); i++) {
        elements.push_back(AtomSettings::get().get_atom_elnr(pieces[i]));
    }

    // verify atom list indices using number of elements
    if (highest_atom_index >= elements.size()) {
        throw std::runtime_error("Invalid element indices encountered in atom list.");
    }

    // skip until we get to coordinates
    // coordinates are always written in Cartesian
    const boost::regex regex_coordinates("Coordinates: Cartesian");
    while (!boost::regex_match(line, what, regex_coordinates)) {
        std::getline(infile, line);
    }

    // read coordinates
    static const boost::regex regex_double3("^\\s*([0-9e.-]+)\\s+([0-9e.-]+)\\s+([0-9e.-]+)\\s*(.*)$");

    for (unsigned int i = 0; i < atom_indices.size(); i++) {
        unsigned int elid = elements[atom_indices[i]];

        std::getline(infile, line);
        boost::smatch what;
        if (boost::regex_match(line, what, regex_double3)) {
            double x = boost::lexical_cast<double>(what[1]);
            double y = boost::lexical_cast<double>(what[2]);
            double z = boost::lexical_cast<double>(what[3]);

            structure->add_atom(elid, x, y, z);
        }
    }

    infile.close();

    // package inside vector
    std::vector<std::shared_ptr<Structure>> structures;
    structures.push_back(structure);

    return structures;
}


/**
 * @brief      Validate whether a YAML file follows the PyMKMKit structure schema
 *
 * @param[in]  filename  The filename
 *
 * @return     True if file contains both 'pymkmkit' and 'structure' root elements
 */
bool StructureLoader::is_pymkmkit_yaml(const std::string& filename) {
    try {
        YAML::Node yaml = YAML::LoadFile(filename);
        return yaml["pymkmkit"] && yaml["structure"];
    } catch(const YAML::Exception&) {
        return false;
    }
}

/**
 * @brief      Load structure from PyMKMKit YAML file
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_pymkmkit_yaml(const std::string& filename) {
    YAML::Node yaml;

    try {
        yaml = YAML::LoadFile(filename);
    } catch(const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse YAML file '" + filename + "': " + e.what());
    }

    if(!yaml["pymkmkit"] || !yaml["structure"]) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: missing 'pymkmkit' and/or 'structure' root elements.");
    }

    const YAML::Node structure_node = yaml["structure"];
    const YAML::Node lattice_vectors = structure_node["lattice_vectors"];
    const YAML::Node coordinates_direct = structure_node["coordinates_direct"];

    if(!lattice_vectors || !lattice_vectors.IsSequence() || lattice_vectors.size() != 3) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: 'structure.lattice_vectors' must be a sequence of three vectors.");
    }

    if(!coordinates_direct || !coordinates_direct.IsSequence()) {
        throw std::runtime_error("Invalid PyMKMKit YAML file: 'structure.coordinates_direct' must be a sequence.");
    }

    MatrixUnitcell unitcell = MatrixUnitcell::Zero(3, 3);
    for(std::size_t i = 0; i < 3; i++) {
        const YAML::Node row = lattice_vectors[i];
        if(!row.IsSequence() || row.size() != 3) {
            throw std::runtime_error("Invalid PyMKMKit YAML file: each lattice vector must contain exactly three values.");
        }

        unitcell(i, 0) = row[0].as<double>();
        unitcell(i, 1) = row[1].as<double>();
        unitcell(i, 2) = row[2].as<double>();
    }

    auto structure = std::make_shared<Structure>(unitcell);

    static const boost::regex regex_coord_direct("^\\s*([A-Za-z]+)\\s+([0-9eE.+-]+)\\s+([0-9eE.+-]+)\\s+([0-9eE.+-]+)\\s*$");

    for(std::size_t i = 0; i < coordinates_direct.size(); i++) {
        const std::string atom_line = coordinates_direct[i].as<std::string>();
        boost::smatch what;

        if(!boost::regex_match(atom_line, what, regex_coord_direct)) {
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

    return { structure };
}

/**
 * @brief      Load structure from MKS file
 *
 * @param[in]  filename  The filename
 *
 * @return     Structure
 */
std::vector<std::shared_ptr<Structure>> StructureLoader::load_mks(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open " + filename);
    }

    std::string line;

    unsigned int nr_atoms = 0;
    double energy = 0.0;

    MatrixUnitcell unitcell = MatrixUnitcell::Zero(3, 3);

    // --- Read number of atoms ---
    while (std::getline(infile, line)) {
        boost::trim(line);
        if (line == "# Number of atoms") {
            std::getline(infile, line);
            nr_atoms = boost::lexical_cast<unsigned int>(line);
            break;
        }
    }

    // --- Read electronic energy ---
    while (std::getline(infile, line)) {
        boost::trim(line);
        if (line == "# Electronic energy (eV)") {
            std::getline(infile, line);
            energy = boost::lexical_cast<double>(line);
            break;
        }
    }

    // --- Read cell vectors ---
    while (std::getline(infile, line)) {
        boost::trim(line);
        if (line == "# Cell vectors (Å)") {
            for (unsigned int i = 0; i < 3; i++) {
                std::getline(infile, line);
                boost::trim(line);

                std::vector<std::string> pieces;
                boost::split(pieces, line, boost::is_any_of(" \t"), boost::token_compress_on);

                for (unsigned int j = 0; j < 3; j++) {
                    unitcell(i, j) = boost::lexical_cast<double>(pieces[j]);
                }
            }
            break;
        }
    }

    auto structure = std::make_shared<Structure>(unitcell);
    structure->set_energy(energy);

    // --- Read atomic coordinates ---
    while (std::getline(infile, line)) {
        boost::trim(line);
        if (line == "# Atomic coordinates (Å)") {
            break;
        }
    }

    static const boost::regex regex_atom(
        "^\\s*([A-Za-z]+)\\s+([0-9eE.+-]+)\\s+([0-9eE.+-]+)\\s+([0-9eE.+-]+)\\s*$"
    );

    for (unsigned int i = 0; i < nr_atoms; i++) {
        std::getline(infile, line);
        boost::smatch what;

        if (boost::regex_match(line, what, regex_atom)) {
            unsigned int elid = AtomSettings::get().get_atom_elnr(what[1]);
            double x = boost::lexical_cast<double>(what[2]);
            double y = boost::lexical_cast<double>(what[3]);
            double z = boost::lexical_cast<double>(what[4]);

            structure->add_atom(elid, x, y, z);
        } else {
            throw std::runtime_error("Invalid atom line in MKS file: " + line);
        }
    }

    infile.close();

    return { structure };
}
