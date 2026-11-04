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

// QPY importer / exporter

#ifndef __qiskitcpp_circuit_qpy_hpp__
#define __qiskitcpp_circuit_qpy_hpp__

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>

#include "circuit/quantumcircuit_def.hpp"
#include "utils/capi_features.hpp"

#if QISKIT_CPP_HAS_QPY

namespace Qiskit {
namespace circuit {

class QpyImporter {
protected:
  // RAII guards for Qiskit C-API handles so cleanup is exception-safe and the
  // manual qk_*_free calls cannot be missed on an early return/throw.
  class ScopedQpyError {
    char *message_;

  public:
    ScopedQpyError(void) : message_(nullptr) {}
    ~ScopedQpyError() {
      if (message_ != nullptr) {
        qk_str_free(message_);
      }
    }
    ScopedQpyError(const ScopedQpyError &) = delete;
    ScopedQpyError &operator=(const ScopedQpyError &) = delete;
    char **out(void) { return &message_; }
    std::string str(void) const {
      return message_ != nullptr ? std::string(message_) : std::string();
    }
  };

  class ScopedLoadedCircuits {
    QkQpyLoadedCircuits circuits_;

  public:
    // The C-API only writes the members on success, so they are initialized
    // here to the state qk_qpy_loaded_circuits_clear treats as empty.
    ScopedLoadedCircuits(void) {
      circuits_.data = nullptr;
      circuits_.len = 0;
    }
    ~ScopedLoadedCircuits() { qk_qpy_loaded_circuits_clear(&circuits_); }
    ScopedLoadedCircuits(const ScopedLoadedCircuits &) = delete;
    ScopedLoadedCircuits &operator=(const ScopedLoadedCircuits &) = delete;
    QkQpyLoadedCircuits *out(void) { return &circuits_; }
    std::size_t size(void) const { return circuits_.len; }
    const QkCircuit *at(const std::size_t index) const {
      return circuits_.data[index];
    }
  };

  class ScopedStr {
    char *value_;

  public:
    explicit ScopedStr(char *value) : value_(value) {}
    ~ScopedStr() {
      if (value_ != nullptr) {
        qk_str_free(value_);
      }
    }
    ScopedStr(const ScopedStr &) = delete;
    ScopedStr &operator=(const ScopedStr &) = delete;
    std::string str(void) const {
      return value_ != nullptr ? std::string(value_) : std::string();
    }
  };

  /// @brief Throw a ``std::runtime_error`` describing a failed QPY call.
  /// @param context what was being attempted, used as the message prefix
  /// @param code the exit code returned by the C-API
  /// @param error the diagnostic reported by the C-API, if any
  static void raise(const std::string &context, const QkExitCode code,
                    const ScopedQpyError &error) {
    std::stringstream message;
    message << "QPY import failed : " << context << " (" << code << ")";
    const std::string detail = error.str();
    if (!detail.empty()) {
      message << " : " << detail;
    }
    throw std::runtime_error(message.str());
  }

  /// @brief Return the circuit index each bit of a register maps to.
  /// @param base output for the circuit index of the register's first bit
  /// @param name the register name, used for diagnostics
  /// @param num_bits the number of bits in the register
  /// @param bits the circuit indices reported by the C-API
  /// @details qiskit-cpp addresses a register's bits as ``base_index + i``, so
  ///   a register whose bits are not a contiguous ascending run of circuit
  ///   indices cannot be represented. Such a register is rejected rather than
  ///   silently given indices that address the wrong bits.
  static void register_base_index(uint_t &base, const std::string &name,
                                  const std::size_t num_bits,
                                  const std::vector<std::uint32_t> &bits) {
    base = 0;
    if (num_bits == 0) {
      return;
    }
    // The C-API reports this for a register bit the circuit does not contain.
    const std::uint32_t missing_bit = std::numeric_limits<std::uint32_t>::max();
    for (std::size_t i = 0; i < num_bits; i++) {
      if (bits[i] == missing_bit) {
        std::stringstream message;
        message << "QPY import failed : bit " << i << " of register \"" << name
                << "\" is not present in the circuit.";
        throw std::runtime_error(message.str());
      }
      if (i > 0 && bits[i] != bits[i - 1] + 1) {
        std::stringstream message;
        message << "QPY import failed : register \"" << name
                << "\" does not occupy a contiguous ascending range of circuit"
                   " bits, which qiskit-cpp cannot represent.";
        throw std::runtime_error(message.str());
      }
    }
    base = (uint_t)bits[0];
  }

  /// @brief Rebuild the quantum registers of an imported circuit in place.
  /// @param circuit the circuit to populate, already holding its C-API circuit
  /// @details ``Register``'s copy constructor copies the bits without
  ///   re-pointing them at the new owner, and ``Bit::global_index()`` follows
  ///   that pointer. The registers are therefore built directly inside the
  ///   circuit's own vector and sized through ``resize()``, which re-points
  ///   every bit at the stored register. Doing this after the circuit reached
  ///   its final location is what keeps those pointers valid.
  static void import_qregs(QuantumCircuit &circuit) {
    const QkCircuit *handle = circuit.rust_circuit_.get();
    const std::size_t num_qregs = qk_circuit_num_quantum_registers(handle);

    circuit.qregs_.clear();
    // Reserve so that the later push_backs cannot reallocate and invalidate
    // the bit pointers set up below.
    circuit.qregs_.reserve(num_qregs);
    for (std::size_t i = 0; i < num_qregs; i++) {
      // Borrowed from the circuit; it must not be freed here.
      const QkQuantumRegister *qreg = qk_circuit_get_quantum_register(handle, i);
      const ScopedStr name(qk_quantum_register_name(qreg));
      const std::size_t num_bits = qk_quantum_register_num_bits(qreg);

      std::vector<std::uint32_t> bits(num_bits == 0 ? 1 : num_bits, 0);
      if (num_bits > 0) {
        qk_quantum_register_circuit_bits(qreg, handle, bits.data());
      }
      uint_t base = 0;
      register_base_index(base, name.str(), num_bits, bits);

      // QuantumRegister allocates its own C-API register. That is only
      // bookkeeping on the qiskit-cpp side: the imported circuit already holds
      // the register, so it must not be added to the circuit again.
      circuit.qregs_.push_back(QuantumRegister((uint_t)num_bits, name.str()));
      QuantumRegister &stored = circuit.qregs_.back();
      stored.resize((uint_t)num_bits);
      stored.set_base_index(base);
    }
  }

  /// @brief Rebuild the classical registers of an imported circuit in place.
  /// @param circuit the circuit to populate, already holding its C-API circuit
  /// @details See ``import_qregs`` for why the registers are built in place.
  static void import_cregs(QuantumCircuit &circuit) {
    const QkCircuit *handle = circuit.rust_circuit_.get();
    const std::size_t num_cregs = qk_circuit_num_classical_registers(handle);

    circuit.cregs_.clear();
    circuit.cregs_.reserve(num_cregs);
    for (std::size_t i = 0; i < num_cregs; i++) {
      // Borrowed from the circuit; it must not be freed here.
      const QkClassicalRegister *creg =
          qk_circuit_get_classical_register(handle, i);
      const ScopedStr name(qk_classical_register_name(creg));
      const std::size_t num_bits = qk_classical_register_num_bits(creg);

      std::vector<std::uint32_t> bits(num_bits == 0 ? 1 : num_bits, 0);
      if (num_bits > 0) {
        qk_classical_register_circuit_bits(creg, handle, bits.data());
      }
      uint_t base = 0;
      register_base_index(base, name.str(), num_bits, bits);

      circuit.cregs_.push_back(ClassicalRegister((uint_t)num_bits, name.str()));
      ClassicalRegister &stored = circuit.cregs_.back();
      stored.resize((uint_t)num_bits);
      stored.set_base_index(base);
    }
  }

  /// @brief Attach a copy of a loaded circuit to a QuantumCircuit.
  /// @param circuit the circuit to populate
  /// @param handle the loaded C-API circuit to copy
  /// @details The circuit is copied rather than adopted: the pointers in a
  ///   ``QkQpyLoadedCircuits`` are owned by the C-API and released as a group
  ///   by ``qk_qpy_loaded_circuits_clear``, so taking one over would mean
  ///   freeing a single element out of that array. The copy costs one
  ///   duplication per circuit at load time and keeps ownership entirely
  ///   within the documented contract.
  static void attach_circuit(QuantumCircuit &circuit, const QkCircuit *handle) {
    QkCircuit *copied = qk_circuit_copy(handle);
    if (copied == nullptr) {
      throw std::runtime_error(
          "QPY import failed : could not copy a loaded circuit.");
    }
    circuit.rust_circuit_ =
        std::shared_ptr<rust_circuit>(copied, qk_circuit_free);

    circuit.num_qubits_ = qk_circuit_num_qubits(copied);
    circuit.num_clbits_ = qk_circuit_num_clbits(copied);

    // The global phase is a property of the circuit, so it is copied straight
    // across. qk_circuit_global_phase hands back an owned QkParam, which
    // Parameter adopts.
    circuit.global_phase_ = Parameter(qk_circuit_global_phase(copied));

    // Collect the measured qubits, as set_qiskit_circuit does for a transpiled
    // circuit. qubit_map_ is left empty: QPY written through the C-API carries
    // no transpiler layout.
    circuit.measure_map_.clear();
    const std::size_t nops = qk_circuit_num_instructions(copied);
    for (std::size_t i = 0; i < nops; i++) {
      if (qk_circuit_instruction_kind(copied, i) == QkOperationKind_Measure) {
        QuantumCircuit::ScopedCircuitInstruction op(copied, i);
        circuit.measure_map_.push_back(std::pair<uint_t, uint_t>(
            (uint_t)op.get().qubits[0], (uint_t)op.get().clbits[0]));
      }
    }
  }

  /// @brief Build QuantumCircuit objects for every loaded circuit.
  /// @param loaded the circuits returned by the C-API
  /// @return the imported circuits, in the order stored in the payload
  static std::vector<QuantumCircuit>
  import_circuits(const ScopedLoadedCircuits &loaded) {
    // Size the vector up front and fill it in place. The registers rebuilt
    // below hold pointers into the register objects stored in each circuit, so
    // the circuits must not be relocated afterwards; growing by push_back
    // could reallocate and leave those pointers behind.
    std::vector<QuantumCircuit> circuits(loaded.size());

    for (std::size_t i = 0; i < loaded.size(); i++) {
      const QkCircuit *handle = loaded.at(i);
      if (handle == nullptr) {
        std::stringstream message;
        message << "QPY import failed : loaded circuit at index " << i
                << " is null.";
        throw std::runtime_error(message.str());
      }
      attach_circuit(circuits[i], handle);
      import_qregs(circuits[i]);
      import_cregs(circuits[i]);
    }
    return circuits;
  }

public:
  /// @brief Read circuits from a QPY file.
  /// @param filename the path to read
  /// @return the circuits stored in the file
  static std::vector<QuantumCircuit> load_file(const std::string &filename) {
    ScopedQpyError error;
    ScopedLoadedCircuits loaded;
    const QkExitCode ret =
        qk_qpy_load_file(loaded.out(), filename.c_str(), error.out());
    if (ret != QkExitCode_Success) {
      raise("could not read \"" + filename + "\"", ret, error);
    }
    return import_circuits(loaded);
  }

  /// @brief Read circuits from a QPY payload.
  /// @param payload the QPY bytes to read
  /// @return the circuits stored in the payload
  static std::vector<QuantumCircuit>
  load_buffer(const std::vector<std::uint8_t> &payload) {
    // The C-API requires a non-null buffer pointer even for a zero length, but
    // std::vector::data() may return null when empty. An empty payload is not
    // valid QPY either way, so it is rejected before the call.
    if (payload.empty()) {
      throw std::runtime_error(
          "QPY import failed : the payload is empty, which is not valid QPY.");
    }
    ScopedQpyError error;
    ScopedLoadedCircuits loaded;
    const QkExitCode ret = qk_qpy_load_buffer(loaded.out(), payload.data(),
                                              payload.size(), error.out());
    if (ret != QkExitCode_Success) {
      raise("could not read the payload", ret, error);
    }
    return import_circuits(loaded);
  }
};

inline std::vector<QuantumCircuit>
QuantumCircuit::import_qpy(const std::string &filename) {
  return QpyImporter::load_file(filename);
}

inline std::vector<QuantumCircuit>
QuantumCircuit::from_qpy(const std::vector<std::uint8_t> &payload) {
  return QpyImporter::load_buffer(payload);
}


class QpyExporter {
protected:
  // RAII guards for Qiskit C-API handles so cleanup is exception-safe and the
  // manual qk_*_free calls cannot be missed on an early return/throw.
  class ScopedQpyError {
    char *message_;

  public:
    ScopedQpyError(void) : message_(nullptr) {}
    ~ScopedQpyError() {
      if (message_ != nullptr) {
        qk_str_free(message_);
      }
    }
    ScopedQpyError(const ScopedQpyError &) = delete;
    ScopedQpyError &operator=(const ScopedQpyError &) = delete;
    char **out(void) { return &message_; }
    std::string str(void) const {
      return message_ != nullptr ? std::string(message_) : std::string();
    }
  };

  class ScopedQpyBuffer {
    std::uint8_t *data_;
    std::size_t size_;

  public:
    ScopedQpyBuffer(void) : data_(nullptr), size_(0) {}
    // qk_qpy_free_buffer is a no-op for a null buffer, and a failed dump leaves
    // both members untouched, so this is safe on every path.
    ~ScopedQpyBuffer() { qk_qpy_free_buffer(data_, size_); }
    ScopedQpyBuffer(const ScopedQpyBuffer &) = delete;
    ScopedQpyBuffer &operator=(const ScopedQpyBuffer &) = delete;
    std::uint8_t **data_out(void) { return &data_; }
    std::size_t *size_out(void) { return &size_; }
    const std::uint8_t *data(void) const { return data_; }
    std::size_t size(void) const { return size_; }
  };

  /// @brief Return a non-null pointer to the start of a handle list.
  /// @param handles the C-API circuits to serialize
  /// @param fallback storage to point at when ``handles`` is empty
  /// @return a pointer the C-API may read ``handles.size()`` entries from
  /// @details The C-API requires a non-null, aligned circuit array even when
  ///   the count is zero, but ``std::vector::data()`` may return null for an
  ///   empty vector. Pointing at ``fallback`` keeps the contract; the C-API
  ///   never dereferences it because the count it is given is zero.
  static const QkCircuit *const *
  handle_data(const std::vector<const QkCircuit *> &handles,
              const QkCircuit *const &fallback) {
    return handles.empty() ? &fallback : handles.data();
  }

  /// @brief Throw a ``std::runtime_error`` describing a failed QPY call.
  /// @param context what was being attempted, used as the message prefix
  /// @param code the exit code returned by the C-API
  /// @param error the diagnostic reported by the C-API, if any
  static void raise(const std::string &context, const QkExitCode code,
                    const ScopedQpyError &error) {
    std::stringstream message;
    message << "QPY export failed : " << context << " (" << code << ")";
    const std::string detail = error.str();
    if (!detail.empty()) {
      message << " : " << detail;
    }
    throw std::runtime_error(message.str());
  }

public:
  /// @brief Collect the C-API circuit handle backing a circuit.
  /// @param circuit the circuit to serialize
  /// @param index the position of the circuit in the payload, for diagnostics
  /// @return the ``QkCircuit`` pointer
  static const QkCircuit *rust_circuit(QuantumCircuit &circuit,
                                       const std::size_t index) {
    // get_rust_circuit() flushes any pending control flow operation, so the
    // payload matches what the circuit would execute. It has to be called on
    // the caller's circuit rather than on a copy: the copy constructor shares
    // the underlying circuit but does not carry the pending operation over.
    const QkCircuit *handle = circuit.get_rust_circuit().get();
    if (handle == nullptr) {
      std::stringstream message;
      message << "QPY export failed : circuit at index " << index
              << " is not backed by a Qiskit C-API circuit.";
      throw std::runtime_error(message.str());
    }
    return handle;
  }

  /// @brief Collect the C-API circuit handles backing a list of circuits.
  /// @param circuits the circuits to serialize
  /// @return the ``QkCircuit`` pointers, in the order given
  static std::vector<const QkCircuit *>
  rust_circuits(std::vector<QuantumCircuit> &circuits) {
    std::vector<const QkCircuit *> handles;
    handles.reserve(circuits.size());
    for (std::size_t i = 0; i < circuits.size(); i++) {
      handles.push_back(rust_circuit(circuits[i], i));
    }
    return handles;
  }

  /// @brief Write circuits to a QPY file.
  /// @param handles the C-API circuits to serialize
  /// @param filename the path to write
  /// @param version the QPY format version, or 0 for the default
  static void dump_file(const std::vector<const QkCircuit *> &handles,
                        const std::string &filename,
                        const std::uint8_t version) {
    ScopedQpyError error;
    const QkCircuit *empty = nullptr;
    const QkCircuit *const *data = handle_data(handles, empty);
    const QkExitCode ret =
        version == 0
            ? qk_qpy_dump_file(data, handles.size(), filename.c_str(),
                               error.out())
            : qk_qpy_dump_file_with_version(data, handles.size(),
                                            filename.c_str(), version,
                                            error.out());
    if (ret != QkExitCode_Success) {
      raise("could not write \"" + filename + "\"", ret, error);
    }
  }

  /// @brief Serialize circuits into a QPY payload.
  /// @param handles the C-API circuits to serialize
  /// @param version the QPY format version, or 0 for the default
  /// @return the QPY payload bytes
  static std::vector<std::uint8_t>
  dump_buffer(const std::vector<const QkCircuit *> &handles,
              const std::uint8_t version) {
    ScopedQpyError error;
    ScopedQpyBuffer buffer;
    const QkCircuit *empty = nullptr;
    const QkCircuit *const *data = handle_data(handles, empty);
    const QkExitCode ret =
        version == 0
            ? qk_qpy_dump_buffer(data, handles.size(), buffer.data_out(),
                                 buffer.size_out(), error.out())
            : qk_qpy_dump_buffer_with_version(data, handles.size(),
                                              buffer.data_out(),
                                              buffer.size_out(), version,
                                              error.out());
    if (ret != QkExitCode_Success) {
      raise("could not serialize the circuits", ret, error);
    }
    if (buffer.data() == nullptr || buffer.size() == 0) {
      return std::vector<std::uint8_t>();
    }
    return std::vector<std::uint8_t>(buffer.data(),
                                     buffer.data() + buffer.size());
  }
};

inline void QuantumCircuit::export_qpy(const std::string &filename,
                                       const std::uint8_t version) {
  const std::vector<const QkCircuit *> handles(
      1, QpyExporter::rust_circuit(*this, 0));
  QpyExporter::dump_file(handles, filename, version);
}

inline std::vector<std::uint8_t>
QuantumCircuit::to_qpy(const std::uint8_t version) {
  const std::vector<const QkCircuit *> handles(
      1, QpyExporter::rust_circuit(*this, 0));
  return QpyExporter::dump_buffer(handles, version);
}

inline void QuantumCircuit::export_qpy(std::vector<QuantumCircuit> &circuits,
                                       const std::string &filename,
                                       const std::uint8_t version) {
  QpyExporter::dump_file(QpyExporter::rust_circuits(circuits), filename,
                         version);
}

inline std::vector<std::uint8_t>
QuantumCircuit::to_qpy(std::vector<QuantumCircuit> &circuits,
                       const std::uint8_t version) {
  return QpyExporter::dump_buffer(QpyExporter::rust_circuits(circuits),
                                  version);
}

inline std::uint8_t QuantumCircuit::qpy_write_min_version(void) {
  return qk_qpy_write_min_version();
}

inline std::uint8_t QuantumCircuit::qpy_read_min_version(void) {
  return qk_qpy_read_min_version();
}

} // namespace circuit
} // namespace Qiskit

#endif // QISKIT_CPP_HAS_QPY

#endif //__qiskitcpp_circuit_qpy_hpp__
