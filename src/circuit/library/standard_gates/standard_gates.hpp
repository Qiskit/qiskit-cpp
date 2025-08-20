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

// standard gates definitions

#ifndef __qiskitcpp_circuit_library_standard_gates_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_def_hpp__

#include "circuit/library/standard_gates/dcx.hpp"
#include "circuit/library/standard_gates/ecr.hpp"
#include "circuit/library/standard_gates/global_phase.hpp"
#include "circuit/library/standard_gates/h.hpp"
#include "circuit/library/standard_gates/i.hpp"
#include "circuit/library/standard_gates/iswap.hpp"
#include "circuit/library/standard_gates/p.hpp"
#include "circuit/library/standard_gates/r.hpp"
#include "circuit/library/standard_gates/rx.hpp"
#include "circuit/library/standard_gates/rxx.hpp"
#include "circuit/library/standard_gates/ry.hpp"
#include "circuit/library/standard_gates/ryy.hpp"
#include "circuit/library/standard_gates/rz.hpp"
#include "circuit/library/standard_gates/rzx.hpp"
#include "circuit/library/standard_gates/rzz.hpp"
#include "circuit/library/standard_gates/s.hpp"
#include "circuit/library/standard_gates/swap.hpp"
#include "circuit/library/standard_gates/sx.hpp"
#include "circuit/library/standard_gates/t.hpp"
#include "circuit/library/standard_gates/u.hpp"
#include "circuit/library/standard_gates/u1.hpp"
#include "circuit/library/standard_gates/u2.hpp"
#include "circuit/library/standard_gates/u3.hpp"
#include "circuit/library/standard_gates/x.hpp"
#include "circuit/library/standard_gates/xx_minus_yy.hpp"
#include "circuit/library/standard_gates/xx_plus_yy.hpp"
#include "circuit/library/standard_gates/y.hpp"
#include "circuit/library/standard_gates/z.hpp"

namespace Qiskit {
namespace circuit {

std::unordered_map<std::string, Instruction> get_standard_gate_name_mapping(void)
{
    // mapping of gate string and QkGate
    std::unordered_map<std::string, Instruction> name_mapping;
    name_mapping["global_phase"] = GlobalPhaseGate();
    name_mapping["h"] = HGate();
    name_mapping["i"] = IGate();
    name_mapping["x"] = XGate();
    name_mapping["y"] = YGate();
    name_mapping["z"] = ZGate();
    name_mapping["p"] = PhaseGate();
    name_mapping["r"] = RGate();
    name_mapping["rx"] = RXGate();
    name_mapping["ry"] = RYGate();
    name_mapping["rz"] = RZGate();
    name_mapping["s"] = SGate();
    name_mapping["sdg"] = SdgGate();
    name_mapping["sx"] = SXGate();
    name_mapping["sxdg"] = SXdgGate();
    name_mapping["t"] = TGate();
    name_mapping["tdg"] = TdgGate();
    name_mapping["u"] = UGate();
    name_mapping["u1"] = U1Gate();
    name_mapping["u2"] = U2Gate();
    name_mapping["u3"] = U3Gate();
    name_mapping["ch"] = CHGate();
    name_mapping["cx"] = CXGate();
    name_mapping["cy"] = CYGate();
    name_mapping["cz"] = CZGate();
    name_mapping["dcx"] = DCXGate();
    name_mapping["ecr"] = ECRGate();
    name_mapping["swap"] = SwapGate();
    name_mapping["iswap"] = ISwapGate();
    name_mapping["cp"] = CPhaseGate();
    name_mapping["crx"] = CRXGate();
    name_mapping["cry"] = CRYGate();
    name_mapping["crz"] = CRZGate();
    name_mapping["cs"] = CSGate();
    name_mapping["csdg"] = CSdgGate();
    name_mapping["csx"] = CSXGate();
    name_mapping["cu"] = CUGate();
    name_mapping["cu1"] = CU1Gate();
    name_mapping["cu3"] = CU3Gate();
    name_mapping["rxx"] = RXXGate();
    name_mapping["ryy"] = RYYGate();
    name_mapping["rzz"] = RZZGate();
    name_mapping["rzx"] = RZXGate();
    name_mapping["xx_minus_yy"] = XXMinusYYGate();
    name_mapping["xx_plus_yy"] = XXPlusYYGate();
    name_mapping["ccx"] = CCXGate();
    name_mapping["ccz"] = CCZGate();
    name_mapping["cswap"] = CSwapGate();
    name_mapping["rccx"] = RCCXGate();
    name_mapping["mcx"] = MCXGate();
    name_mapping["c3sx"] = C3XGate();
    name_mapping["rcccx"] = RC3XGate();
    return name_mapping;
}



} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_def_hpp__
