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

// statevector sampler implementation

#ifndef __qiskitcpp_primitives_statevector_sampler_def_hpp__
#define __qiskitcpp_primitives_statevector_sampler_def_hpp__

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "primitives/local_primitive_job.hpp"
#include "utils/rng.hpp"
//#include "qc_statevector.h"

namespace Qiskit {
namespace primitives {

/*

/// @class StatevectorSampler
/// @brief Implementation of SamplerV2 using state vector simulation.
class StatevectorSampler {
protected:
    uint_t default_shots_;
    int_t seed_;
    LocalPrimitiveJob job_;
public:
    /// @brief Create a new StatevectorSampler
    /// @param default_shots The default shots
    /// @param seed The seed or Generator object for random number generation.
    StatevectorSampler(uint_t default_shots = 1024, int_t seed = -1)
    {
        default_shots_ = default_shots;
        seed_ = seed;
    }

    /// @brief Run and collect samples from each pub.
    /// @pubs An iterable of pub-like objects.
    /// @return LocalPrimitiveJob
    LocalPrimitiveJob& run(std::vector<SamplerPub> pubs)
    {
        RngEngine rng;
        if (seed_ >= 0) {
            rng.set_seed(seed_);
        }

        // allocate new results
        job_.result() = PrimitiveResult(pubs.size());

        for (int_t ipub=0;ipub<pubs.size();ipub++) {
            SamplerPub& pub = pubs[ipub];

            Statevector* sv = qc_statevector(pub.circuit().num_qubits(), pub.circuit().num_clbits());

            if (pub.values().size() > 0) {
                auto param_circ = qiskitcpp::circuit::QuantumCircuit(pub.circuit());
                param_circ.assign_parameters(pub.params(), pub.values());
                qc_statevector_run(sv, param_circ.get_rust_circuit());
            }
            else{
                qc_statevector_run(sv, pub.circuit().get_rust_circuit());
            }
            uint_t num_shots = default_shots_;
            if (pub.shots() > 0)
                num_shots = pub.shots();

            std::vector<double> shots(num_shots);
            for(int i=0;i<num_shots;i++) {
                shots[i] = rng.rand();
            }
            uint_t* samples = qc_statevector_sampling(sv,num_shots,shots.data());
            job_.result()[ipub].data().from_samples(samples, num_shots, pub.circuit().num_qubits());

            qc_statevector_free(sv);
            qc_statevector_sampling_free(samples);
        }
        return job_;
    }
};

*/

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_statevector_sampler_def_hpp__
