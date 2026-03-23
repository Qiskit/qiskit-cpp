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

#ifndef __qiskitcpp_transpiler_passmanager_def_hpp__
#define __qiskitcpp_transpiler_passmanager_def_hpp__

#include "utils/types.hpp"
#include "qiskit.h"

#include "circuit/quantumcircuit.hpp"
#include "providers/backend.hpp"
#include "transpiler/target.hpp"

namespace Qiskit
{
namespace transpiler
{

/// @class PassManager
/// @brief base class for PassManager for transpilation
class PassManager {
protected:
    Target target_;
public:
    PassManager() {}

    PassManager(const PassManager& other)
    {
        target_ = other.target_;
    }

    PassManager(const Target& target) : target_(target) {}

    /// @brief virtual function to run transpiler pass
    /// @param input an input quantum circuit
    /// @return a new quantum circuit
    virtual circuit::QuantumCircuit run(circuit::QuantumCircuit& input) = 0;

    /// @brief virtual function to run transpiler pass
    /// @param circuits a list of input quantum circuit
    /// @return a list of output quantum circuits
    virtual std::vector<circuit::QuantumCircuit> run(std::vector<circuit::QuantumCircuit>& circuits)
    {
        std::vector<circuit::QuantumCircuit> output;
        for (auto &circ : circuits) {
            output.push_back(run(circ));
        }
        return output;
    }
};



/// @class StagedPassManager
/// @brief StagedPassManager class
class StagedPassManager : public PassManager
{
protected:
    std::vector<std::string> stages_;
    uint8_t optimization_level_ = 2;
    double approximation_degree_ = 1.0;
    int seed_transpiler_ = -1;
public:
    /// @brief Create a new StagedPassManager
    StagedPassManager() {}

    /// @brief Create a new StagedPassManager
    StagedPassManager(const StagedPassManager& other) : PassManager(other)
    {
        stages_ = other.stages_;
        optimization_level_ = other.optimization_level_;
        approximation_degree_ = other.approximation_degree_;
        seed_transpiler_ = other.seed_transpiler_;
    }

    /// @brief Create a new StagedPassManager
    /// @param stages list of stages used for this pass
    /// @param target target for the backend
    StagedPassManager(std::vector<std::string> stages, Target& target, uint8_t level = 2, double approximation_degree = 1.0, int seed_transpiler = -1)
    {
        target_ = target;
        stages_ = stages;
        optimization_level_ = level;
        approximation_degree_ = approximation_degree;
        seed_transpiler_ = seed_transpiler;
    }

    /// @brief Create a new StagedPassManager
    /// @param stages list of stages used for this pass
    /// @param backend backend used for this pass
    StagedPassManager(std::vector<std::string> stages, providers::BackendV2& backend, uint8_t level = 2, double approximation_degree = 1.0, int seed_transpiler = -1)
    {
        target_ = backend.target();
        stages_ = stages;
        optimization_level_ = level;
        approximation_degree_ = approximation_degree;
        seed_transpiler_ = seed_transpiler;
    }

    ~StagedPassManager()
    {
    }


    circuit::QuantumCircuit run(circuit::QuantumCircuit& circ) override;
};


circuit::QuantumCircuit StagedPassManager::run(circuit::QuantumCircuit& circ)
{
    QkTranspileOptions options = qk_transpiler_default_options();
    options.optimization_level = optimization_level_;
    options.approximation_degree = approximation_degree_;
    if (seed_transpiler_ >= 0) {
        options.seed = seed_transpiler_;
    }
    char *error;
    QkExitCode ret;

    if (stages_.size() == 6) {
        if (stages_[0] == "init" && stages_[1] == "layout" && stages_[2] == "routing" &&
            stages_[3] == "translation" && stages_[4] == "optimization" && stages_[5] == "scheduling") {
            // use default transpiler

            QkTranspileResult result;

            ret = qk_transpile(circ.get_rust_circuit().get(), target_.rust_target(), &options, &result, &error);
            if (ret != QkExitCode_Success) {
                std::cerr << "transpile error (" << ret << ") : " << error << std::endl;
                return circ.copy();
            }
            // save qubit map after transpile
            std::vector<uint32_t> layout_map(qk_transpile_layout_num_output_qubits(result.layout));
            qk_transpile_layout_final_layout(result.layout, false, layout_map.data());

            circuit::QuantumCircuit transpiled = circ;
            transpiled.set_qiskit_circuit(std::shared_ptr<rust_circuit>(result.circuit, qk_circuit_free), layout_map);

            qk_transpile_layout_free(result.layout);
            return transpiled;
        }
    }

    QkDag* dag = qk_circuit_to_dag(circ.get_rust_circuit().get());
    if (dag == nullptr) {
        return circ.copy();
    }
    QkTranspilerStageState* state = nullptr;

    for (auto stage : stages_) {
        if (stage == "init") {
            ret = qk_transpile_stage_init(dag, target_.rust_target(), &options, &state, &error);
            if (ret != QkExitCode_Success) {
                std::cerr << "StagedPassManager Error in init stage (" << ret << ") : " << error << std::endl;
            }
        } else if (stage == "layout") {
            ret = qk_transpile_stage_layout(dag, target_.rust_target(), &options, &state, &error);
            if (ret != QkExitCode_Success) {
                std::cerr << "StagedPassManager Error in layout stage (" << ret << ") : " << error << std::endl;
            }
        } else if (stage == "routing") {
            if (state == nullptr) {
                int nq_dag = qk_dag_num_qubits(dag);
                std::vector<uint32_t> def_map(nq_dag + 1, 0);
                for (int i = 0; i < nq_dag; i++) {
                    def_map[i] = (uint32_t)i;
                }
                QkTranspileLayout* def_layout = qk_transpile_layout_generate_from_mapping(dag, target_.rust_target(), def_map.data());
                qk_transpile_state_new(&state);
                qk_transpile_state_layout_set(state, def_layout);
            }

            ret = qk_transpile_stage_routing(dag, target_.rust_target(), &options, state, &error);
            if (ret != QkExitCode_Success) {
                std::cerr << "StagedPassManager Error in routing stage (" << ret << ") : " << error << std::endl;
            }
        } else if (stage == "translation") {
            ret = qk_transpile_stage_translation(dag, target_.rust_target(), &options, &error);
            if (ret != QkExitCode_Success) {
                std::cerr << "StagedPassManager Error in transplation stage (" << ret << ") : " << error << std::endl;
            }
        } else if (stage == "optimization") {
            if (state == nullptr) {
                int nq_dag = qk_dag_num_qubits(dag);
                std::vector<uint32_t> def_map(nq_dag + 1, 0);
                for (int i = 0; i < nq_dag; i++) {
                    def_map[i] = (uint32_t)i;
                }
                QkTranspileLayout* def_layout = qk_transpile_layout_generate_from_mapping(dag, target_.rust_target(), def_map.data());
                qk_transpile_state_new(&state);
                qk_transpile_state_layout_set(state, def_layout);
            }

            ret = qk_transpile_stage_optimization(dag, target_.rust_target(), &options, &error, state);
            if (ret != QkExitCode_Success) {
                std::cerr << "StagedPassManager Error in optimization stage (" << ret << ") : " << error << std::endl;
            }
        } else if (stage == "scheduling") {
            // to be implemented (?) in C-API
        }
    }
    QkCircuit* result_circ = qk_dag_to_circuit(dag);

    circuit::QuantumCircuit transpiled = circ;
    if (state) {
        QkTranspileLayout* layout = qk_transpile_state_layout(state);
        std::vector<uint32_t> layout_map(qk_transpile_layout_num_output_qubits(layout));
        qk_transpile_layout_final_layout(layout, false, layout_map.data());

        transpiled.set_qiskit_circuit(std::shared_ptr<rust_circuit>(result_circ, qk_circuit_free), layout_map);

        qk_transpile_state_free(state);
    } else {
        std::vector<uint32_t> layout_map;
        transpiled.set_qiskit_circuit(std::shared_ptr<rust_circuit>(result_circ, qk_circuit_free), layout_map);
    }

    return transpiled;
}




} // namespace transpiler
} // namespace Qiskit

#endif //__qiskitcpp_transpiler_passmanager_def_hpp__


