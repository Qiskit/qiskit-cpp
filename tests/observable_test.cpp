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

// Test quantm circuit of Qiskit C++

#include <iostream>
#include <cstdint>

#include "quantum_info/sparse_observable.hpp"

using namespace Qiskit::quantum_info;

int main()
{
  auto a = SparseObservable::zero(10);
  auto b = SparseObservable::identity(10);
  a += b;
  std::cout << a.to_string() <<std::endl;


  auto c = SparseObservable::from_label(std::string("-III+IZIXXYXI"));
  std::cout << c.to_string() <<std::endl;

  std::vector<std::pair<std::string, std::complex<double>>> list;
  list.push_back(std::make_pair(std::string("IIIXXIZYII"), std::complex(1.0)));
  list.push_back(std::make_pair(std::string("I++IZIIXII"), std::complex(-1.0)));
  auto d = SparseObservable::from_list(list);
  std::cout << d.to_string() <<std::endl;

  return 0;
}

