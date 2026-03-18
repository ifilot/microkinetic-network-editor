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

#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QDebug>

#include <chrono>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <exception>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

class Model {
private:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    bool flag_loaded_vao = false;

public:
    struct Instance {
        glm::vec3 scale;
        glm::mat4 rotation;
        glm::vec3 translation;
        glm::vec4 color;
    };

private:
    std::vector<Instance> instances;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo[4];

public:
    /**
     * @brief Constructor function
     *
     * @param[in] _positions  Vertex positions.
     * @param[in] _normals    Vertex normals.
     * @param[in] _indices    Triangle indices.
     */
    Model(std::vector<glm::vec3> _positions, std::vector<glm::vec3> _normals, std::vector<unsigned int> _indices);

    /**
     * @brief add_instance.
     */
    void add_instance(const glm::vec3& scale, const glm::mat4& rotation, const glm::vec3& translation, const glm::vec4& color);

    /**
     * @brief      Convenience function for adding an instance
     */
    inline void add_instance_default() {
        this->add_instance_color(glm::vec4(1.0));
    }

    /**
     * @brief      Convenience function for adding an instance by only
     *             specifying a color
     *
     * @param[in]  _color  The color
     */
    inline void add_instance_color(const glm::vec4& _color) {
        this->add_instance(glm::vec3(1.0), glm::mat4(1.0), glm::vec3(0.0), _color);
    }

    /**
     * @brief Overwrite instance properties
     *
     * @param[in] id            Instance identifier.
     * @param[in] _scale        Scale vector.
     * @param[in] _rotation     Rotation matrix.
     * @param[in] _translation  Translation vector.
     * @param[in] _color        RGBA color vector.
     */
    inline void set_instance_properties(size_t id,
                             const glm::vec3& _scale,
                             const glm::mat4& _rotation,
                             const glm::vec3& _translation,
                             const glm::vec4& _color) {

        if(id >= this->instances.size()) {
            /**
             * @brief runtime_error.
             */
            throw std::runtime_error("id exceeds vector length");
        }

        this->instances[id].scale = _scale;
        this->instances[id].rotation = _rotation;
        this->instances[id].translation = _translation;
        this->instances[id].color = _color;
    }

    /**
     * @brief      Draw the model
     */
    void draw();

    /**
     * @brief      Destroys the object.
     */
    ~Model();

    /**
     * @brief      Get maximum vector
     *
     * @return     The maximum vector distance
     */
    glm::vec3 get_max_dim() const;

    /**
     * @brief      Load all data to a vertex array object
     */
    void load_to_vao();

    /**
     * @brief get_instances.
     */
    inline const auto& get_instances() const {
        return this->instances;
    }

    /**
     * @brief get_num_vertices.
     */
    inline size_t get_num_vertices() const {
        return this->positions.size();
    }

    /**
     * @brief get_num_normals.
     */
    inline size_t get_num_normals() const {
        return this->normals.size();
    }

    /**
     * @brief get_num_indices.
     */
    inline size_t get_num_indices() const {
        return this->indices.size();
    }

    /**
     * @brief is_loaded.
     */
    inline bool is_loaded() const {
        return this->flag_loaded_vao;
    }
};
