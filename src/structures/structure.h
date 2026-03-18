// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#include <algorithm>

#include "matrixmath.h"
#include "atom.h"
#include "bond.h"
#include "atom_settings.h"

/**
 * @brief      This class describes a chemical structure.
 */
class Structure {
private:
    std::vector<Atom> atoms;                // atoms in the structure
    std::vector<Bond> bonds;                // bonds between the atoms

    std::vector<Atom> atoms_expansion;  // atoms in the unit cell expansion
    std::vector<Bond> bonds_expansion;  // bonds in the unit cell expansion

    double energy = 0.0;                    // energy of the structure (if known, zero otherwise)
    std::vector<VectorPosition> forces;     // forces on the atoms (if known, empty array otherwise)

    MatrixUnitcell unitcell;                // matrix describing the unit cell

    std::unordered_map<std::string, unsigned int> element_types;    // elements present in the structure

    bool localized = false; //flag to specify whether this calculation originates from a localized calculation
    bool xy_expansion_enabled_ = false; // whether XY unit-cell expansion should be built

public:
    /**
     * @brief      Constructs a new instance.
     */
    Structure(const MatrixUnitcell& unitcell, bool _localized = false);

    /*
     * @brief      Set unitcell dimensions
     */
    inline void set_unitcell(const MatrixUnitcell& _unitcell) {
        this->unitcell = _unitcell;
    }

    /**
     * @brief      Constructs a new instance.
     */
    Structure(unsigned int elnr);

    /**
     * @brief      Sets the energy.
     *
     * @param[in]  _energy  The energy
     */
    inline void set_energy(double _energy) {
        this->energy = _energy;
    }

    /**
     * @brief      Gets the energy.
     *
     * @return     The energy.
     */
    double get_energy() const {
        return this->energy;
    }

    /**
     * @brief      Gets the root mean square force
     *
     * @return     The root mean square force.
     */
    double get_rms_force() const;

    /**
     * @brief      Get all atoms from the structure
     *
     * @return     The atoms.
     */
    inline const auto& get_atoms() const {
        return this->atoms;
    }

    /**
     * @brief      Get all atoms from the structure
     *
     * @return     The atoms.
     */
    inline const auto& get_expansion_atoms() const {
        return this->atoms_expansion;
    }

    /**
     * @brief      Get all bonds from the structure
     *
     * @return     The atoms.
     */
    inline const auto& get_bonds() const {
        return this->bonds;
    }

    /**
     * @brief      Get all bonds from the structure
     *
     * @return     The atoms.
     */
    inline const auto& get_expansion_bonds() const {
        return this->bonds_expansion;
    }

    /**
     * @brief      Get specific atom
     *
     * @param[in]  idx   The index
     *
     * @return     The atom.
     */
    inline const Atom& get_atom(unsigned int idx) const {
        return this->atoms[idx];
    }

    /**
     * @brief      Gets the unitcell.
     *
     * @return     The unitcell.
     */
    inline const auto& get_unitcell() const {
        return this->unitcell;
    }

    /**
     * @brief      Add an atom to the structure
     *
     * @param[in]  atnr  Atom number
     * @param[in]  x     x coordinate
     * @param[in]  y     y coordinate
     * @param[in]  z     z coordinate
     */
    void add_atom(unsigned int atnr, double x, double y, double z);

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
    void add_atom(unsigned int atnr, double x, double y, double z, double fx, double fy, double fz);

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
    void add_atom(unsigned int atnr, double x, double y, double z, bool sx, bool sy, bool sz);

    /**
     * @brief      Gets the total number of atoms.
     *
     * @return     The number of atoms
     */
    inline size_t get_nr_atoms() const {
        return this->atoms.size();
    }

    /**
     * @brief Enable/disable building of XY expansion atoms and bonds.
     */
    inline void set_xy_expansion_enabled(bool enabled) {
        this->xy_expansion_enabled_ = enabled;
    }

    /**
     * @brief Get whether XY expansion is enabled.
     */
    inline bool get_xy_expansion_enabled() const {
        return this->xy_expansion_enabled_;
    }

    //********************************************
    // [END BLOCK] DATA GETTERS AND SETTERS
    //********************************************

    /**
     * @brief      Gets the elements in this structure as a string
     *
     * @return     String holding comma separated list of elements
     */
    std::string get_elements_string() const;

    /**
     * @brief      Updates the object.
     */
    void update();

private:
    /**
     * @brief      Count the number of elements
     */
    void count_elements();

    /**
     * @brief      Center the structure at the origin
     */
    void center();

    /**
     * @brief      Center the structure at the origin
     */
    void center_geometrical();

    /**
     * @brief      Construct the bonds
     */
    void construct_bonds();

    /**
     * @brief      Expand unit cell
     */
    void build_expansion();
};
