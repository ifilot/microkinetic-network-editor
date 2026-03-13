// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "shader_program.h"

/**
 * @brief ShaderProgram.
 */
ShaderProgram::ShaderProgram(const std::string& _name, const ShaderProgramType type, const QString& vertex_filename, const QString& fragment_filename) {
    this->name = _name;
    this->type = type;
    this->vertex_filename = vertex_filename;
    this->fragment_filename = fragment_filename;

    this->m_program = new QOpenGLShaderProgram;

    if (!this->m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertex_filename)) {
        throw std::runtime_error("Could not add vertex shader: " + this->m_program->log().toStdString());
    }
    if (!this->m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragment_filename)) {
        throw std::runtime_error("Could not add fragment shader: " + this->m_program->log().toStdString());
    }

    this->add_attributes();

    if (!this->m_program->link()) {
        throw std::runtime_error("Could not link shader: " + this->m_program->log().toStdString());
    }

    this->add_uniforms();
}

/**
 * @brief ShaderProgram destructor.
 */
ShaderProgram::~ShaderProgram() {
    delete this->m_program;
    this->m_program = 0;
}

/**
 * @brief add_attributes.
 */
void ShaderProgram::add_attributes() {
    // add attributes depending on the shader program type
    switch(this->type) {
        case ShaderProgramType::ModelShader:
            this->m_program->bindAttributeLocation("position", 0);
            this->m_program->bindAttributeLocation("normal", 1);
        break;
        case ShaderProgramType::AxesShader:
            this->m_program->bindAttributeLocation("position", 0);
            this->m_program->bindAttributeLocation("normal", 1);
        break;
        default:
            // nothing to do
        break;
    }
}

/**
 * @brief add_uniforms.
 */
void ShaderProgram::add_uniforms() {
    // add uniforms depending on the shader program type
    if (this->type == ShaderProgramType::ModelShader) {
        qDebug() << "ModelShader: " << this->vertex_filename;
        this->uniforms.emplace("mvp", this->m_program->uniformLocation("mvp"));
        this->uniforms.emplace("model", this->m_program->uniformLocation("model"));
        this->uniforms.emplace("view", this->m_program->uniformLocation("view"));
        this->uniforms.emplace("lightpos", this->m_program->uniformLocation("lightpos"));
        this->uniforms.emplace("color", this->m_program->uniformLocation("color"));
    }

    if (this->type == ShaderProgramType::StereoscopicShader) {
        qDebug() << "StereoscopicShader: " << this->vertex_filename;
        this->uniforms.emplace("left_eye_texture", this->m_program->uniformLocation("left_eye_texture"));
        this->uniforms.emplace("right_eye_texture", this->m_program->uniformLocation("right_eye_texture"));
        this->uniforms.emplace("screen_x", this->m_program->uniformLocation("screen_x"));
        this->uniforms.emplace("screen_y", this->m_program->uniformLocation("screen_y"));
    }

    if (this->type == ShaderProgramType::AxesShader) {
        qDebug() << "AxesShader " << this->vertex_filename;
        this->uniforms.emplace("mvp", this->m_program->uniformLocation("mvp"));
        this->uniforms.emplace("model", this->m_program->uniformLocation("model"));
        this->uniforms.emplace("view", this->m_program->uniformLocation("view"));
        this->uniforms.emplace("light_pos", this->m_program->uniformLocation("light_pos"));
        this->uniforms.emplace("color", this->m_program->uniformLocation("color"));
    }

    if (this->type == ShaderProgramType::UnitcellShader) {
        qDebug() << "UnitcellShader " << this->vertex_filename;
        this->uniforms.emplace("mvp", this->m_program->uniformLocation("mvp"));
        this->uniforms.emplace("color", this->m_program->uniformLocation("color"));
    }
}
