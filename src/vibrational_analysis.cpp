#include "vibrational_analysis.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <Eigen/Eigenvalues>
#include <yaml-cpp/yaml.h>

namespace {

double get_atomic_mass_amu(const QString& symbol) {
    static const std::unordered_map<std::string, double> kMassByElement = {
        {"H", 1.00784}, {"He", 4.002602}, {"Li", 6.94}, {"Be", 9.0121831}, {"B", 10.81},
        {"C", 12.011}, {"N", 14.007}, {"O", 15.999}, {"F", 18.998403163}, {"Ne", 20.1797},
        {"Na", 22.98976928}, {"Mg", 24.305}, {"Al", 26.9815385}, {"Si", 28.085}, {"P", 30.973761998},
        {"S", 32.06}, {"Cl", 35.45}, {"Ar", 39.948}, {"K", 39.0983}, {"Ca", 40.078},
        {"Sc", 44.955908}, {"Ti", 47.867}, {"V", 50.9415}, {"Cr", 51.9961}, {"Mn", 54.938044},
        {"Fe", 55.845}, {"Co", 58.933194}, {"Ni", 58.6934}, {"Cu", 63.546}, {"Zn", 65.38},
        {"Ga", 69.723}, {"Ge", 72.630}, {"As", 74.921595}, {"Se", 78.971}, {"Br", 79.904},
        {"Kr", 83.798}, {"Rb", 85.4678}, {"Sr", 87.62}, {"Y", 88.90584}, {"Zr", 91.224},
        {"Nb", 92.90637}, {"Mo", 95.95}, {"Tc", 98.0}, {"Ru", 101.07}, {"Rh", 102.90550},
        {"Pd", 106.42}, {"Ag", 107.8682}, {"Cd", 112.414}, {"In", 114.818}, {"Sn", 118.710},
        {"Sb", 121.760}, {"Te", 127.60}, {"I", 126.90447}, {"Xe", 131.293}, {"Cs", 132.90545196},
        {"Ba", 137.327}, {"La", 138.90547}, {"Ce", 140.116}, {"Pr", 140.90766}, {"Nd", 144.242},
        {"Pm", 145.0}, {"Sm", 150.36}, {"Eu", 151.964}, {"Gd", 157.25}, {"Tb", 158.92535},
        {"Dy", 162.500}, {"Ho", 164.93033}, {"Er", 167.259}, {"Tm", 168.93422}, {"Yb", 173.045},
        {"Lu", 174.9668}, {"Hf", 178.49}, {"Ta", 180.94788}, {"W", 183.84}, {"Re", 186.207},
        {"Os", 190.23}, {"Ir", 192.217}, {"Pt", 195.084}, {"Au", 196.966569}, {"Hg", 200.592},
        {"Tl", 204.38}, {"Pb", 207.2}, {"Bi", 208.98040}, {"Po", 209.0}, {"At", 210.0},
        {"Rn", 222.0}, {"Fr", 223.0}, {"Ra", 226.0}, {"Ac", 227.0}, {"Th", 232.0377},
        {"Pa", 231.03588}, {"U", 238.02891}, {"Np", 237.0}, {"Pu", 244.0}, {"Am", 243.0},
        {"Cm", 247.0}, {"Bk", 247.0}, {"Cf", 251.0}, {"Es", 252.0}, {"Fm", 257.0},
        {"Md", 258.0}, {"No", 259.0}, {"Lr", 266.0}, {"Rf", 267.0}, {"Db", 268.0},
        {"Sg", 269.0}, {"Bh", 270.0}, {"Hs", 277.0}, {"Mt", 278.0}, {"Ds", 281.0},
        {"Rg", 282.0}, {"Cn", 285.0}, {"Nh", 286.0}, {"Fl", 289.0}, {"Mc", 290.0},
        {"Lv", 293.0}, {"Ts", 294.0}, {"Og", 294.0},
    };

    const auto it = kMassByElement.find(symbol.toStdString());
    if (it != kMassByElement.end()) {
        return it->second;
    }

    return 0.0;
}

} // namespace

VibrationModesData vibration_modes_from_partial_hessian(const QString& structure_path, std::size_t atom_count) {
    VibrationModesData result;

    const YAML::Node root = YAML::LoadFile(structure_path.toStdString());
    const YAML::Node partial_hessian = root["vibrations"]["partial_hessian"];
    if (!partial_hessian) {
        return result;
    }

    const YAML::Node dof_labels = partial_hessian["dof_labels"];
    const YAML::Node matrix_node = partial_hessian["matrix"];
    if (!dof_labels || !dof_labels.IsSequence() || !matrix_node || !matrix_node.IsSequence()) {
        return result;
    }

    const int dof_count = static_cast<int>(dof_labels.size());
    if (dof_count == 0 || static_cast<int>(matrix_node.size()) != dof_count) {
        return result;
    }

    std::vector<double> masses_by_atom(atom_count, 0.0);
    const YAML::Node coordinates_direct = root["structure"]["coordinates_direct"];
    if (coordinates_direct && coordinates_direct.IsSequence()) {
        for (std::size_t atom_index = 0; atom_index < atom_count && atom_index < coordinates_direct.size(); ++atom_index) {
            const QString atom_line = QString::fromStdString(coordinates_direct[atom_index].as<std::string>());
            const QStringList tokens = atom_line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (tokens.isEmpty()) {
                continue;
            }

            const double mass = get_atomic_mass_amu(tokens[0]);
            if (mass > 0.0) {
                masses_by_atom[atom_index] = mass;
            }
        }
    }

    std::vector<double> masses_by_dof(dof_count, 0.0);
    for (int dof = 0; dof < dof_count; ++dof) {
        const QString label = QString::fromStdString(dof_labels[dof].as<std::string>());
        if (label.size() < 2) {
            return result;
        }

        bool ok = false;
        const int atom_id = label.left(label.size() - 1).toInt(&ok);
        if (!ok || atom_id <= 0) {
            return result;
        }

        const std::size_t atom_index = static_cast<std::size_t>(atom_id - 1);
        if (atom_index >= masses_by_atom.size() || masses_by_atom[atom_index] <= 0.0) {
            return result;
        }

        masses_by_dof[dof] = masses_by_atom[atom_index];
    }

    Eigen::MatrixXd hessian(dof_count, dof_count);
    for (int i = 0; i < dof_count; ++i) {
        const YAML::Node row = matrix_node[i];
        if (!row.IsSequence() || static_cast<int>(row.size()) != dof_count) {
            return result;
        }
        for (int j = 0; j < dof_count; ++j) {
            hessian(i, j) = row[j].as<double>() / std::sqrt(masses_by_dof[i] * masses_by_dof[j]);
        }
    }

    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(hessian);
    if (solver.info() != Eigen::Success) {
        return result;
    }

    const Eigen::VectorXd eigenvalues = solver.eigenvalues();
    const Eigen::MatrixXd eigenvectors = solver.eigenvectors();

    std::vector<std::vector<QVector3D>> all_modes(
        dof_count, std::vector<QVector3D>(atom_count, QVector3D(0.0f, 0.0f, 0.0f)));
    std::vector<double> all_frequencies_cm_inv(dof_count, 0.0);
    std::vector<bool> all_imaginary(dof_count, false);

    constexpr double kEvAmuA2ToCmInv = 521.47083;

    for (int mode_index = 0; mode_index < dof_count; ++mode_index) {
        for (int dof = 0; dof < dof_count; ++dof) {
            const QString label = QString::fromStdString(dof_labels[dof].as<std::string>());
            if (label.size() < 2) {
                continue;
            }

            const QChar axis = label.back().toUpper();
            bool ok = false;
            const int atom_id = label.left(label.size() - 1).toInt(&ok);
            if (!ok || atom_id <= 0) {
                continue;
            }

            const std::size_t atom_index = static_cast<std::size_t>(atom_id - 1);
            if (atom_index >= all_modes[mode_index].size()) {
                continue;
            }

            QVector3D& atom_displacement = all_modes[mode_index][atom_index];
            const float value = static_cast<float>(eigenvectors(dof, mode_index));
            if (axis == 'X') {
                atom_displacement.setX(value);
            } else if (axis == 'Y') {
                atom_displacement.setY(value);
            } else if (axis == 'Z') {
                atom_displacement.setZ(value);
            }
        }

        float max_norm = 0.0f;
        for (const QVector3D& v : all_modes[mode_index]) {
            max_norm = std::max(max_norm, v.length());
        }
        if (max_norm > 1e-6f) {
            for (QVector3D& v : all_modes[mode_index]) {
                v /= max_norm;
            }
        }

        const double eigenvalue = eigenvalues(mode_index);
        all_imaginary[mode_index] = eigenvalue < 0.0;
        all_frequencies_cm_inv[mode_index] = kEvAmuA2ToCmInv * std::sqrt(std::abs(eigenvalue));
    }

    std::vector<int> order;
    order.reserve(dof_count);

    // rank imaginary first
    for (int i = 0; i < dof_count; ++i) {
        if (all_imaginary[i]) {
            order.push_back(i);
        }
    }
    for (int i = 0; i < dof_count; ++i) {
        if (!all_imaginary[i]) {
            order.push_back(i);
        }
    }

    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (all_imaginary[a] != all_imaginary[b]) {
            return all_imaginary[a] > all_imaginary[b];
        }
        return all_frequencies_cm_inv[a] > all_frequencies_cm_inv[b];
    });

    result.modes.reserve(dof_count);
    result.frequencies_cm_inv.reserve(dof_count);
    result.is_imaginary.reserve(dof_count);
    for (const int index : order) {
        result.modes.push_back(all_modes[index]);
        result.frequencies_cm_inv.push_back(all_frequencies_cm_inv[index]);
        result.is_imaginary.push_back(all_imaginary[index]);
    }

    return result;
}
