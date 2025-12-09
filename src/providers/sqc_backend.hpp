/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2025.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// SQC Backend

#ifndef __qiskitcpp_providers_SQC_backend_def_hpp__
#define __qiskitcpp_providers_SQC_backend_def_hpp__

#include <memory>

#include "utils/types.hpp"
#include "transpiler/target.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "providers/sqc_job.hpp"

#include "sqc_ecode.h"
#include "sqc_api.h"

namespace Qiskit {
namespace providers {

/// @brief Convert a qiskit quantum circuit to a SQC circuit.
/// @param An original qiskit circuit
/// @return a SQC circuit equivalent to the given qiskit circuit
/// @note Currently parameterized circuits are not supported.
std::shared_ptr<sqcQC> qk_circ_to_sqc_circ(circuit::QuantumCircuit& qk_circ);

/// @class SQCBackend
/// @brief Backend class using SQC.
class SQCBackend : public BackendV2 {
private:
    const sqcBackend backend_type_;
    std::shared_ptr<transpiler::Target> target_;

public:
    /// @brief Create a new SQCBackend. Internally this initializes SQC.
    SQCBackend()
        : SQCBackend("unspecified")
    {}

    /// @brief Create a new SQCBackend object
    /// @param backend_name a resource name for backend.
    SQCBackend(const std::string name)
        : BackendV2(name),
          backend_type_(SQC_RPC_SCHED_QC_TYPE_IBM_DACC),
          target_(nullptr)
    {}

    SQCBackend(const SQCBackend& other)
        : BackendV2(other.name_),
          backend_type_(other.backend_type_),
          target_(other.target_)
    {}

    ~SQCBackend() {}

    /// @brief Return a target properties for this backend.
    /// @return a target class (nullptr)
    std::shared_ptr<transpiler::Target> target(void) override
    {
        if(target_) return target_;

        // Create a dummy circuit to get target json files
        std::unique_ptr<sqcQC, decltype(&sqcDestroyQuantumCircuit)> qc_handle(sqcQuantumCircuit(0), &sqcDestroyQuantumCircuit);
        if(sqcIbmdTranspileInfo(qc_handle.get(), backend_type_) != SQC_RESULT_OK) {
            std::cerr << "Failed to get the target information" << std::endl;
            return nullptr;
        }

        nlohmann::ordered_json target_json;
        target_json["configuration"] = nlohmann::ordered_json::parse(qc_handle->backend_config_json);
        target_json["properties"] = nlohmann::ordered_json::parse(qc_handle->backend_props_json);
        auto target = std::make_shared<transpiler::Target>();
        if(!target->from_json(target_json)) {
            std::cerr << "Failed to create a target from json files" << std::endl;
            return nullptr;
        }
        target_ = target;

        return target_;
    }

    /// @brief Run and collect samples from each pub.
    /// @return SQCJob
    std::shared_ptr<providers::Job> run(std::vector<primitives::SamplerPub>& input_pubs, uint_t shots) override
    {
        auto circuit = input_pubs[0].circuit();
        const auto qasm3_str = circuit.to_qasm3();
        std::cout << "run qasm3: \n" << qasm3_str << std::endl;

        // Create a sqcQC from a qiskit circuit
        const uint_t qasm_len = qasm3_str.size() + 500;
        auto sqc_circ = qk_circ_to_sqc_circ(circuit);
        if(!sqc_circ)
        {
            std::cerr << "Error: Failed to convert a given qiskit circuit to a SQC circuit." << std::endl;
            return nullptr;
        }
        sqc_circ->qasm = (char*)malloc(qasm_len);
        sqcConvQASMtoMemory(sqc_circ.get(), backend_type_, sqc_circ->qasm, qasm_len);

        std::unique_ptr<sqcRunOptions> run_options(new sqcRunOptions);
        sqcInitializeRunOpt(run_options.get());
        run_options->nshots = shots;
        run_options->qubits = sqc_circ->qubits;
        run_options->outFormat = SQC_OUT_RAW; // @TODO

        std::shared_ptr<sqcOut> result(new sqcOut, [](sqcOut* out) { sqcFreeOut(out, SQC_OUT_RAW); });
        int error_code = sqcQCRun(sqc_circ.get(), backend_type_, *run_options, result.get());

        if(error_code != SQC_RESULT_OK)
        {
            std::cerr << "Error: Failed to run a SQC circuit." << std::endl;
            return nullptr;
        }

        auto results_json = nlohmann::ordered_json::parse(result->result);

        return std::make_shared<SQCJob>(results_json);
    }
};


std::shared_ptr<sqcQC> qk_circ_to_sqc_circ(circuit::QuantumCircuit& qk_circ)
{
    if(qk_circ.num_instructions() > MAX_N_GATES)
    {
        std::cerr << "Error: The number of a given circuit exceeds the limit of SQC." << std::endl;
        return nullptr;
    }

    auto sqc_circ = std::shared_ptr<sqcQC>(sqcQuantumCircuit(circuit.num_qubits()), sqcDestroyQuantumCircuit);
    for(int i = 0; i < static_cast<int>(qk_circ.num_instructions()); ++i) {
        const auto circ_instr = qk_circ[i];
        const auto& instr_name = circ_instr.instruction().name();
        const auto& qubits = circ_instr.qubits();
        const auto& clbits = circ_instr.clbits();

        if(instr_name == "measure")
        {
            sqcMeasure(sqc_circ, qubits[0], clbits[0], NULL);
        }
        else if(instr_name == "h")
        {
            sqcHGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "cx")
        {
            sqcCXGate(sqc_circ, qubits[0], qubits[1]);
        }
        else if(instr_name == "cz")
        {
            sqcCZGate(sqc_circ, qubits[0], qubits[1]);
        } 
        else if(instr_name == "rz")
        {
            sqcRZGate(sqc_circ, qubits[0], qubits[1]);
        }
        else if(instr_name == "s")
        {
            sqcSGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "sdg")
        {
            sqcSdgGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "rx")
        {
            const auto& instr = circ_instr.instruction();
            assert(instr.params().size() == 1);
            sqcRXGate(sqc_circ, instr.params()[0], qubits[0]);
        }
        else if(instr_name == "ry")
        {
            const auto& instr = circ_instr.instruction();
            assert(instr.params().size() == 1);
            sqcRYGate(sqc_circ, instr.params()[0], qubits[0]);
        }
        else if(instr_name == "x")
        {
            sqcXGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "z")
        {
            sqcZGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "p")
        {
            const auto& instr = circ_instr.instruction();
            sqcU1Gate(sqc_circ, instr.params()[0], qubits[0]);
        }
        else if(instr_name == "reset")
        {
            sqcReset(sqc_circ, qubits[0]);
        }
        else if(instr_name == "barrier")
        {
            for(auto qubit : qubits)
            {
                sqcBarrier(sqc_circ, qubit);
            }
        }
        else if(instr_name == "ecr")
        {
            sqcECRGate(sqc_circ, qubits[0], qubits[1]);
        }
        else if(instr_name == "sx")
        {
            sqcSXGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "id")
        {
            sqcIDGate(sqc_circ, qubits[0]);
        }
        else if(instr_name == "delay")
        {
            // TODO
            std::cerr << "Error (WIP): The delay operation is not support now." << std::endl;
            return nullptr;
        }
        else
        {
            std::cerr << "Error: An instruction " << instr_name << " is not supported in SQC." << std::endl;
            return nullptr;
        }
    }

    return sqc_circ;
}


} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_SQC_backend_def_hpp__


