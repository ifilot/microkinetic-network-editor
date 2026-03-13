// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "primitivebuilder.h"

/**
 * @brief PrimitiveBuilder.
 */
PrimitiveBuilder::PrimitiveBuilder() {
    this->unitcell = MatrixUnitcell(3,3);
    this->unitcell.diagonal() << 1,1,1;
}

/**
 * @brief build_models.
 */
void PrimitiveBuilder::build_models() {
    qDebug() << "Loading primitives";
    this->generate_sphere_coordinates(3);
    this->generate_cylinder_coordinates(2, 24);
    this->generate_coordinates_unitcell(this->unitcell);
}

/**
 * @brief      Generate coordinates of a sphere
 *
 * @param[in]  tesselation_level  The tesselation level
 */
void PrimitiveBuilder::generate_sphere_coordinates(unsigned int tesselation_level) {
    std::vector<glm::vec3> vertices;

    vertices.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    vertices.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    vertices.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
    vertices.push_back(glm::vec3(0.0f, 0.0f, -1.0f));

    std::vector<unsigned int> triangles;
    triangles.resize(24);

    triangles[0] = 0;
    triangles[1] = 3;
    triangles[2] = 5;

    triangles[3] = 3;
    triangles[4] = 1;
    triangles[5] = 5;

    triangles[6] = 3;
    triangles[7] = 4;
    triangles[8] = 1;

    triangles[9] = 0;
    triangles[10] = 4;
    triangles[11] = 3;

    triangles[12] = 2;
    triangles[13] = 0;
    triangles[14] = 5;

    triangles[15] = 2;
    triangles[16] = 5;
    triangles[17] = 1;

    triangles[18] = 4;
    triangles[19] = 0;
    triangles[20] = 2;

    triangles[21] = 4;
    triangles[22] = 2;
    triangles[23] = 1;

    std::vector<unsigned int> new_triangles;

    for (unsigned int j = 0; j < tesselation_level; j++) {
        new_triangles.resize(0);
        unsigned int size = triangles.size();

        for (unsigned int i = 0; i < size; i += 3) {
            glm::vec3 center1 = glm::normalize((vertices[triangles[i]] + vertices[triangles[i+1]]) / 2.0f);
            glm::vec3 center2 = glm::normalize((vertices[triangles[i]] + vertices[triangles[i+2]]) / 2.0f);
            glm::vec3 center3 = glm::normalize((vertices[triangles[i+1]] + vertices[triangles[i+2]]) / 2.0f);

            vertices.push_back(center1);
            unsigned int a = vertices.size() - 1;
            vertices.push_back(center2);
            unsigned int b = vertices.size() - 1;
            vertices.push_back(center3);
            unsigned int c = vertices.size() - 1;

            new_triangles.push_back(triangles[i]);
            new_triangles.push_back(a);
            new_triangles.push_back(b);

            new_triangles.push_back(triangles[i+1]);
            new_triangles.push_back(c);
            new_triangles.push_back(a);

            new_triangles.push_back(triangles[i+2]);
            new_triangles.push_back(b);
            new_triangles.push_back(c);

            new_triangles.push_back(a);
            new_triangles.push_back(c);
            new_triangles.push_back(b);
        }
        triangles = new_triangles;
    }

    this->sphere_vertices = vertices;
    this->sphere_normals = vertices;  // for a sphere, vertices and normals are equal
    this->sphere_indices = triangles;

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    this->vao_sphere.create();
    this->vao_sphere.bind();

    this->vbo_sphere[0].create();
    this->vbo_sphere[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_sphere[0].bind();
    this->vbo_sphere[0].allocate(&this->sphere_vertices[0][0], this->sphere_vertices.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo_sphere[1].create();
    this->vbo_sphere[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_sphere[1].bind();
    this->vbo_sphere[1].allocate(&this->sphere_normals[0][0], this->sphere_normals.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo_sphere[2] = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    this->vbo_sphere[2].create();
    this->vbo_sphere[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_sphere[2].bind();
    this->vbo_sphere[2].allocate(&this->sphere_indices[0], this->sphere_indices.size() * sizeof(unsigned int));

    this->vao_sphere.release();
}

/**
 * @brief      Generate coordinates for a default cylinder (radius 1, height 1)
 *
 * @param[in]  stack_count  The stack count
 * @param[in]  slice_count  The slice count
 */
void PrimitiveBuilder::generate_cylinder_coordinates(unsigned int stack_count, unsigned int slice_count) {
    // construct vertices and normals
    for(unsigned int stack = 0; stack < stack_count; ++stack) {
        for(unsigned int slice = 0; slice < slice_count; ++slice) {
            float x = std::sin(2.0f * (float) M_PI * (float)slice / slice_count);
            float y = std::cos(2.0f * (float) M_PI * (float)slice / slice_count);
            float z = stack / (stack_count - 1.0f);

            this->cylinder_vertices.push_back(glm::vec3(x, y, z));

            glm::vec3 normal = glm::normalize(glm::vec3(x, y, 0));

            this->cylinder_normals.push_back(normal);
        }
    }

    // construct indices
    for (unsigned int stack = 0; stack < stack_count - 1; ++stack) {
        for (unsigned int slice = 0; slice < slice_count; ++slice) {
            // point 1
            this->cylinder_indices.push_back((float)stack * slice_count + slice);

            // point 4
            this->cylinder_indices.push_back(((float)stack + 1) * slice_count + slice);

            // point 3
            if (slice + 1 == slice_count) {
                this->cylinder_indices.push_back(((float)stack + 1) * slice_count);
            } else {
                this->cylinder_indices.push_back(((float)stack + 1) * slice_count + slice + 1);
            }

            // point 1
            this->cylinder_indices.push_back((float)stack * slice_count + slice);

            // point 3
            if (slice + 1 == slice_count) {
                this->cylinder_indices.push_back(((float)stack + 1) * slice_count);
            } else {
                this->cylinder_indices.push_back(((float)stack + 1) * slice_count + slice + 1);
            }

            // point 2
            if (slice + 1 == slice_count) {
                this->cylinder_indices.push_back((float)stack * slice_count);
            } else {
                this->cylinder_indices.push_back((float)stack * slice_count + slice + 1);
            }
        }
    }

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    this->vao_cylinder.create();
    this->vao_cylinder.bind();

    this->vbo_cylinder[0].create();
    this->vbo_cylinder[0].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_cylinder[0].bind();
    this->vbo_cylinder[0].allocate(&this->cylinder_vertices[0][0], this->cylinder_vertices.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo_cylinder[1].create();
    this->vbo_cylinder[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_cylinder[1].bind();
    this->vbo_cylinder[1].allocate(&this->cylinder_normals[0][0], this->cylinder_normals.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo_cylinder[2] = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    this->vbo_cylinder[2].create();
    this->vbo_cylinder[2].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_cylinder[2].bind();
    this->vbo_cylinder[2].allocate(&this->cylinder_indices[0], this->cylinder_indices.size() * sizeof(unsigned int));

    this->vao_cylinder.release();
}

/**
 * @brief      Generate the coordinates of the unitcell
 */
void PrimitiveBuilder::generate_coordinates_unitcell(const MatrixUnitcell& unitcell) {
    // set vertices of the unitcell
    std::vector<glm::vec3> unitcell_vertices;
    unitcell_vertices.push_back(glm::vec3(0,0,0));
    unitcell_vertices.push_back(glm::vec3(unitcell(0,0), unitcell(0,1), unitcell(0,2)));
    unitcell_vertices.push_back(glm::vec3(unitcell(1,0), unitcell(1,1), unitcell(1,2)));
    unitcell_vertices.push_back(glm::vec3(unitcell(2,0), unitcell(2,1), unitcell(2,2)));
    unitcell_vertices.push_back(unitcell_vertices[1] + unitcell_vertices[2]);
    unitcell_vertices.push_back(unitcell_vertices[1] + unitcell_vertices[3]);
    unitcell_vertices.push_back(unitcell_vertices[2] + unitcell_vertices[3]);
    unitcell_vertices.push_back(unitcell_vertices[4] + unitcell_vertices[3]);

    // translate the center
    glm::vec3 ctr = glm::vec3(0,0,0);
    for(const glm::vec3& p : unitcell_vertices) {
        ctr += p;
    }
    ctr /= unitcell_vertices.size();
    for(unsigned int i=0; i<unitcell_vertices.size(); i++) {
        unitcell_vertices[i] -= ctr;
    }

    // set indices of the unit cell
    std::vector<unsigned int> unitcell_indices;
    unitcell_indices.push_back(0);
    unitcell_indices.push_back(1);
    unitcell_indices.push_back(0);
    unitcell_indices.push_back(2);
    unitcell_indices.push_back(0);
    unitcell_indices.push_back(3);
    unitcell_indices.push_back(1);
    unitcell_indices.push_back(4);
    unitcell_indices.push_back(2);
    unitcell_indices.push_back(4);
    unitcell_indices.push_back(1);
    unitcell_indices.push_back(5);
    unitcell_indices.push_back(4);
    unitcell_indices.push_back(7);
    unitcell_indices.push_back(2);
    unitcell_indices.push_back(6);
    unitcell_indices.push_back(6);
    unitcell_indices.push_back(7);
    unitcell_indices.push_back(7);
    unitcell_indices.push_back(5);
    unitcell_indices.push_back(3);
    unitcell_indices.push_back(5);
    unitcell_indices.push_back(6);
    unitcell_indices.push_back(3);

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    if(!this->vao_unitcell.isCreated()) {
        qDebug() << "Creating unitcell VAO";
        this->vao_unitcell.create();
    } else {
        qDebug() << "Updating unitcell VAO";
    }
    this->vao_unitcell.bind();

    this->vbo_unitcell[0].create();
    this->vbo_unitcell[0].setUsagePattern(QOpenGLBuffer::DynamicDraw);
    this->vbo_unitcell[0].bind();
    this->vbo_unitcell[0].allocate(&unitcell_vertices[0][0], unitcell_vertices.size() * 3 * sizeof(float));
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    this->vbo_unitcell[1] = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    this->vbo_unitcell[1].create();
    this->vbo_unitcell[1].setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->vbo_unitcell[1].bind();
    this->vbo_unitcell[1].allocate(&unitcell_indices[0], unitcell_indices.size() * sizeof(unsigned int));

    this->vao_unitcell.release();
}
