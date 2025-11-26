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

#include "utils/types.hpp"
#include "transpiler/target.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "providers/job.hpp"

#include "sqc_api.h"

namespace Qiskit {
namespace providers {

/// @class SQCBackend
/// @brief Backend class using SQC.
class SQCBackend : public BackendV2 {
private:
    sqcInitOptions* init_options_;
    const sqcBackend backend_type_;

public:
    /// @brief Create a new SQCBackend. Internally this initializes SQC.
    SQCBackend()
        : SQCBackend("unspecified") // @TODO
    {}

    /// @brief Create a new SQCBackend object
    /// @param backend_name a resource name for backend.
    SQCBackend(const std::string name)
        : name_(name),
          init_options_(nullptr),
          backend_type_(SQC_RPC_SCHED_QC_TYPE_IBM_DACC)
    {
        init_options_ = sqcMallocInitOptions();
        init_options_->use_qiskit = 1; // only ibm_dacc is supported
        sqcInitialize(init_options_);
    }

    SQCBackend(const SQCBackend& other) = delete;

    ~SQCBackend() {
        sqcFinalize(init_options_);
        sqcFreeInitOptions(init_options_);
    }

    /// @brief Return a target properties for this backend.
    /// @return a target class (nullptr)
    std::shared_ptr<transpiler::Target> target(void) override
    {
        if(target_) {
            return target_;
        }

        // Create a dummy circuit to get target json files
        std::unique_ptr<sqcQC> qc_handle(sqcQuantumCircuit(0), sqcDestroyQuantumCircuit);
        if(sqcIbmdTranspileInfo(qc_handle.get(), backend_type_) != SQC_RESULT_OK) {
            std::cerr << "Failed to get the target information" << std::endl;
            return nullptr;
        }

        nlohmann::ordered_json target_json;
        target_json["configuration"] = nlohmann::ordered_json::parse(qc_handle->backend_config_json)
        target_json["properties"] = nlohmann::ordered_json::parse(qc_handle->backend_props_json);
        auto target = std::shared_ptr<transpiler::Target>();
        if(!target->from_json(target_json)) {
            std::cerr << "Failed to create a target from json files" << std::endl;
            return nullptr;
        }
        target_ = target;

        return target_;
    }

    /// @brief Run and collect samples from each pub.
    /// @return PrimitiveJob
    std::shared_ptr<providers::Job> run(std::vector<primitives::SamplerPub>& input_pubs, uint_t shots) override
    {
        auto circuit = input_pubs[0].circuit();
        auto qasm_str = circuit.to_qasm3();
        std::cout << "run qasm3: \n" << qasm_str << std::endl;

        return nullptr;
    }
};


/// @brief Convert a qiskit quantum circuit to a SQC circuit.
/// @param A SQC circuit where a result is stored
/// @param An original qiskit circuit
/// @note This function destroys the original SQC circuit data.
/// @note Currently parameterized circuits are not supported.
bool qk_circ_to_sqc_circ(sqcQC* qc_handle, const circuit::QuantumCircuit& qk_circ)
{
    if(qc_handle == NULL)
    {
        std::cerr << "Error: Given SQC handle is null." << std::endl;
        return false;
    }
    if(qk_circ.num_instructions() > MAX_N_GATES)
    {
        std::cerr << "Error: The number of a given circuit exceeds the limit of SQC." << std::endl;
        return false;
    }

    qc_handle->qubits = static_cast<int>(qk_circ.num_qubits());
    qc_handle->ngates = static_cast<int>(qk_circ.num_instructions());

    for(int i = 0; i < static_cast<int>(qk_circ.num_instructions()); ++i) {
        const auto circ_instr = qk_circ[i];
        const auto& instr_name = circ_instr.instruction().name();
        const auto& qubits = circ_instr.qubits();
        const auto& clbits = circ_instr.clbits();

        if(instr_name == "measure")
        {
            sqcMeasure(qc_handle, qubits[0], clbits[0], NULL);
        }
        else if(instr_name == "h")
        {
            sqcHGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "cx")
        {
            sqcCXGate(qc_handle, qubits[0], qubits[1]);
        }
        else if(instr_name == "cz")
        {
            sqcCXGate(qc_handle, qubits[0], qubits[1]);
        } 
        else if(instr_name == "rz")
        {
            sqcRZGate(qc_handle, qubits[0], qubits[1]);
        }
        else if(instr_name == "s")
        {
            sqcSGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "sdg")
        {
            sqcSdgGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "rx")
        {
            const auto& instr = circ_instr.instruction();
            assert(instr.params().size() == 1);
            sqcRXGate(qc_handle, instr.params()[0], qubits[0]);
        }
        else if(instr_name == "ry")
        {
            const auto& instr = circ_instr.instruction();
            assert(instr.params().size() == 1);
            sqcRZGate(qc_handle, instr.params()[0], qubits[0]);
        }
        else if(instr_name == "x")
        {
            sqcXGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "z")
        {
            sqcZGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "p")
        {
            sqcU1Gate(qc_handle, qubits[0]);
        }
        else if(instr_name == "reset")
        {
            sqcReset(qc_handle, qubits[0]);
        }
        else if(instr_name == "barrier")
        {
            for(auto qubit : qubits)
            {
                sqcBarrier(qc_handle, qubit);
            }
        }
        else if(instr_name == "ecr")
        {
            sqcECRGate(qc_handle, qubits[0], qubits[1]);
        }
        else if(instr_name == "sx")
        {
            sqcSXGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "id")
        {
            sqcIDGate(qc_handle, qubits[0]);
        }
        else if(instr_name == "delay")
        {
            // TODO
            std::cerr << "Error (WIP): The delay operation is not support now." << std::endl;
            return false;
        }
        else
        {
            std::cerr << "Error: An instruction " << instr_name << " is not supported in SQC." << std::endl;
            return false;
        }
    }

    return true;
}


} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_SQC_backend_def_hpp__


