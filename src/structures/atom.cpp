// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "atom.h"

Atom::Atom(unsigned int _atnr, double _x, double _y, double _z, unsigned int _atomtype) :
atnr(_atnr),
x(_x),
y(_y),
z(_z),
atomtype(_atomtype) {

}

/**
 * @brief      Distance between two atoms
 *
 * @param[in]  other  The other atom
 *
 * @return     Distance
 */
double Atom::dist(const Atom& other) const {
    return std::sqrt(this->dist2(other));
}

/**
 * @brief      Squared distance between two atoms
 *
 * @param[in]  other  The other atom
 *
 * @return     Squared distance
 */
double Atom::dist2(const Atom& other) const {
    return (this->x - other.x) * (this->x - other.x) +
           (this->y - other.y) * (this->y - other.y) +
           (this->z - other.z) * (this->z - other.z);
}

/**
 * @brief      Select this atom
 */
void Atom::select_atom() {
    select++;
    select = select % 3;
}
