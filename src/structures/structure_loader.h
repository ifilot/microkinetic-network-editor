// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <QDebug>

#include "atom_settings.h"
#include "structure.h"

enum OutcarReadStatus {
    VASP_OUTCAR_READ_STATE_UNDEFINED,
    VASP_OUTCAR_READ_STATE_ELEMENTS,
    VASP_OUTCAR_READ_STATE_IONS_PER_ELEMENT,
    VASP_OUTCAR_READ_STATE_LATTICE_VECTORS,
    VASP_OUTCAR_READ_STATE_ATOMS,
    VASP_OUTCAR_READ_STATE_OPEN,
    VASP_OUTCAR_READ_STATE_FINISHED
};

class StructureLoader {
private:

public:
    /**
     * @brief      Constructs a new instance.
     */
    StructureLoader();

    /**
     * @brief load_file.
     */
    std::vector<std::shared_ptr<Structure>> load_file(const QString& path);

    /**
     * @brief      Load structure from OUTCAR file
     *
     * @param[in]  filename  The filename
     *
     * @return     Structures
     */
    std::vector<std::shared_ptr<Structure>> load_outcar(const std::string& filename);

    /**
     * @brief      Loads a structure from a binary structure pack file
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_structurepack(const std::string& filename);

    /**
     * @brief      Load structure from POSCAR file
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_poscar(const std::string& filename);

    /**
     * @brief      Load structure from ADF logfile
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_adf_logfile(const std::string& filename);

    /**
     * @brief      Load structure from Gaussian logfile
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_gaussian_logfile(const std::string& filename);

    /**
     * @brief      Load structure from ANNP DATA file
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_data(const std::string& filename);

    /**
     * @brief      Load structure from MKS file
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_mks(const std::string& filename);

    /**
     * @brief      Validate whether a YAML file follows the PyMKMKit structure schema
     *
     * @param[in]  filename  The filename
     *
     * @return     True if file contains both 'pymkmkit' and 'structure' root elements
     */
    bool is_pymkmkit_yaml(const std::string& filename);

    /**
     * @brief      Load structure from PyMKMKit YAML file
     *
     * @param[in]  filename  The filename
     *
     * @return     Structure
     */
    std::vector<std::shared_ptr<Structure>> load_pymkmkit_yaml(const std::string& filename);

private:
};
