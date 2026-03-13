// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "model.h"

/**
 * @brief      Construct model object
 *
 * @param[in]  path  Path to object file
 */
Model::Model(std::vector<glm::vec3> _positions, std::vector<glm::vec3> _normals, std::vector<unsigned int> _indices) :
    positions(_positions),
    normals(_normals),
    indices(_indices) {}

/**
 * @brief add_instance.
 */
void Model::add_instance(const glm::vec3& scale, const glm::mat4& rotation, const glm::vec3& translation, const glm::vec4& color) {
    this->instances.push_back({scale, rotation, translation, color});
}

/**
 * @brief Model destructor.
 */
Model::~Model() {
    this->vao.bind();
    for(unsigned int i=0; i<4; i++) {
        this->vbo[i].destroy();
    }
    this->vao.destroy();
}

/**
 * @brief      Draw the model
 */
void Model::draw() {
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    this->vao.bind();
    f->glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
    this->vao.release();
}

/**
 * @brief      Get maximum vector
 *
 * @return     The maximum vector distance
 */
glm::vec3 Model::get_max_dim() const {
    float maxdist2 = 0.0;
    glm::vec3 vmax(0.0);
    for(const glm::vec3& v : this->positions) {
        float dist2 = glm::length2(v);
        if(dist2 > maxdist2) {
            maxdist2 = dist2;
            vmax = v;
        }
    }

    return vmax;
}

/**
 * @brief      Load all data to a vertex array object
 */
void Model::load_to_vao() {
    // no need to load a model twice, just early exit when called
    if(this->flag_loaded_vao) {
        return;
    }

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    // qDebug() << "Loading object onto GPU:";
    // qDebug() << "\tVertices:\t" << this->positions.size();
    // qDebug() << "\tNormals:\t" << this->normals.size();
    // qDebug() << "\tIndices:\t" << this->indices.size();
    // qDebug() << "\tFacets:\t\t" << (indices.size() / 3);

    this->vao.create();
    this->vao.bind();

    this->vbo[0].create();
    this->vbo[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo[0].bind();
    this->vbo[0].allocate(&this->positions[0][0], this->positions.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo[1].create();
    this->vbo[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo[1].bind();
    this->vbo[1].allocate(&this->normals[0][0], this->normals.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo[2] = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    this->vbo[2].create();
    this->vbo[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo[2].bind();
    this->vbo[2].allocate(&this->indices[0], this->indices.size() * sizeof(unsigned int));

    this->vao.release();
    this->flag_loaded_vao = true;
}
