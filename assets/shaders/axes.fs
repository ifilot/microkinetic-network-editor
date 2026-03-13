// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>

#version 330 core

// Inputs from the vertex shader
in vec3 normal_worldspace;          // normal in world space (optional)
in vec3 normal_eyespace;            // normal in eye space
in vec3 vertex_direction_eyespace;  // fragment -> camera direction (eye space)
in vec3 lightdirection_eyespace;    // fragment -> light direction (eye space)

// Final output color
out vec4 frag_color;

// Base surface color (albedo)
uniform vec3 color;

// Lighting strengths
const float ambient_strength  = 0.1;
const float specular_strength = 0.5;
const float shininess = 32.0;

void main() {

    float alpha = 1.0;

    // Light color (white)
    vec3 light_color = vec3(1.0);

    // ---- Normalize vectors ----

    vec3 light_dir = normalize(lightdirection_eyespace);
    vec3 normal    = normalize(normal_eyespace);
    vec3 view_dir  = normalize(vertex_direction_eyespace);

    // Reflection vector for specular highlight
    vec3 reflect_dir = reflect(-light_dir, normal);

    // ---- Diffuse lighting (Lambert) ----
    float diffuse_factor = clamp(dot(normal, light_dir), 0.0, 1.0);

    // ---- Specular lighting (Phong) ----
    float specular_factor = clamp(dot(view_dir, reflect_dir), 0.0, 1.0);

    // ---- Lighting components ----
    vec3 ambient  = ambient_strength * light_color;
    vec3 diffuse  = diffuse_factor * light_color;
    vec3 specular = pow(specular_factor, shininess)
                    * specular_strength * light_color;

    vec3 result = (ambient + diffuse + specular) * color;

    frag_color = vec4(result, alpha);
}