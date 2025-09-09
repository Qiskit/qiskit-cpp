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

// rnadom number generator

#ifndef __qiskitcpp_utils_rng_hpp__
#define __qiskitcpp_utils_rng_hpp__

#include <algorithm>
#include <cstdint>
#include <random>

#include "utils/types.hpp"

namespace Qiskit
{

//============================================================================
// RngEngine Class
//
// Objects of this class are used to generate random numbers for backends.
// These are used to decide outcomes of measurements and resets, and for
// implementing noise.
//
//============================================================================

/// @class RngEngine
/// @brief Objects of this class are used to generate random numbers for backends.
/// These are used to decide outcomes of measurements and resets, and for implementing noise.
class RngEngine
{
public:
    //-----------------------------------------------------------------------
    // Constructors
    //-----------------------------------------------------------------------

    // Default constructor initialize RNG engine with a random seed
    /// @brief Default constructor initialize RNG engine with a random seed
    RngEngine() { set_random_seed(); }

    // Seeded constructor initialize RNG engine with a fixed seed
    /// @brief Seeded constructor initialize RNG engine with a fixed seed
    explicit RngEngine(size_t seed) { set_seed(seed); }

    //-----------------------------------------------------------------------
    // Set fixed or random seed for RNG
    //-----------------------------------------------------------------------

    // Set random seed for the RNG engine
    /// @brief Set random seed for the RNG engine
    void set_random_seed()
    {
        std::random_device rd;
        set_seed(rd());
    }

    // Set a fixed seed for the RNG engine
    /// @brief Set a fixed seed for the RNG engine
    /// @param (seed) generator seed
    void set_seed(size_t seed)
    {
        rng.seed(seed);
        initial_seed_ = seed;
    }

    /// @brief Return seed used to initialize rng engine
    /// @return seed used to initialize rng engine
    size_t initial_seed(void) { return initial_seed_; }

    //-----------------------------------------------------------------------
    // Sampling methods
    //-----------------------------------------------------------------------

    // Generate a uniformly distributed pseudo random real in the half-open
    // interval [a,b)
    /// @brief Generate a uniformly distributed pseudo random real in the half-open
    /// interval [a,b)
    /// @param (a) a of [a, b)
    /// @param (b) b of [a, b)
    /// @return a uniformly distributed pseudo random real
    double rand(double a, double b)
    {
        return std::uniform_real_distribution<double>(a, b)(rng);
    }

    // Generate a uniformly distributed pseudo random real in the half-open
    // interval [0,b)
    /// @brief Generate a uniformly distributed pseudo random real in the half-open
    /// interval [0,b)
    /// @param (b) b of [0, b)
    /// @return a uniformly distributed pseudo random real
    double rand(double b) { return rand(double(0), b); };

    // Generate a uniformly distributed pseudo random real in the half-open
    // interval [0,1)
    /// @brief Generate a uniformly distributed pseudo random real in the half-open
    /// interval [0,1)
    /// @return a uniformly distributed pseudo random real
    double rand() { return rand(0, 1); };

    // Generate a normal distributed pseudo random real in the half-open
    // interval [a,b)
    /// @brief Generate a normal distributed pseudo random real in the half-open
    /// interval [a,b)
    /// @return a normal distributed pseudo random real
    double normal()
    {
        return std::normal_distribution<double>(0.0, 1.0)(rng);
    }

    // make permutation array
    /// @brief  make permutation array
    reg_t permutation(uint_t n)
    {
        reg_t ret(n);
        for (uint_t i = 0; i < n; i++)
            ret[i] = i;
        std::shuffle(ret.begin(), ret.end(), rng);
        return ret;
    }

    // Generate a uniformly distributed pseudo random integer in the closed
    // interval [a,b]
    uint_t rand_int(uint_t a, uint_t b)
    {
        return std::uniform_int_distribution<uint_t>(a, b)(rng);
    }

    // Generate a pseudo random integer from a a discrete distribution
    // constructed from an input vector of probabilities for [0,..,n-1]
    // where n is the lenght of the vector. If this vector is not normalized
    // it will be scaled when it is converted to a discrete_distribution
    size_t rand_int(const std::vector<double> &probs)
    {
        return std::discrete_distribution<size_t>(probs.begin(), probs.end())(rng);
    }

private:
    std::mt19937_64 rng;  // Mersenne twister rng engine
    size_t initial_seed_; // save seed used to initialize rng engine
};

//------------------------------------------------------------------------------
} // namespace Qiskit
#endif
