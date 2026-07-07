/*
# This code is part of Qiskit.
# (C) Copyright IBM 2017, 2026.
# Any modifications or derivative works of this code must retain this copyright notice.
*/

#ifndef __qiskitcpp_providers_clifft_backend_hpp__
#define __qiskitcpp_providers_clifft_backend_hpp__

#include <string>
#include <iostream>

namespace Qiskit {
namespace providers {

// Wstrzyknięta struktura bazowa, żeby odciąć błąd brakującego qiskit.h
class BackendV2 {
protected:
    std::string name_;
public:
    BackendV2() {}
    BackendV2(std::string name) { name_ = name; }
    BackendV2(BackendV2& other) { name_ = other.name_; }
    virtual ~BackendV2() {}
};

class ClifftBackend : public BackendV2 {
public:
    ClifftBackend() : BackendV2("clifft_backend") {}
    ClifftBackend(std::string name) : BackendV2(name) {}

    std::string name() const {
        return name_;
    }

    void status() const {
        std::cout << "Clifft Backend Provider Prototype Status: ACTIVE" << std::endl;
    }
};

} // namespace providers
} // namespace Qiskit

#endif // __qiskitcpp_providers_clifft_backend_hpp__
