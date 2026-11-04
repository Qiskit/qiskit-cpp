/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2024.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// delay operation

#ifndef __qiskitcpp_circuit_delay_def_hpp__
#define __qiskitcpp_circuit_delay_def_hpp__

#include <cmath>
#include <cstdint>
#include <string>

#include "circuit/instruction.hpp"
// Delay stores its duration as a Parameter. instruction.hpp uses Parameter
// without including it, relying on its includer, so include it here explicitly.
#include "circuit/parameter.hpp"

namespace Qiskit {
namespace circuit {

/// @brief The unit of a delay duration.
enum class DelayUnit {
    dt, ///< Integer multiple of the backend's cycle time (the default).
    s,  ///< Seconds.
    ms, ///< Milliseconds.
    us, ///< Microseconds.
    ns, ///< Nanoseconds.
    ps, ///< Picoseconds.
};

/// @brief Return the name of a delay unit, as used by OpenQASM 3 and Qiskit.
/// @param unit the unit
/// @return the unit name, one of "dt", "s", "ms", "us", "ns" or "ps"
inline const char *delay_unit_name(const DelayUnit unit)
{
    switch (unit) {
    case DelayUnit::s:
        return "s";
    case DelayUnit::ms:
        return "ms";
    case DelayUnit::us:
        return "us";
    case DelayUnit::ns:
        return "ns";
    case DelayUnit::ps:
        return "ps";
    case DelayUnit::dt:
    default:
        return "dt";
    }
}

/// @brief Convert a delay unit name to a DelayUnit.
/// @param name the unit name, one of "dt", "s", "ms", "us", "ns" or "ps"
/// @param unit output for the matching unit
/// @return true if the name is a supported unit
inline bool delay_unit_from_name(const std::string &name, DelayUnit &unit)
{
    if (name == "dt") {
        unit = DelayUnit::dt;
    } else if (name == "s") {
        unit = DelayUnit::s;
    } else if (name == "ms") {
        unit = DelayUnit::ms;
    } else if (name == "us") {
        unit = DelayUnit::us;
    } else if (name == "ns") {
        unit = DelayUnit::ns;
    } else if (name == "ps") {
        unit = DelayUnit::ps;
    } else {
        return false;
    }
    return true;
}

/// @brief Map a delay unit to the matching Qiskit C-API unit.
/// @param unit the unit
/// @return the ``QkDelayUnit`` value for the unit
/// @note ``QkDelayUnit_DT`` is rejected by ``qk_circuit_delay``;
///   ``qk_circuit_delay_dt`` must be used for ``dt`` durations instead.
inline QkDelayUnit delay_unit_map(const DelayUnit unit)
{
    switch (unit) {
    case DelayUnit::s:
        return QkDelayUnit_S;
    case DelayUnit::ms:
        return QkDelayUnit_MS;
    case DelayUnit::us:
        return QkDelayUnit_US;
    case DelayUnit::ns:
        return QkDelayUnit_NS;
    case DelayUnit::ps:
        return QkDelayUnit_PS;
    case DelayUnit::dt:
    default:
        return QkDelayUnit_DT;
    }
}

/// @brief Convert a Qiskit C-API duration unit to a DelayUnit.
/// @param unit the C-API unit
/// @return the matching unit; ``DelayUnit::dt`` for units with no equivalent
inline DelayUnit delay_unit_unmap(const QkDelayUnit unit)
{
    switch (unit) {
    case QkDelayUnit_S:
        return DelayUnit::s;
    case QkDelayUnit_MS:
        return DelayUnit::ms;
    case QkDelayUnit_US:
        return DelayUnit::us;
    case QkDelayUnit_NS:
        return DelayUnit::ns;
    case QkDelayUnit_PS:
        return DelayUnit::ps;
    case QkDelayUnit_DT:
    default:
        return DelayUnit::dt;
    }
}

/// @brief Read the duration of a delay instruction.
/// @param param the instruction's duration parameter
/// @param unit the instruction's duration unit
/// @return the duration, as a double
/// @details A ``dt`` duration is stored as an integer parameter, for which
///   ``qk_param_as_real`` performs a lossy conversion, so it is read through
///   ``qk_param_as_int`` instead. A duration left unbound has no numeric value
///   and reads as zero.
inline double delay_duration_value(const QkParam *param, const QkDelayUnit unit)
{
    if (param == nullptr) {
        return 0.0;
    }
    if (unit == QkDelayUnit_DT) {
        std::int64_t duration = 0;
        if (qk_param_as_int(param, &duration)) {
            return (double)duration;
        }
    }
    const double duration = qk_param_as_real(param);
    return std::isnan(duration) ? 0.0 : duration;
}

/// @class Delay
/// @brief Do nothing and just delay/wait/idle for a specified duration.
/// @note The duration and unit are stored on ``Instruction`` so that they
///   survive being copied into a ``CircuitInstruction``, which holds an
///   ``Instruction`` by value.
class Delay : public Instruction {
protected:
public:
    /// @brief Create a new Delay
    /// @param duration the duration of the delay
    /// @param unit the unit of the duration, "dt" by default
    Delay(const double duration = 0.0, const DelayUnit unit = DelayUnit::dt)
    {
        name_ = "delay";
        num_qubits_ = 1;
        num_clbits_ = 0;
        delay_duration_ = duration;
        delay_unit_ = delay_unit_map(unit);
        params_.push_back(Parameter(duration));
    }

    /// @brief Return the duration of this delay.
    /// @return the duration, in the unit returned by ``unit()``
    double duration(void) const
    {
        return delay_duration_;
    }

    /// @brief Return the unit of this delay's duration.
    /// @return the unit
    DelayUnit unit(void) const
    {
        return delay_unit_unmap(delay_unit_);
    }

    /// @brief Return the name of the unit of this delay's duration.
    /// @return the unit name, one of "dt", "s", "ms", "us", "ns" or "ps"
    const char *unit_name(void) const
    {
        return delay_unit_name(unit());
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_delay_def_hpp__
