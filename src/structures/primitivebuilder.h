// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QDebug>
#include <QMatrix4x4>
#include <QtMath>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <vector>

#include "matrixmath.h"


class PrimitiveBuilder {
private:
    // sphere facets
    std::vector<glm::vec3> sphere_vertices;
    std::vector<glm::vec3> sphere_normals;
    std::vector<unsigned int> sphere_indices;

    // cylinder facets
    std::vector<glm::vec3> cylinder_vertices;
    std::vector<glm::vec3> cylinder_normals;
    std::vector<unsigned int> cylinder_indices;

    // vao and vbo for rendering
    QOpenGLVertexArrayObject vao_sphere;
    QOpenGLBuffer vbo_sphere[3];

    QOpenGLVertexArrayObject vao_cylinder;
    QOpenGLBuffer vbo_cylinder[3];

    QOpenGLVertexArrayObject vao_unitcell;
    QOpenGLBuffer vbo_unitcell[2];

    MatrixUnitcell unitcell;

public:
   /**
    * @brief PrimitiveBuilder.
    */
    PrimitiveBuilder();

    /**
     * @brief build_models.
     */
    void build_models();

    /**
     * @brief set_unitcell.
     */
    inline void set_unitcell(const MatrixUnitcell& _unitcell) {
        this->unitcell = _unitcell;
        if(QOpenGLContext::currentContext()->isValid()) {
            this->generate_coordinates_unitcell(this->unitcell);
        }
    }

    // getters

    /**
     * @brief get_vao_sphere.
     */
    inline QOpenGLVertexArrayObject* get_vao_sphere() {
        return &this->vao_sphere;
    }

    /**
     * @brief get_vao_cylinder.
     */
    inline QOpenGLVertexArrayObject* get_vao_cylinder() {
        return &this->vao_cylinder;
    }

    /**
     * @brief get_vao_unitcell.
     */
    inline QOpenGLVertexArrayObject* get_vao_unitcell() {
        return &this->vao_unitcell;
    }

    /**
     * @brief get_num_vertices_sphere.
     */
    inline size_t get_num_vertices_sphere() const {
        return this->sphere_indices.size();
    }

    /**
     * @brief get_num_vertices_cylinder.
     */
    inline size_t get_num_vertices_cylinder() const {
        return this->cylinder_indices.size();
    }

    /**
     * @brief      Generate coordinates of a sphere
     *
     * @param[in]  tesselation_level  The tesselation level
     */
    void generate_sphere_coordinates(unsigned int tesselation_level);

    /**
     * @brief      Generate coordinates for a default cylinder (radius 1, height 1)
     *
     * @param[in]  stack_count  The stack count
     * @param[in]  slice_count  The slice count
     */
    void generate_cylinder_coordinates(unsigned int stack_count, unsigned int slice_count);

    /**
     * @brief      Generate the coordinates of the unitcell
     */
    void generate_coordinates_unitcell(const MatrixUnitcell& unitcell);

};
