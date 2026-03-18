// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>

#version 330 core

// Vertex attributes (model space)
in vec3 position;   // vertex position in model space
in vec3 normal;     // vertex normal in model space

// Outputs to fragment shader
out vec3 normal_worldspace;          // normal -> world space
out vec3 normal_eyespace;            // normal -> eye space
out vec3 vertex_direction_eyespace;  // fragment -> camera direction (eye space)
out vec3 lightdirection_eyespace;    // fragment -> light direction (eye space)

// Transformation matrices
uniform mat4 mvp;    // model -> view -> projection
uniform mat4 model;  // model -> world
uniform mat4 view;   // world -> eye (camera)

// Light position in world space
uniform vec3 light_pos;

void main()
{
    // Transform vertex position into clip space
    gl_Position = mvp * vec4(position, 1.0);

    // ---- Position calculations ----

    // Vertex position in world space
    vec3 position_world = (model * vec4(position, 1.0)).xyz;

    // Vertex position in eye space (camera at origin)
    vec3 position_eye = (view * vec4(position_world, 1.0)).xyz;

    // Direction from vertex -> camera (eye space)
    vertex_direction_eyespace = -position_eye;

    // ---- Light direction ----

    // Direction from vertex -> light (world space)
    vec3 light_dir_world = light_pos - position_world;

    // Convert light direction world -> eye space
    // w = 0 → direction vector (ignore translation)
    lightdirection_eyespace =
        (view * vec4(light_dir_world, 0.0)).xyz;

    // ---- Normal transformation ----

    // Correct normal transform (handles non-uniform scaling)

    // model -> world
    normal_worldspace =
        (transpose(inverse(model)) * vec4(normal, 0.0)).xyz;

    // model -> eye
    normal_eyespace =
        (transpose(inverse(view * model)) * vec4(normal, 0.0)).xyz;
}