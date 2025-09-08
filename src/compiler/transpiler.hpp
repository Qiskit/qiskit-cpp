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

#include "qiskit.h"
#include "circuit/quantumcircuit.hpp"
#include "providers/backend.hpp"

namespace Qiskit {
namespace compiler {

/// @brief Return the transpiled circuit
/// @param circ QuantumCircuit
/// @param backend a backend used for transpiling
/// @param optimization_level level of optimization 0, 1, 2 or 3 (default = 2)
/// @param seed_transpiler The seed for the transpiler (default = -1)
/// @param approximation_degree The approximation degree a heurstic dial (default = 1.0)
/// @return transpiled QuantumCircuit
circuit::QuantumCircuit transpile(circuit::QuantumCircuit& circ, providers::BackendV2& backend, int optimization_level = 2, double approximation_degree = 1.0, int seed_transpiler = -1)
{
  auto target = backend.target();
  if (target == nullptr) {
    return circ;
  }

  QkTranspileOptions options = qk_transpiler_default_options();
  options.optimization_level = (std::uint8_t)optimization_level;
  options.seed = seed_transpiler;
  options.approximation_degree = approximation_degree;

  QkTranspileResult result;
  char* error;

  QkExitCode ret = qk_transpile(circ.get_rust_circuit().get(), target->rust_target(), &options, &result, &error);
  if (ret != QkExitCode_Success) {
    std::cerr << "transpile error (" << ret << ") : "<< error << std::endl;
    target.reset();
    return circ;
  }
  circuit::QuantumCircuit transpiled;
  std::vector<uint32_t> layout_map(qk_transpile_layout_num_output_qubits(result.layout));

  qk_transpile_layout_output_permutation(result.layout, layout_map.data());

  transpiled.from_rust_circuit(std::shared_ptr<rust_circuit>(result.circuit, qk_circuit_free), layout_map);
  transpiled.set_target(target);

  qk_transpile_layout_free(result.layout);
  target.reset();

  return transpiled;
}


} // namespace compiler
} // namespace Qiskit


#endif //__qiskitcpp_compiler_transpiler_def_hpp__
