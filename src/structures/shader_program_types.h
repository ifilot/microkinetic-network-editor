// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

// Define different shader program types so that each type can be linked to a
// set of uniforms
enum class ShaderProgramType {
    ModelShader,
    StereoscopicShader,
    AxesShader,
    UnitcellShader
};
