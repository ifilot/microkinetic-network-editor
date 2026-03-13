// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <memory>

#include <QString>

#include "shader_program.h"
#include "shader_program_types.h"

class ShaderProgramManager {
private:
    std::unordered_map<std::string, std::unique_ptr<ShaderProgram> > shader_program_map;

public:
    /**
     * @brief      Default constructor
     */
    ShaderProgramManager();

    /**
     * @brief      Get pointer to shader program
     *
     * @param[in]  name  The name
     *
     * @return     The shader program.
     */
    ShaderProgram* get_shader_program(const std::string& name);

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
    ShaderProgram* create_shader_program(const std::string& name, const ShaderProgramType type, const QString& vertex_filename, const QString& fragment_filename);

    /**
     * @brief      Bind a shader using name
     *
     * @param[in]  name  The name
     */
    void bind(const std::string& name);

    /**
     * @brief      Release a shader by name
     *
     * @param[in]  name  The name
     */
    void release(const std::string& name);

private:

};
