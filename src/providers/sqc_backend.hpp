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

        std::shared_ptr<sqcQC> sqc_circ(sqcQuantumCircuit(circuit.num_qubits()), sqcDestroyQuantumCircuit);
        sqc_circ->qasm = strdup(qasm3_str.c_str());

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


} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_SQC_backend_def_hpp__
