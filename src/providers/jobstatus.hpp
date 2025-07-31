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

// jobstatus enum values

#ifndef __qiskitcpp_providers_jobstatus_def_hpp__
#define __qiskitcpp_providers_jobstatus_def_hpp__

#include "utils/types.hpp"

namespace Qiskit {
namespace providers {

/// @class JobStatus
/// @brief Class for job status enumerated type.
enum class JobStatus {
    INITIALIZING,
    QUEUED,
    VALIDATING,
    RUNNING,
    CANCELLED,
    DONE,
    ERROR,
    FAILED,
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_jobstatus_def_hpp__


