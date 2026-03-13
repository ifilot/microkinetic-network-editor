// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


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
