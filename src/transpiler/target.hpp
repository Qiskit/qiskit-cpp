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

// target class

#ifndef __qiskitcpp_transpiler_target_def_hpp__
#define __qiskitcpp_transpiler_target_def_hpp__

#include "utils/types.hpp"
#include "qiskit.h"

#include "circuit/library/standard_gates/standard_gates.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Qiskit
{
namespace transpiler
{

/// @class Target
/// @brief target object to describe backend's properties
class Target
{
protected:
    QkTarget *target_ = nullptr;
    std::string backend_name_;
    std::vector<std::string> basis_gates_;
    double dt_;
    uint_t max_experiments_;
    uint_t max_shots_;
    uint_t num_qubits_ = 0;
    bool is_set_ = false;

public:
    /// @brief Create a new target
    Target() {}

    ~Target()
    {
        if (target_)
        {
            qk_target_free(target_);
        }
    }
    bool is_set(void) const
    {
        return is_set_;
    }
    const QkTarget *rust_target(void) const
    {
        return target_;
    }

    /// @brief name of the target
    /// @return name of target
    const std::string &name(void) const
    {
        return backend_name_;
    }

    /// @brief number of qubits
    /// @return number of qubits
    uint_t num_qubits(void) const
    {
        return num_qubits_;
    }

    /// @brief basis gates for this target
    /// @return a list of basis gates in string
    const std::vector<std::string> &basis_gates(void) const
    {
        return basis_gates_;
    }

    /// @brief make target from json
    /// @param input json target obtained from IQP
    /// @return true if target is successfully made
    bool from_json(nlohmann::ordered_json &input);
};

bool Target::from_json(nlohmann::ordered_json &input)
{
    auto backend_configuration = input["configuration"];
    auto backend_properties = input["properties"];
    if (target_)
    {
        qk_target_free(target_);
    }

    num_qubits_ = backend_configuration["n_qubits"];

    target_ = qk_target_new((uint32_t)num_qubits_);
    if (target_ == nullptr)
        return false;

    dt_ = backend_configuration["dt"];
    max_experiments_ = backend_configuration["max_experiments"];
    max_shots_ = backend_configuration["max_shots"];

    // set target configs available in C-API
    qk_target_set_dt(target_, dt_);
    qk_target_set_granularity(target_, backend_configuration["timing_constraints"]["granularity"]);
    qk_target_set_min_length(target_, backend_configuration["timing_constraints"]["min_length"]);
    qk_target_set_pulse_alignment(target_, backend_configuration["timing_constraints"]["pulse_alignment"]);
    qk_target_set_acquire_alignment(target_, backend_configuration["timing_constraints"]["acquire_alignment"]);

    // get basis gates and make property entries
    auto name_map = Qiskit::circuit::get_standard_gate_name_mapping();
    for (auto &gate : backend_configuration["basis_gates"])
    {
        basis_gates_.push_back(gate);
    }

    // add gate properties
    std::unordered_map<std::string, QkTargetEntry *> property_map;
    for (auto &prop : backend_properties["gates"])
    {
        std::string gate = prop["gate"];
        if (gate == "rzz")
        {
            // TODO: Add RZZ support when we have angle wrapping in
            // Qiskit's target and C transpiler.
            continue;
        }
        std::vector<uint32_t> qubits = prop["qubits"];
        double duration = 0.0;
        double error = 0.0;
        for (auto &param : prop["parameters"])
        {
            if (param["name"] == "gate_error")
            {
                error = param["value"];
            }
            else if (param["name"] == "gate_length")
            {
                duration = 1e-9 * (double)param["value"];
            }
        }

        QkTargetEntry *target_entry = nullptr;
        auto entry = property_map.find(gate);
        if (entry == property_map.end())
        {
            auto inst = name_map.find(gate);
            if (inst == name_map.end())
            {
                if (gate == "reset")
                {
                    target_entry = qk_target_entry_new_reset();
                    property_map[gate] = target_entry;
                }
            }
            else
            {
                target_entry = qk_target_entry_new(inst->second.gate_map());
                property_map[gate] = target_entry;
            }
        }
        else
        {
            target_entry = entry->second;
        }
        if (target_entry)
        {
            QkExitCode ret = qk_target_entry_add_property(target_entry, qubits.data(), (uint32_t)qubits.size(), duration, error);
            if (ret != QkExitCode_Success)
            {
                std::cerr << " target qk_target_entry_add_property error (" << ret << ") : " << gate << " [";
                for (int i = 0; i < qubits.size(); i++)
                {
                    std::cerr << qubits[i] << ", ";
                }
                std::cerr << "]" << std::endl;
            }
        }
    }

    for (auto &entry : property_map)
    {
        QkExitCode ret = qk_target_add_instruction(target_, entry.second);
        if (ret != QkExitCode_Success)
        {
            std::cerr << " target qk_target_add_instruction error (" << ret << ")" << std::endl;
        }
        //    qk_target_entry_free(entry.second);
    }

    // add measure properties
    QkTargetEntry *measure = qk_target_entry_new_measure();
    uint32_t qubit = 0;
    for (uint32_t qubit = 0; qubit < backend_properties["qubits"].size(); qubit++)
    {
        double duration = 0.0;
        double error = 0.0;
        for (auto &param : backend_properties["qubits"][qubit])
        {
            if (param["name"] == "readout_error")
            {
                error = param["value"];
            }
            else if (param["name"] == "readout_length")
            {
                duration = 1e-9 * (double)param["value"];
            }
        }
        qk_target_entry_add_property(measure, &qubit, 1, duration, error);
    }
    qk_target_add_instruction(target_, measure);

    is_set_ = true;
    return true;
}

} // namespace transpiler
} // namespace Qiskit

#endif //__qiskitcpp_transpiler_target_def_hpp__
