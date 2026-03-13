// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>
#include <QFile>
#include <QDataStream>

#include "model.h"

class ModelLoader {

private:

public:
   /**
    * @brief ModelLoader.
    */
    ModelLoader();

    /**
     * @brief load_model.
     */
    std::unique_ptr<Model> load_model(const std::string& path);

private:
    /**
     * @brief      Load object data from obj file
     *
     * @param[in]  path   Path to file
     */
    std::unique_ptr<Model> load_data_obj(const std::string& path);

    /**
     * @brief      Load object data from ply file
     *
     * @param[in]  path   Path to file
     */
    std::unique_ptr<Model> load_data_ply(const std::string& path);

    /**
     * @brief      Loads a ply file from hard drive stored as little endian binary
     *
     * @param[in]  path   Path to file
     */
    std::unique_ptr<Model> load_data_ply_binary(const std::string& path);

    /**
     * @brief      Loads a ply file from hard drive stored as little endian binary
     *
     * @param[in]  path   Path to file
     */
    std::unique_ptr<Model> load_data_ply_ascii(const std::string& path);
};
