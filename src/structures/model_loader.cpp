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


#include "model_loader.h"

ModelLoader::ModelLoader() {}

/**
 * @brief load_model.
 */
std::unique_ptr<Model> ModelLoader::load_model(const std::string& path) {
    if (path.substr(path.size()-4, 4) == ".obj") {
        return this->load_data_obj(path);
    }

    if (path.substr(path.size()-4, 4) == ".ply") {
        return this->load_data_ply(path);
    }

    // throw an exception if the function came till here
    throw std::runtime_error("Unknown extension: " + path);
}

/**
 * @brief      Load object data from obj file
 *
 * @param[in]  path   Path to file
 */
std::unique_ptr<Model> ModelLoader::load_data_obj(const std::string& path) {
    QFile file(path.c_str());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    // set regex patterns
    static const boost::regex v_line("v\\s+([0-9.-]+)\\s+([0-9.-]+)\\s+([0-9.-]+).*");
    static const boost::regex vn_line("vn\\s+([0-9.-]+)\\s+([0-9.-]+)\\s+([0-9.-]+).*");
    static const boost::regex vt_line("vt\\s+([0-9.-]+)\\s+([0-9.-]+).*");
    static const boost::regex f_line("f\\s+([0-9]+)\\/([0-9]+)\\/([0-9]+)\\s+([0-9]+)\\/([0-9]+)\\/([0-9]+)\\s+([0-9]+)\\/([0-9]+)\\/([0-9]+).*");
    static const boost::regex f2_line("f\\s+([0-9]+)\\/\\/([0-9]+)\\s+([0-9]+)\\/\\/([0-9]+)\\s+([0-9]+)\\/\\/([0-9]+).*");

    // construct holders
    std::vector<glm::vec3> _positions;
    std::vector<glm::vec3> _normals;
    std::vector<unsigned int> position_indices;
    std::vector<unsigned int> normal_indices;

    // construct vectors for final Model
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> colors;

    QTextStream in(&file);

    // start reading
    while (!in.atEnd()) {
         std::string line = in.readLine().toStdString();
         boost::smatch what1;

         // positions
        if (boost::regex_match(line, what1, v_line)) {
            glm::vec3 pos(boost::lexical_cast<float>(what1[1]),
                          boost::lexical_cast<float>(what1[2]),
                          boost::lexical_cast<float>(what1[3]));
            _positions.push_back(pos);
            continue;
        }

        // normals
        if (boost::regex_match(line, what1, vn_line)) {
            glm::vec3 normal(boost::lexical_cast<float>(what1[1]),
                             boost::lexical_cast<float>(what1[2]),
                             boost::lexical_cast<float>(what1[3]));
            _normals.push_back(normal);
            continue;
        }

        if (boost::regex_match(line, what1, f_line)) {
            position_indices.push_back(boost::lexical_cast<unsigned int>(what1[1]) - 1);
            position_indices.push_back(boost::lexical_cast<unsigned int>(what1[4]) - 1);
            position_indices.push_back(boost::lexical_cast<unsigned int>(what1[7]) - 1);

            normal_indices.push_back(boost::lexical_cast<unsigned int>(what1[3]) - 1);
            normal_indices.push_back(boost::lexical_cast<unsigned int>(what1[6]) - 1);
            normal_indices.push_back(boost::lexical_cast<unsigned int>(what1[9]) - 1);
            continue;
        }

        if (boost::regex_match(line, what1, f2_line)) {
            position_indices.push_back(boost::lexical_cast<unsigned int>(what1[1]) - 1);
            position_indices.push_back(boost::lexical_cast<unsigned int>(what1[3]) - 1);
            position_indices.push_back(boost::lexical_cast<unsigned int>(what1[5]) - 1);

            normal_indices.push_back(boost::lexical_cast<unsigned int>(what1[2]) - 1);
            normal_indices.push_back(boost::lexical_cast<unsigned int>(what1[4]) - 1);
            normal_indices.push_back(boost::lexical_cast<unsigned int>(what1[6]) - 1);
            continue;
        }
    }

    // restructure data
    for(unsigned int i=0; i<position_indices.size(); i++) {
        positions.push_back(_positions[position_indices[i]]);
        normals.push_back(_normals[normal_indices[i]]);
        indices.push_back(i);
    }

    auto model = std::make_unique<Model>(positions, normals, indices);
    return model;
}

/**
 * @brief      Load object data from ply file
 *
 * @param[in]  path   Path to file
 */
std::unique_ptr<Model> ModelLoader::load_data_ply(const std::string& path) {
    QFile file(path.c_str());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::string line = file.readLine().toStdString();
    boost::trim(line);
    if(line != "ply") {
        throw std::runtime_error("File with .ply extension does not start with \"ply\" header.");
    }

    line = file.readLine().toStdString();
    boost::trim(line);
    if(line == "format ascii 1.0") {
        file.close();
        return this->load_data_ply_ascii(path);
    }

    if(line == "format binary_little_endian 1.0") {
        file.close();
        return this->load_data_ply_binary(path);
    }

    throw std::runtime_error("Unsupported formatting encountered: " + line);
}

/**
 * @brief      Loads a ply file from hard drive stored as little endian binary
 *
 * @param[in]  path   Path to file
 */
std::unique_ptr<Model> ModelLoader::load_data_ply_binary(const std::string& path) {
    QFile file(path.c_str());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    // set regex patterns
    static const boost::regex comment_line("$comment.*");
    static const boost::regex element_vertex("element vertex ([0-9]+)");
    static const boost::regex element_face("element face ([0-9]+)");
    static const boost::regex property_float("property float [A-Za-z]+");
    static const boost::regex property_uchar("property uchar ([A-Za-z]+)");

    // construct vectors for final Model
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> colors;

    // start reading
    std::string line = file.readLine().toStdString();
    unsigned int headersz = line.size() + 1;
    boost::trim(line);
    unsigned int nrvertices = 0;
    unsigned int nrfaces = 0;
    bool has_colors = false;
    while(line != "end_header") {
        boost::smatch what;

        // discard comments
        if (boost::regex_match(line, what, comment_line)) {
        }

        if (boost::regex_match(line, what, element_vertex)) {
            nrvertices = boost::lexical_cast<unsigned int>(what[1]);
        }

        if (boost::regex_match(line, what, element_face)) {
            nrfaces = boost::lexical_cast<unsigned int>(what[1]);
        }

        if (boost::regex_match(line, what, property_uchar)) {
            if(what[1] == "red") {
                has_colors = true;
            }
        }

        line = file.readLine().toStdString();
        headersz += line.size() + 1;
        boost::trim(line);
    }
    headersz -= line.size() + 3;
    file.close();

    QFile fileraw(path.c_str());
    if (!fileraw.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    QDataStream in(&fileraw);
    in.skipRawData(headersz);

    // resize arrays
    positions.resize(nrvertices);
    normals.resize(nrvertices);
    colors.resize(nrvertices, glm::vec3(1.0f, 1.0f, 1.0f));
    indices.resize(nrfaces * 3);

    // read positions and normals
    for(unsigned int i=0; i<nrvertices; i++) {
        in.readRawData((char*)&positions[i][0], sizeof(float) * 3);
        in.readRawData((char*)&normals[i][0], sizeof(float) * 3);
        if(has_colors) {
            std::array<uint8_t, 3> col;
            in.readRawData((char*)&col[0], sizeof(uint8_t) * 3); // read the color data, but ignore it in the visualization
        }
    }

    // read faces
    uint8_t facesz = 0;
    for(unsigned int i=0; i<nrfaces; i++) {
        in.readRawData((char*)&facesz, sizeof(uint8_t));
        if(facesz != 3) {
            throw std::runtime_error("Unsupported face size encountered: " + std::to_string(facesz));
        }
        in.readRawData((char*)&indices[i * 3], sizeof(unsigned int) * 3);
    }

    file.close();

    auto model = std::make_unique<Model>(positions, normals, indices);
    return model;
}

/**
 * @brief      Loads a ply file from hard drive stored in ascii format
 *
 * @param[in]  path   The path
 */
std::unique_ptr<Model> ModelLoader::load_data_ply_ascii(const std::string& path) {
    std::ifstream file(path);

    if(file.is_open()) {
        std::string line;

        while(std::getline(file, line)) {
            if(line == "end_header") {
                break;
            }
        }

        // construct vectors for final Model
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;
        std::vector<glm::vec3> colors;

        while(std::getline(file, line)) {
            std::vector<std::string> pieces;
            boost::split(pieces, line, boost::is_any_of("\t "), boost::token_compress_on);
            if(pieces.size() == 9) {
                positions.emplace_back(boost::lexical_cast<float>(pieces[0]), boost::lexical_cast<float>(pieces[1]), boost::lexical_cast<float>(pieces[2]));
                normals.emplace_back(boost::lexical_cast<float>(pieces[3]), boost::lexical_cast<float>(pieces[4]), boost::lexical_cast<float>(pieces[5]));
                colors.emplace_back(boost::lexical_cast<float>(pieces[6]), boost::lexical_cast<float>(pieces[7]), boost::lexical_cast<float>(pieces[8]));
                colors.back() /= 255.0f; // colors are stored, but not used further on in the program
            }
            if(pieces.size() == 5) {
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[1]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[2]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[3]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[1]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[3]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[4]));
            }
            if(pieces.size() == 4) {
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[1]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[2]));
                indices.push_back(boost::lexical_cast<unsigned int>(pieces[3]));
            }
        }

        file.close();

        auto model = std::make_unique<Model>(positions, normals, indices);
        return model;

    } else {
        throw std::runtime_error("Could not open file: " + path);
    }
}
