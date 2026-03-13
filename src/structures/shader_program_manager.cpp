// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "shader_program_manager.h"

ShaderProgramManager::ShaderProgramManager() {}

/**
 * @brief      Get pointer to shader program
 *
 * @param[in]  name  The name
 *
 * @return     The shader program.
 */
ShaderProgram* ShaderProgramManager::get_shader_program(const std::string& name) {
    auto got = this->shader_program_map.find(name);

    if (got != this->shader_program_map.end()) {
        // shader program is in map, so return it
        return got->second.get();
    } else {
        throw std::runtime_error("Unknown shader program: " + name);
    }
}

/**
 * @brief      Creates a shader program.
 *
 * @param[in]  name               The name
 * @param[in]  type               The type
 * @param[in]  vertex_filename    The vertex filename
 * @param[in]  fragment_filename  The fragment filename
 *
 * @return     pointer to shader program
 */
/**
 * @brief create_shader_program.
 */
ShaderProgram* ShaderProgramManager::create_shader_program(const std::string& name, const ShaderProgramType type, const QString& vertex_filename, const QString& fragment_filename) {
    // create program
    ShaderProgram* m_program = new ShaderProgram(name, type, vertex_filename, fragment_filename);

    // add new shader program to unordered map
    this->shader_program_map.emplace(name, m_program);

    // return pointer to shader program
    return m_program;
}

/**
 * @brief      Bind a shader using name
 *
 * @param[in]  name  The name
 */
void ShaderProgramManager::bind(const std::string& name) {
    this->get_shader_program(name)->bind();
}

/**
 * @brief      Release a shader by name
 *
 * @param[in]  name  The name
 */
void ShaderProgramManager::release(const std::string& name) {
    this->get_shader_program(name)->release();
}
