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

#include <array>

#include "matrixmath.h"

enum {
    ATOM_CENTRAL_UNITCELL,
    ATOM_EXPANSION_XY,
    ATOM_EXPANSION_Z
};

class Atom {
public:
    unsigned int atnr;
    double x,y,z;
    double fx, fy, fz;
    unsigned int atomtype;
    unsigned int select = 0;
    std::array<bool, 3> selective_dynamics = {true, true, true};

   /**
    * @brief Atom.
    */
    Atom(unsigned int _atnr, double _x, double _y, double _z, unsigned int _atomtype = (1 << ATOM_CENTRAL_UNITCELL));

    /**
     * @brief      Gets the position.
     *
     * @return     The position.
     */
    inline std::array<double,3> get_pos() const {
        return {this->x, this->y, this->z};
    }

    /**
     * @brief      Gets the position.
     *
     * @return     The position.
     */
    inline VectorPosition get_vector_pos() const {
        VectorPosition v;
        v[0] = this->x;
        v[1] = this->y;
        v[2] = this->z;
        return v;
    }

    /**
     * @brief      Distance between two atoms
     *
     * @param[in]  other  The other atom
     *
     * @return     Distance
     */
    double dist(const Atom& other) const;

    /**
     * @brief      Squared distance between two atoms
     *
     * @param[in]  other  The other atom
     *
     * @return     Squared distance
     */
    double dist2(const Atom& other) const;

    /**
     * @brief      Translate an atom
     *
     * @param[in]  dx    translation x
     * @param[in]  dy    translation y
     * @param[in]  dz    translation z
     */
    inline void translate(double dx, double dy, double dz) {
        this->x += dx;
        this->y += dy;
        this->z += dz;
    }

    /**
     * @brief      Select this atom
     */
    void select_atom();

private:
};
