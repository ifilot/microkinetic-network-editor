// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "atom.h"

class Bond {
public:
    // atom 1
    Atom atom1;
    Atom atom2;

    uint16_t atom_id_1;
    uint16_t atom_id_2;

    // length of the bond
    double length;

    // bond orientation
    Vec3d direction;
    Vec3d axis;
    double angle;

   /**
    * @brief Bond.
    */
    Bond(const Atom& _atom1, const Atom& _atom2, uint16_t i, uint16_t j);

private:
};
