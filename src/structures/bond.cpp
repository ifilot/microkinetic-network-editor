// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "bond.h"

Bond::Bond(const Atom& _atom1, const Atom& _atom2, uint16_t i, uint16_t j) :
atom1(_atom1),
atom2(_atom2),
atom_id_1(i),
atom_id_2(j) {
    Vec3d v = this->atom2.get_vector_pos() - this->atom1.get_vector_pos();

    this->length = v.norm();
    this->direction = v.normalized();

    // avoid gimball locking
    if (fabs(this->direction[2]) > .999) {
        if(this->direction[2] < 0.0) {
            this->axis = Vec3d(0.0, 1.0, 0.0);
            this->angle = -M_PI;
        } else {
            this->axis = Vec3d(0.0, 0.0, 1.0);
            this->angle = 0.0;
        }
    } else {
        this->axis = Vec3d(0.0, 0.0, 1.0).cross(this->direction);
        this->angle = std::acos(this->direction[2]);
    }
}
