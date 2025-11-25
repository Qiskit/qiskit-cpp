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

// Qiskit Runtime Service for SQC

#ifndef __qiskitcpp_providers_qiskit_runtime_service_SQC_def_hpp__
#define __qiskitcpp_providers_qiskit_runtime_service_SQC_def_hpp__

#include "providers/sqc_backend.hpp"

namespace Qiskit {
namespace service {

class QiskitRuntimeService {
private:
public:
    /// @brief Create a new runtime service class
    QiskitRuntimeService()
    {
    }

    ~QiskitRuntimeService()
    {
    }

    /// @brief create a new backend object
    /// @return a new QkrtBackend class
    Qiskit::providers::SQCBackend backend()
    {
        return Qiskit::providers::SQCBackend();
    }
};

} // namespace service
} // namespace Qiskit

#endif
