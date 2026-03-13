// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "structure.h"

Structure::Structure(const MatrixUnitcell& _unitcell, bool _localized) :
unitcell(_unitcell),
localized(_localized) {}

/**
 * @brief      Constructs a new instance.
 */
Structure::Structure(unsigned int elnr) {
    this->unitcell = MatrixUnitcell::Identity() * 2.5f;
    this->atoms.emplace_back(elnr, 0.0, 0.0, 0.0);
}

/**
 * @brief      Gets the root mean square force
 *
 * @return     The root mean square force.
 */
double Structure::get_rms_force() const {
    double sum = 0.0;
    for(const auto& force : this->forces) {
        sum += force.squaredNorm();
    }

    return sum / (double)this->forces.size();
}

/**
 * @brief      Add an atom to the structure
 *
 * @param[in]  atnr  Atom number
 * @param[in]  x     x coordinate
 * @param[in]  y     y coordinate
 * @param[in]  z     z coordinate
 */
void Structure::add_atom(unsigned int atnr, double x, double y, double z) {
    this->atoms.emplace_back(atnr, x, y, z);
}

/**
 * @brief      Add an atom to the structure including forces
 *
 * @param[in]  atnr  Atom number
 * @param[in]  x     x coordinate
 * @param[in]  y     y coordinate
 * @param[in]  z     z coordinate
 * @param[in]  fx    force in x direction
 * @param[in]  fy    force in y direction
 * @param[in]  fz    force in z direction
 */
/**
 * @brief add_atom.
 */
void Structure::add_atom(unsigned int atnr, double x, double y, double z, double fx, double fy, double fz) {
    this->add_atom(atnr, x, y, z);
    this->atoms.back().fx = fx;
    this->atoms.back().fy = fy;
    this->atoms.back().fz = fz;
}

/**
 * @brief      Add an atom to the structure including forces
 *
 * @param[in]  atnr  Atom number
 * @param[in]  x     x coordinate
 * @param[in]  y     y coordinate
 * @param[in]  z     z coordinate
 * @param[in]  sx    Selective dynamics x direction
 * @param[in]  sy    Selective dynamics y direction
 * @param[in]  sz    Selective dynamics z direction
 */
/**
 * @brief add_atom.
 */
void Structure::add_atom(unsigned int atnr, double x, double y, double z, bool sx, bool sy, bool sz) {
    this->add_atom(atnr, x, y, z);
    this->atoms.back().selective_dynamics = {sx, sy, sz};
}

/**
 * @brief      Gets the elements in this structure as a string
 *
 * @return     String holding comma seperated list of elements
 */
std::string Structure::get_elements_string() const {
    std::string result;

    for(const auto& item : this->element_types) {
        result += (boost::format("%s (%i); ") % item.first % item.second).str();
    }

    // remove last two characters
    result.pop_back();
    result.pop_back();

    return result;
}

/**
 * @brief      Updates the object.
 */
void Structure::update() {
    if(this->localized) {
        // use geometrical centering when the calculation is based on a localized orbital approach
        this->center_geometrical();
    } else {
        // otherwise, use 'conventional' unit cell centering
        this->center();
    }
    this->build_expansion();
    this->construct_bonds();
}

/**
 * @brief      Construct the bonds
 */
void Structure::construct_bonds() {
    this->bonds.clear();
    this->bonds_expansion.clear();

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        const auto& atom1 = this->atoms[i];
        for(unsigned int j=i+1; j<this->atoms.size(); j++) {
            const auto& atom2 = this->atoms[j];
            double maxdist2 = AtomSettings::get().get_bond_distance(atom1.atnr, atom2.atnr);

            double dist2 = atom1.dist(atom2);

            // check if atoms are bonded
            if(dist2 < maxdist2) {
                this->bonds.emplace_back(atom1, atom2, i, j);
            }
        }
    }

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        const auto& atom1 = this->atoms[i];
        for(unsigned int j=0; j<this->atoms_expansion.size(); j++) {
            const auto& atom2 = this->atoms_expansion[j];
            double maxdist2 = AtomSettings::get().get_bond_distance(atom1.atnr, atom2.atnr);

            double dist2 = atom1.dist(atom2);

            // check if atoms are bonded
            if(dist2 < maxdist2) {
                this->bonds_expansion.emplace_back(atom1, atom2, i, j+this->atoms.size());
            }
        }
    }
    for(unsigned int i=0; i<this->atoms_expansion.size(); i++) {
        const auto& atom1 = this->atoms_expansion[i];
        for(unsigned int j=i+1; j<this->atoms_expansion.size(); j++) {
            const auto& atom2 = this->atoms_expansion[j];
            double maxdist2 = AtomSettings::get().get_bond_distance(atom1.atnr, atom2.atnr);

            double dist2 = atom1.dist(atom2);

            // check if atoms are bonded
            if(dist2 < maxdist2) {
                this->bonds_expansion.emplace_back(atom1, atom2, i+this->atoms.size(), j+this->atoms.size());
            }
        }
    }
}

/**
 * @brief      Count the number of elements
 */
void Structure::count_elements() {
    this->element_types.clear();

    for(const auto& atom : this->atoms) {
        std::string atomname = AtomSettings::get().get_name_from_elnr(atom.atnr);
        auto got = this->element_types.find(atomname);
        if(got != this->element_types.end()) {
            got->second++;
        } else {
            this->element_types.emplace(atomname, 1);
        }
    }
}

/**
 * @brief      Center the structure at the origin
 */
void Structure::center() {
    double sumx = 0.0;
    double sumy = 0.0;
    double sumz = 0.0;

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        sumx += this->atoms[i].x;
    }

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        sumy += this->atoms[i].y;
    }

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        sumz += this->atoms[i].z;
    }

    sumx /= (float)this->atoms.size();
    sumy /= (float)this->atoms.size();
    sumz /= (float)this->atoms.size();

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        this->atoms[i].x -= sumx;
        this->atoms[i].y -= sumy;
        this->atoms[i].z -= sumz;
    }
}

/**
 * @brief      Center the structure at the origin
 */
void Structure::center_geometrical() {
    // keep track of molecular dimensions
    double xmin = 1000, xmax = -1000;
    double ymin = 1000, ymax = -1000;
    double zmin = 1000, zmax = -1000;

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        xmin = std::min(xmin, this->atoms[i].x);
        ymin = std::min(ymin, this->atoms[i].y);
        zmin = std::min(zmin, this->atoms[i].z);

        xmax = std::max(xmax, this->atoms[i].x);
        ymax = std::max(ymax, this->atoms[i].y);
        zmax = std::max(zmax, this->atoms[i].z);
    }

    double ctrx = (xmax + xmin) / 2.0;
    double ctry = (ymax + ymin) / 2.0;
    double ctrz = (zmax + zmin) / 2.0;

    for(unsigned int i=0; i<this->atoms.size(); i++) {
        this->atoms[i].x -= ctrx;
        this->atoms[i].y -= ctry;
        this->atoms[i].z -= ctrz;
    }
}

/**
 * @brief      Expand unit cell
 */
void Structure::build_expansion() {
    this->atoms_expansion.clear();

    VectorPosition p;
    int z = 0;
    for(int y=-1; y<=1; y++) {
        p[1] = y;
        for(int x=-1; x<=1; x++) {
            p[0] = x;
            if(!(x == 0 && y == 0 && z == 0)) {
                VectorPosition dp = this->unitcell.transpose() * p;
                for(const auto& atom : this->atoms) {
                    unsigned int atomtype = 0;
                    if(z != 0) {
                        atomtype |= (1 << ATOM_EXPANSION_Z);
                    }

                    if(x != 0 || y != 0) {
                        atomtype |= (1 << ATOM_EXPANSION_XY);
                    }

                    this->atoms_expansion.emplace_back(atom.atnr, atom.x, atom.y, atom.z, atomtype);
                    this->atoms_expansion.back().translate(dp[0], dp[1], dp[2]);
                }
            }
        }
    }
}
