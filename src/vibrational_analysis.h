#pragma once

#include <QString>
#include <QVector3D>

#include <cstddef>
#include <vector>

struct VibrationModesData {
    std::vector<std::vector<QVector3D>> modes;
    std::vector<double> frequencies_cm_inv;
    std::vector<bool> is_imaginary;
};

VibrationModesData vibration_modes_from_partial_hessian(const QString& structure_path, std::size_t atom_count);
