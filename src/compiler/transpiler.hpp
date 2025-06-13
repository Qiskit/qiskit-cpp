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

// transpiler function

#ifndef __qiskitcpp_compiler_transpiler_def_hpp__
#define __qiskitcpp_compiler_transpiler_def_hpp__

#include "circuit/quantumcircuit.hpp"

#include "qc_transpile.h"


namespace qiskitcpp {
namespace circuit {

/// @brief Return the transpiled circuit
/// @param (circ) QuantumCircuit
/// @param (coupling_map) Coupling map
/// @return transpiled QuantumCircuit
/*
qiskitcpp::circuit::QuantumCircuit transpile(qiskitcpp::circuit::QuantumCircuit& circ, std::vector<std::pair<uint_t, uint_t>> &coupling_map)
{
  // to dag circuit
  auto src_dag = qc_to_dag_circuit(circ.get_rust_circuit());

  // physical layout
  auto dag_out = qc_trivial_layout(src_dag);
  // routing
  if (coupling_map.size() > 0) {
    dag_out = qc_basic_swap(dag_out, (uint_t*)coupling_map.data(), coupling_map.size());
  }
  // basis translation
  dag_out = qc_basis_translation(dag_out);
  // unitary synthesis
  dag_out = qc_unitary_synthesis(dag_out);

  // convert to QuantumCircuit
  auto rust_circ = qc_from_dag_circuit(dag_out);

  // new circuit, attributes are copied from original
  qiskitcpp::circuit::QuantumCircuit trans_circ(circ);
  trans_circ.from_rust_circuit(rust_circ);
  return trans_circ;
}
*/



} // namespace circuit
} // namespace qiskitcpp


#endif //__qiskitcpp_compiler_transpiler_def_hpp__
