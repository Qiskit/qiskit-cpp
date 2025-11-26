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
        : SQCBackend("unspecified")
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
        std::cerr << "Error: SQCBackend::target() cannot be used" << std::endl;
        return nullptr;
    }

    /// @brief Run and collect samples from each pub.
    /// @return PrimitiveJob
    std::shared_ptr<providers::Job> run(std::vector<primitives::SamplerPub>& input_pubs, uint_t shots) override
    {
        auto circuit = input_pubs[0].circuit();
        std::cout << "run qasm3: \n" << circuit.to_qasm3() << std::endl;
        return nullptr;
    }
};


} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_SQC_backend_def_hpp__


