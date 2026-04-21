/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2017, 2024.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// passmanager class

#ifndef __qiskitcpp_transpiler_generate_preset_pass_manager_def_hpp__
#define __qiskitcpp_transpiler_generate_preset_pass_manager_def_hpp__

#include "transpiler/passmanager.hpp"

namespace Qiskit
{
namespace transpiler
{


const std::vector<std::string> default_stages = {"init", "layout", "routing", "translation", "optimization", "scheduling"};


/// @brief generate presetmanager from backend
/// @param optimization_level an optimization level (0 to 3)
/// @param backend a backend object used for transpilation
/// @param approximation_degree  Heuristic dial used for circuit approximation
///         (1.0=no approximation, 0.0=maximal approximation).
/// @param seed_transpiler Sets random seed for the stochastic parts of the transpiler.
/// @return a new preset pass manager
inline StagedPassManager generate_preset_pass_manager(uint_t optimization_level, providers::BackendV2& backend, double approximation_degree = 1.0, int seed_transpiler = -1)
{
    return StagedPassManager(default_stages, backend, optimization_level, approximation_degree, seed_transpiler);
}


/// @brief generate presetmanager from target
/// @param optimization_level an optimization level (0 to 3)
/// @param target a target object used for transpilation
/// @param approximation_degree  Heuristic dial used for circuit approximation
///         (1.0=no approximation, 0.0=maximal approximation).
/// @param seed_transpiler Sets random seed for the stochastic parts of the transpiler.
/// @return a new preset pass manager
inline StagedPassManager generate_preset_pass_manager(uint_t optimization_level, Target& target, double approximation_degree = 1.0, int seed_transpiler = -1)
{
    return StagedPassManager(default_stages, target, optimization_level, approximation_degree, seed_transpiler);
}

/// @brief generate presetmanager from basis gates and coupling map
/// @param optimization_level an optimization level (0 to 3)
/// @param basis_gates a list of basis gates in string
/// @param coupling_map a list of pairs of connected qubits
/// @param approximation_degree  Heuristic dial used for circuit approximation
///         (1.0=no approximation, 0.0=maximal approximation).
/// @param seed_transpiler Sets random seed for the stochastic parts of the transpiler.
/// @return a new preset pass manager
inline StagedPassManager generate_preset_pass_manager(uint_t optimization_level, const std::vector<std::string>& basis_gates, const std::vector<std::pair<uint32_t, uint32_t>>& coupling_map, double approximation_degree = 1.0, int seed_transpiler = -1)
{
    auto target = Target(basis_gates, coupling_map);
    return StagedPassManager(default_stages, target, optimization_level, approximation_degree, seed_transpiler);
}


/// @brief generate presetmanager from instruction properties
/// @param optimization_level an optimization level (0 to 3)
/// @param props a list of instruction properties for the backend used for transpilation
/// @param approximation_degree  Heuristic dial used for circuit approximation
///         (1.0=no approximation, 0.0=maximal approximation).
/// @param seed_transpiler Sets random seed for the stochastic parts of the transpiler.
/// @return a new preset pass manager
inline StagedPassManager generate_preset_pass_manager(uint_t optimization_level, const std::unordered_map<std::string, std::vector<InstructionProperty>>& props, double approximation_degree = 1.0, int seed_transpiler = -1)
{
    auto target = Target(props);
    return StagedPassManager(default_stages, target, optimization_level, approximation_degree, seed_transpiler);
}

} // namespace transpiler
} // namespace Qiskit

#endif //__qiskitcpp_transpiler_passmanager_def_hpp__

