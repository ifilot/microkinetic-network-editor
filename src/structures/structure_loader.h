// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>

#pragma once

#include <memory>
#include <vector>

#include <QString>

#include "structure.h"

class StructureLoader {
public:
    StructureLoader();

    std::vector<std::shared_ptr<Structure>> load_file(const QString& path);

private:
    bool is_pymkmkit_yaml(const std::string& filename);
    std::vector<std::shared_ptr<Structure>> load_pymkmkit_yaml(const std::string& filename);
};
