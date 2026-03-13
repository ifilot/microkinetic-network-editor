// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


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
#include <boost/format.hpp>
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
     * @param vector of vertices
     * @param vector of normals
     * @param vector of indices
     */
    Model(std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, std::vector<unsigned int> indices);

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
     * @param id
     * @param scale vector
     * @param rotation matrix
     * @param translation vector
     * @param color vector
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
