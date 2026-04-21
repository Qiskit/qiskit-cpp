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

/// @struct InstructionProperty
/// @brief properties for an instruction
struct InstructionProperty
{
    std::vector<uint32_t> qargs;
    double duration;
    double error;
};



/// @class Target
/// @brief target object to describe backend's properties
class Target
{
protected:
    std::shared_ptr<QkTarget> target_ = nullptr;
    std::string backend_name_;
    std::vector<std::string> basis_gates_;
    double dt_;
    uint_t max_experiments_;
    uint_t max_shots_;
    uint_t num_qubits_ = 0;
    bool is_set_ = false;
    std::vector<std::pair<uint32_t, uint32_t>> coupling_map_;
    std::unordered_map<std::string, std::vector<InstructionProperty>> properties_;
public:
    /// @brief Create a new target
    Target() {}

    /// @brief Create a new target
    Target(QkTarget* target)
    {
        target_ = std::shared_ptr<QkTarget>(target, qk_target_free);
        num_qubits_ = qk_target_num_qubits(target);
        dt_ = qk_target_dt(target);
        is_set_ = true;
    }

    /// @brief Create a new target
    Target(const Target& other)
    {
        target_ = other.target_;
        backend_name_ = other.backend_name_;
        basis_gates_ = other.basis_gates_;
        dt_ = other.dt_;
        max_experiments_ = other.max_experiments_;
        max_shots_ = other.max_shots_;
        num_qubits_ = other.num_qubits_;
        is_set_ = other.is_set_;
    }

    Target(const std::unordered_map<std::string, std::vector<InstructionProperty>>& props)
    {
        properties_ = props;
    }

    Target(const std::vector<std::string>& basis_gates, const std::vector<std::pair<uint32_t, uint32_t>>& coupling_map, double default_duration = 0.0, double default_error = 0.0)
    {
        basis_gates_ = basis_gates;
        coupling_map_ = coupling_map;

        // get num qubits
        num_qubits_ = 0;
        for (auto &qubits : coupling_map) {
            if (qubits.first > num_qubits_) {
                num_qubits_ = qubits.first;
            }
            if (qubits.second > num_qubits_) {
                num_qubits_ = qubits.second;
            }
        }
        num_qubits_ += 1;

        auto name_map = Qiskit::circuit::get_standard_gate_name_mapping();
        for (auto &gate_name : basis_gates) {
            std::vector<InstructionProperty> props;
            auto gate = name_map.find(gate_name);
            if (gate != name_map.end()) {
                if (gate->second.num_qubits() == 1) {
                    for (uint_t i = 0; i < num_qubits_; i++) {
                        InstructionProperty p;
                        p.qargs = {(uint32_t)i};
                        p.duration = default_duration;
                        p.error = default_error;
                        props.push_back(p);
                    }
                } else if (gate->second.num_qubits() == 2) {
                    for (auto &qargs : coupling_map) {
                        InstructionProperty p;
                        p.qargs = {qargs.first, qargs.second};
                        p.duration = default_duration;
                        p.error = default_error;
                        props.push_back(p);
                    }
                }
            }
            properties_[gate_name] = props;
        }
    }

    ~Target()
    {
        if (target_) {
            target_.reset();
        }
    }
    bool is_set(void) const
    {
        return is_set_;
    }
    const QkTarget *rust_target(void)
    {
        if (!is_set_) {
            build_target();
        }
        if (target_) {
            return target_.get();
        }
        return nullptr;
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
    bool from_json(nlohmann::ordered_json &input)
    {
        if (!input.contains("configuration")) {
            std::cerr << " Target Error : No configuration section found" << std::endl;
            return false;
        }
        auto backend_configuration = input["configuration"];
        if (!input.contains("properties")) {
            std::cerr << " Target Error : No properties section found" << std::endl;
            return false;
        }
        auto backend_properties = input["properties"];

        if (!backend_configuration.contains("n_qubits")) {
            std::cerr << " Target Error : n_qubits not found in backend config" << std::endl;
            return false;
        }
        num_qubits_ = backend_configuration["n_qubits"];

        if (target_) {
            target_.reset();
        }

        QkTarget* t = qk_target_new((uint32_t)num_qubits_);
        if (t == nullptr)
            return false;
        target_ = std::shared_ptr<QkTarget>(t, qk_target_free);

        max_experiments_ = backend_configuration["max_experiments"];
        max_shots_ = backend_configuration["max_shots"];

        // set target configs available in C-API
        if (backend_configuration.at("dt").is_number()) {
            dt_ = backend_configuration["dt"];
            qk_target_set_dt(target_.get(), dt_);
        }
        if (backend_configuration.contains("timing_constraints")) {
            auto timing_constraints = backend_configuration["timing_constraints"];
            qk_target_set_granularity(target_.get(), timing_constraints["granularity"]);
            qk_target_set_min_length(target_.get(), timing_constraints["min_length"]);
            qk_target_set_pulse_alignment(target_.get(), timing_constraints["pulse_alignment"]);
            qk_target_set_acquire_alignment(target_.get(), timing_constraints["acquire_alignment"]);
        }

        // get basis gates and make property entries
        auto name_map = Qiskit::circuit::get_standard_gate_name_mapping();
        for (auto &gate : backend_configuration["basis_gates"]) {
            basis_gates_.push_back(gate);
        }

        // add gate properties
        std::unordered_map<std::string, QkTargetEntry *> property_map;
        for (auto &prop : backend_properties["gates"]) {
            std::string gate = prop["gate"];
            if (gate == "rzz") {
                // TODO: Add RZZ support when we have angle wrapping in
                // Qiskit's target and C transpiler.
                continue;
            }
            std::vector<uint32_t> qubits = prop["qubits"];
            double duration = 0.0;
            double error = 0.0;
            for (auto &param : prop["parameters"]) {
                if (param["name"] == "gate_error") {
                    error = param["value"];
                } else if (param["name"] == "gate_length") {
                    duration = 1e-9 * (double)param["value"];
                }
            }

            QkTargetEntry *target_entry = nullptr;
            auto entry = property_map.find(gate);
            if (entry == property_map.end()) {
                auto inst = name_map.find(gate);
                if (inst == name_map.end()) {
                    if (gate == "reset") {
                        target_entry = qk_target_entry_new_reset();
                        property_map[gate] = target_entry;
                    }
                } else {
                    target_entry = qk_target_entry_new(inst->second.gate_map());
                    property_map[gate] = target_entry;
                }
            } else {
                target_entry = entry->second;
            }
            if (target_entry) {
                QkExitCode ret = qk_target_entry_add_property(target_entry, qubits.data(), (uint32_t)qubits.size(), duration, error);
                if (ret != QkExitCode_Success) {
                    std::cerr << " target qk_target_entry_add_property error (" << ret << ") : " << gate << " [";
                    for (int i = 0; i < qubits.size(); i++) {
                        std::cerr << qubits[i] << ", ";
                    }
                    std::cerr << "]" << std::endl;
                }
            }
        }

        for (auto &entry : property_map) {
            QkExitCode ret = qk_target_add_instruction(target_.get(), entry.second);
            if (ret != QkExitCode_Success) {
                std::cerr << " target qk_target_add_instruction error (" << ret << ")" << std::endl;
            }
            //    qk_target_entry_free(entry.second);
        }

        // add measure properties
        QkTargetEntry *measure = qk_target_entry_new_measure();
        uint32_t qubit = 0;
        for (uint32_t qubit = 0; qubit < backend_properties["qubits"].size(); qubit++) {
            double duration = 0.0;
            double error = 0.0;
            for (auto &param : backend_properties["qubits"][qubit]) {
                if (param["name"] == "readout_error") {
                    error = param["value"];
                } else if (param["name"] == "readout_length") {
                    duration = 1e-9 * (double)param["value"];
                }
            }
            qk_target_entry_add_property(measure, &qubit, 1, duration, error);
        }
        qk_target_add_instruction(target_.get(), measure);

        is_set_ = true;
        return true;
    }

    /// @brief add instruction to the target
    /// @param instruction reference to the instruction to be added
    /// @param properties properties of the instruction
    void add_instruction(const circuit::Instruction& instruction, const std::vector<InstructionProperty>& properties)
    {
        auto prop = properties_.find(instruction.name());
        if (prop == properties_.end()) {
            properties_[instruction.name()] = properties;
        } else {
            prop->second.insert(prop->second.end(), properties.begin(), properties.end());
        }
    }

protected:
    void build_target(void)
    {
        if (target_) {
            target_.reset();
        }

        // get num qubits
        if (num_qubits_ == 0) {
            for (auto &prop : properties_) {
                for (auto &inst : prop.second) {
                    for (auto &qubit : inst.qargs) {
                        if (qubit > num_qubits_) {
                            num_qubits_ = qubit;
                        }
                    }
                }
            }
        }
        num_qubits_ += 1;

        QkTarget* t = qk_target_new((uint32_t)num_qubits_);
        if (t == nullptr)
            return;
        target_ = std::shared_ptr<QkTarget>(t, qk_target_free);

        // add properties
        auto name_map = Qiskit::circuit::get_standard_gate_name_mapping();
        for (auto &prop : properties_) {
            QkTargetEntry *target_entry = nullptr;
            if (prop.first == "reset") {
                target_entry = qk_target_entry_new_reset();
            } else if (prop.first == "rzz") {
                std::cerr << " Target: rzz gate is not supported until Qiskit C-API will support it." << std::endl;
            } else {
                auto gate = name_map.find(prop.first);
                if (gate != name_map.end()) {
                    target_entry = qk_target_entry_new(gate->second.gate_map());
                }
            }

            if (target_entry != nullptr) {
                for (auto &inst : prop.second) {
                    QkExitCode ret = qk_target_entry_add_property(target_entry, inst.qargs.data(), (uint32_t)inst.qargs.size(), inst.duration, inst.error);
                    if (ret != QkExitCode_Success) {
                        std::cerr << " Target : qk_target_entry_add_property error (" << ret << ") : " << prop.first << " [";
                        for (int i = 0; i < inst.qargs.size(); i++) {
                            std::cerr << inst.qargs[i] << ", ";
                        }
                        std::cerr << "]" << std::endl;
                    }
                }
                QkExitCode ret = qk_target_add_instruction(target_.get(), target_entry);
                if (ret != QkExitCode_Success) {
                    std::cerr << " Target : qk_target_add_instruction error (" << ret << ")" << std::endl;
                }
            }
        }

        // add measure properties
        QkTargetEntry *measure = qk_target_entry_new_measure();
        uint32_t qubit = 0;
        for (uint32_t qubit = 0; qubit < num_qubits_; qubit++) {
            double duration = 0.0;
            double error = 0.0;
            qk_target_entry_add_property(measure, &qubit, 1, duration, error);
        }
        qk_target_add_instruction(target_.get(), measure);

        is_set_ = true;
    }
};


} // namespace transpiler
} // namespace Qiskit

#endif //__qiskitcpp_transpiler_target_def_hpp__
