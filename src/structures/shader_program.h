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

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <QOpenGLShaderProgram>
#include <QString>

#include "shader_program_types.h"

class ShaderProgram {
private:
    QOpenGLShaderProgram *m_program;
    ShaderProgramType type;

    std::string name;
    QString vertex_filename;
    QString fragment_filename;

    std::unordered_map<std::string, int> uniforms;

    /**
     * @brief add_attributes.
     */
    void add_attributes();
    /**
     * @brief add_uniforms.
     */
    void add_uniforms();

public:
   /**
    * @brief ShaderProgram.
    */
    ShaderProgram(const std::string& _name, const ShaderProgramType type, const QString& vertex_filename, const QString& fragment_filename);

    template <typename T>
    /**
     * @brief set_uniform.
     */
    void set_uniform(const std::string &name, T const &value) {
        auto got = this->uniforms.find(name);

        if (got == this->uniforms.end()) {
            /**
             * @brief logic_error.
             */
            throw std::logic_error("Invalid uniform name: " + name);
        }

        this->m_program->setUniformValue(got->second, value);
    }

    /**
     * @brief bind.
     */
    inline bool bind() {
        return this->m_program->bind();
    }

    /**
     * @brief release.
     */
    inline void release() {
        this->m_program->release();
    }

    /**
     * @brief get_type.
     */
    inline ShaderProgramType get_type() {
        return this->type;
    }

    /**
     * @brief ShaderProgram.
     */
    ~ShaderProgram();
};
