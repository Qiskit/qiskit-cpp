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

// Shared helpers for the QRMI C API
//
// Requires QRMI 0.13.0 or later.

#ifndef __qiskitcpp_providers_qrmi_common_hpp__
#define __qiskitcpp_providers_qrmi_common_hpp__

#include <sstream>
#include <string>

#include "qrmi.h"

namespace Qiskit {
namespace providers {

/// @brief RAII guard for a string returned by the QRMI C API.
/// @details QRMI hands ownership of these strings to the caller and they must be
///   released with ``qrmi_string_free``. Wrapping them means the free cannot be
///   missed on an early return or a throw.
class ScopedQrmiString {
    char *value_;

public:
    ScopedQrmiString(void) : value_(nullptr) {}
    explicit ScopedQrmiString(char *value) : value_(value) {}
    ~ScopedQrmiString()
    {
        if (value_ != nullptr) {
            qrmi_string_free(value_);
        }
    }
    ScopedQrmiString(const ScopedQrmiString &) = delete;
    ScopedQrmiString &operator=(const ScopedQrmiString &) = delete;

    /// @brief Output location for a QRMI call that returns a string.
    char **out(void) { return &value_; }

    /// @brief Whether QRMI stored a string here.
    bool valid(void) const { return value_ != nullptr; }

    /// @brief The string, or an empty string if QRMI stored nothing.
    std::string str(void) const
    {
        return value_ != nullptr ? std::string(value_) : std::string();
    }
};

/// @brief Return the message text of the most recent QRMI error.
/// @return the message, or an empty string if QRMI did not record one
/// @details The string is copied out and not freed. As of QRMI 0.13.4
///   ``qrmi_get_last_error`` returns a borrowed pointer into thread-local
///   storage (``LAST_ERROR.with(|cell| cstr.as_ptr())`` in ``src/cext.rs``), so
///   passing it to ``qrmi_string_free`` would free a ``CString`` that QRMI still
///   owns. Note that QRMI's documentation and its own C examples both describe
///   this string as caller-owned and free it; the implementation does not match,
///   so this follows the implementation.
/// @note Because QRMI does not clear the error state on read, the message may be
///   left over from an earlier failure. It is only used here to annotate a call
///   that has already reported a failing return code.
inline std::string qrmi_last_error(void)
{
    const char *message = qrmi_get_last_error();
    if (message == nullptr) {
        return std::string();
    }
    return std::string(message);
}

/// @brief Format a QRMI failure, including the cause QRMI reported.
/// @param context what was being attempted
/// @param rc the return code QRMI reported
/// @return a message of the form ``QRMI Error : <context> (<rc>) : <detail>``,
///   with the detail omitted when QRMI recorded no error
inline std::string qrmi_error_message(const std::string &context, const QrmiReturnCode rc)
{
    std::stringstream message;
    message << "QRMI Error : " << context << " (" << rc << ")";
    const std::string detail = qrmi_last_error();
    if (!detail.empty()) {
        message << " : " << detail;
    }
    return message.str();
}

/// @brief Format a QRMI failure that reports no return code.
/// @param context what was being attempted
/// @return a message of the form ``QRMI Error : <context> : <detail>``, with the
///   detail omitted when QRMI recorded no error
inline std::string qrmi_error_message(const std::string &context)
{
    std::stringstream message;
    message << "QRMI Error : " << context;
    const std::string detail = qrmi_last_error();
    if (!detail.empty()) {
        message << " : " << detail;
    }
    return message.str();
}

} // namespace providers
} // namespace Qiskit

#endif //__qiskitcpp_providers_qrmi_common_hpp__
