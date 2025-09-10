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

// Quantum Volume circuit

#ifndef __qiskitcpp_circuit_library_quantum_volume_hpp__
#define __qiskitcpp_circuit_library_quantum_volume_hpp__

#include <cmath>

#include "circuit/quantumcircuit.hpp"
#include "utils/rng.hpp"

namespace Qiskit
{
namespace circuit
{

/// @class QuantumVolume
/// @brief A quantum volume model circuit.
class QuantumVolume : public QuantumCircuit
{
public:
    /// @brief Create a new QuantumVolume
    /// @param (num_qubits) number of active qubits in the model circuit
    /// @param (depth) layers of SU(4) operations in model circuit
    /// @param (seed) random number generator or generator seed
    /// @param (classical) use classical permutations at every layer, rather than quantum.
    QuantumVolume(uint_t num_qubits, uint_t depth = 0, uint_t seed = 0, bool classical = true);

protected:
    void make_random_unitary_matrix(std::vector<complex_t> &matrix, uint_t size, RngEngine &rng);
};

QuantumVolume::QuantumVolume(uint_t num_qubits, uint_t depth, uint_t seed, bool classical) : QuantumCircuit(num_qubits, num_qubits, 0.0)
{
    std::vector<complex_t> unitary;
    RngEngine rng;
    if (seed == 0)
        rng.set_random_seed();
    else
        rng.set_seed(seed);
    if (depth == 0)
        depth = num_qubits;
    uint_t width = num_qubits / 2;
    uint_t size = width * depth;

    make_random_unitary_matrix(unitary, size, rng);
    reg_t perm = rng.permutation(num_qubits);
    if (classical)
    {
        for (uint_t i = 0; i < depth; i++)
        {
            for (uint_t j = 0; j < width; j++)
            {
                std::vector<complex_t> mat(16);
                for (uint_t k = 0; k < 16; k++)
                {
                    mat[k] = unitary[(i * width + j) * 16 + k];
                }
                reg_t bits = {perm[j * 2], perm[j * 2 + 1]};
                QuantumCircuit::unitary(mat, bits);
            }
        }
    }
    else
    {
        // implement using swap gates
    }
}

void QuantumVolume::make_random_unitary_matrix(std::vector<complex_t> &matrix, uint_t size, RngEngine &rng)
{
    const double rsq2 = 1.0 / sqrt(2.0);
    matrix.resize(size * 4 * 4);
    // real
    for (uint_t i = 0; i < size * 4 * 4; i++)
    {
        matrix[i] = rng.normal();
    }
    // imag
    for (uint_t i = 0; i < size * 4 * 4; i++)
    {
        matrix[i] = complex_t(matrix[i].real(), rng.normal());
    }

    // QR factorization to make unitary matrix
    for (uint_t i = 0; i < size; i++)
    {
        complex_t Q[4][4];
        complex_t A[4][4];
        // R is not used for random unitary
        for (int_t j = 0; j < 4; j++)
        {
            for (int_t k = 0; k < 4; k++)
            {
                A[j][k] = rsq2 * matrix[i * 4 * 4 + j * 4 + k];
                Q[j][k] = 0.0;
            }
        }
        double r = 0.0;
        for (int_t j = 0; j < 4; j++)
            r += A[j][0].real() * A[j][0].real() + A[j][0].imag() * A[j][0].imag();
        r = sqrt(r);
        for (int_t j = 0; j < 4; j++)
            Q[j][0] = A[j][0] / r;

        for (int_t k = 1; k < 4; k++)
        {
            for (int_t j = 0; j < k; j++)
            {
                complex_t d = 0.0;
                for (int_t l = 0; l < 4; l++)
                    d += A[l][k] * std::conj(Q[l][j]);
                for (int_t l = 0; l < 4; l++)
                    A[l][k] -= d * Q[l][j];
            }
            r = 0.0;
            for (int_t l = 0; l < 4; l++)
                r += A[l][k].real() * A[l][k].real() + A[l][k].imag() * A[l][k].imag();
            r = sqrt(r);
            for (int_t l = 0; l < 4; l++)
                Q[l][k] = A[l][k] / r;
        }
        for (int_t j = 0; j < 4; j++)
        {
            for (int_t k = 0; k < 4; k++)
            {
                matrix[i * 4 * 4 + j * 4 + k] = Q[j][k];
            }
        }
    }
}

} // namespace circuit
} // namespace qiskitcpp

#endif //__qiskitcpp_circuit_library_quantum_volume_hpp__
