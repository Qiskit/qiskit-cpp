// This code is part of Qiskit.
//
// (C) Copyright IBM 2025.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include <iostream>
#include <cstdint>

#include "circuit/quantumcircuit.hpp"
using namespace Qiskit;
using namespace Qiskit::circuit;

extern "C" {
    #include "common.h"
}

static int test_standard_gates(void) {
    auto circ = QuantumCircuit(4, 4);
    uint_t count = 0;
    CircuitInstruction op;
    std::string name;
    reg_t qubits;
    std::vector<double> params;

    circ.i(0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "id" || qubits[0] != 0) {
        std::cerr << "  standard gate test : id (0) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.h(1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "h" || qubits[0] != 1) {
        std::cerr << "  standard gate test : h (1) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.x(0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "x" || qubits[0] != 0) {
        std::cerr << "  standard gate test : x (0) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.y(0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "y" || qubits[0] != 0) {
        std::cerr << "  standard gate test : y (0) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.z(0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "z" || qubits[0] != 0) {
        std::cerr << "  standard gate test : z (0) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.ch(0, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "ch" || qubits[0] != 0 || qubits[1] != 2) {
        std::cerr << "  standard gate test : cx (0, 1) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cx(0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "cx" || qubits[0] != 0 || qubits[1] != 1) {
        std::cerr << "  standard gate test : cx (0, 1) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cy(0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "cy" || qubits[0] != 0 || qubits[1] != 1) {
        std::cerr << "  standard gate test : cx (0, 1) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cz(0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "cz" || qubits[0] != 0 || qubits[1] != 1) {
        std::cerr << "  standard gate test : cz (0, 1) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.p(0.5, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "p" || qubits[0] != 1 || params[0] != 0.5) {
        std::cerr << "  standard gate test : p 0.5 (1) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.r(0.1, 0.2, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "r" || qubits[0] != 2 || params[0] != 0.1 || params[1] != 0.2) {
        std::cerr << "  standard gate test : r 0.1 0.2 (2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.rx(0.3, 0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rx" || qubits[0] != 0 || params[0] != 0.3) {
        std::cerr << "  standard gate test : rx 0.3 (0) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.ry(0.3, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "ry" || qubits[0] != 1 || params[0] != 0.3) {
        std::cerr << "  standard gate test : rx 0.3 (1) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.rz(0.3, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rz" || qubits[0] != 2 || params[0] != 0.3) {
        std::cerr << "  standard gate test : rz 0.3 (2) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.s(0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "s" || qubits[0] != 0) {
        std::cerr << "  standard gate test : s (0) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.sdg(1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "sdg" || qubits[0] != 1) {
        std::cerr << "  standard gate test : sdg (1) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.sx(2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "sx" || qubits[2] != 0) {
        std::cerr << "  standard gate test : sx (2) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.sxdg(3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "sxdg" || qubits[0] != 3) {
        std::cerr << "  standard gate test : sxdg (3) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.t(0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "t" || qubits[0] != 0) {
        std::cerr << "  standard gate test : t (0) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.tdg(1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "tdg" || qubits[0] != 1) {
        std::cerr << "  standard gate test : tdg (1) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.u(0.1, 0.2, 0.3, 0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "u" || qubits[0] != 0 || params[0] != 0.1 || params[1] != 0.2 || params[2] != 0.3) {
        std::cerr << "  standard gate test : u 0.1 0.2 0.3 (0) != " << name << " " << params[0] << " " << params[1] <<  " " << params[2] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.u1(0.5, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "u1" || qubits[0] != 1 || params[0] != 0.5) {
        std::cerr << "  standard gate test : u1 0.5 (1) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.u2(0.1, 0.2, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "u2" || qubits[0] != 2 || params[0] != 0.1 || params[1] != 0.2) {
        std::cerr << "  standard gate test : u2 0.1 0.2 (2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.u3(0.1, 0.2, 0.3, 0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "u3" || qubits[0] != 0 || params[0] != 0.1 || params[1] != 0.2 || params[2] != 0.3) {
        std::cerr << "  standard gate test : u3 0.1 0.2 0.3 (0) != " << name << " " << params[0] << " " << params[1] <<  " " << params[2] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.dcx(1, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "dcx" || qubits[0] != 1 || qubits[1] != 3) {
        std::cerr << "  standard gate test : dcx (1, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.ecr(0, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "ecr" || qubits[0] != 0 || qubits[1] != 2) {
        std::cerr << "  standard gate test : ecr (0, 2) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.swap(1, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "swap" || qubits[0] != 1 || qubits[1] != 3) {
        std::cerr << "  standard gate test : swap (1, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.iswap(2, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "iswap" || qubits[0] != 2 || qubits[1] != 3) {
        std::cerr << "  standard gate test : iswap (2, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cp(0.5, 0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "cp" || qubits[0] != 0 || qubits[1] != 1 || params[0] != 0.5) {
        std::cerr << "  standard gate test : cp 0.5 (0, 1) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.crx(0.1, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "crx" || qubits[0] != 1 || qubits[1] != 2 || params[0] != 0.1) {
        std::cerr << "  standard gate test : crx 0.1 (1, 2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.cry(0.2, 2, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "cry" || qubits[0] != 2 || qubits[1] != 3 || params[0] != 0.2) {
        std::cerr << "  standard gate test : cry 0.2 (2, 3) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.crz(0.3, 3, 0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "crz" || qubits[0] != 3 || qubits[1] != 0 || params[0] != 0.3) {
        std::cerr << "  standard gate test : crz 0.3 (3, 0) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.cs(1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "cs" || qubits[0] != 1 || qubits[1] != 2) {
        std::cerr << "  standard gate test : cs (1, 2) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.csdg(1, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "csdg" || qubits[0] != 1 || qubits[1] != 3) {
        std::cerr << "  standard gate test : csdg (1, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.csx(3, 0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "csx" || qubits[0] != 3 || qubits[1] != 0) {
        std::cerr << "  standard gate test : csx (3, 0) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cu(0.1, 0.2, 0.3, 0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "cu" || qubits[0] != 0 || qubits[1] != 1 || params[0] != 0.1 || params[1] != 0.2 || params[2] != 0.3) {
        std::cerr << "  standard gate test : cu 0.1 0.2 0.3 (0, 1) != " << name << " " << params[0] << " " << params[1] <<  " " << params[2] << " (" << qubits[0] <<  ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cu1(0.5, 0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "cu1" || qubits[0] != 0 || qubits[1] != 1 || params[0] != 0.5) {
        std::cerr << "  standard gate test : cu1 0.5 (0, 1) != " << name << " " << params[0] << " (" << qubits[0] <<  ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.cu3(0.1, 0.2, 0.3, 0, 1);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "cu3" || qubits[0] != 0 || qubits[1] != 1 || params[0] != 0.1 || params[1] != 0.2 || params[2] != 0.3) {
        std::cerr << "  standard gate test : cu3 0.1 0.2 0.3 (0, 1) != " << name << " " << params[0] << " " << params[1] <<  " " << params[2] << " (" << qubits[0] <<  ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    circ.rxx(0.1, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rxx" || qubits[0] != 1 || qubits[1] != 2 || params[0] != 0.1) {
        std::cerr << "  standard gate test : rxx 0.1 (1, 2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.ryy(0.1, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "ryy" || qubits[0] != 1 || qubits[1] != 2 || params[0] != 0.1) {
        std::cerr << "  standard gate test : ryy 0.1 (1, 2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.rzz(0.1, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rzz" || qubits[0] != 1 || qubits[1] != 2 || params[0] != 0.1) {
        std::cerr << "  standard gate test : rzz 0.1 (1, 2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.rzx(0.1, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rzx" || qubits[0] != 1 || qubits[1] != 2 || params[0] != 0.1) {
        std::cerr << "  standard gate test : rzx 0.1 (1, 2) != " << name << " " << params[0] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.xx_plus_yy(0.1, 0.2, 0, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "xx_plus_yy" || qubits[0] != 0 || qubits[1] != 2 || params[0] != 0.1 || params[1] != 0.2) {
        std::cerr << "  standard gate test : xx_plus_yy 0.1 0.2 (0, 2) != " << name << " " << params[0] << " " << params[1] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.xx_minus_yy(0.3, 0.4, 1, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "xx_minus_yy" || qubits[0] != 1 || qubits[1] != 3 || params[0] != 0.3 || params[1] != 0.4) {
        std::cerr << "  standard gate test : xx_minus_yy 0.3 0.4 (1, 3) != " << name << " " << params[0] << " " << params[1] << " " << params[1] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    circ.ccx(0, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "ccx" || qubits[0] != 0 || qubits[1] != 1 || qubits[2] != 2) {
        std::cerr << "  standard gate test : ccx (0, 1, 2) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] << ")" << std::endl;
        return EqualityError;
    }

    circ.ccz(2, 1, 0);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "ccz" || qubits[0] != 2 || qubits[1] != 1 || qubits[2] != 0) {
        std::cerr << "  standard gate test : ccz (2, 1, 0) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] << ")" << std::endl;
        return EqualityError;
    }

    circ.cswap(0, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "cswap" || qubits[0] != 0 || qubits[1] != 1 || qubits[2] != 2) {
        std::cerr << "  standard gate test : cswap (0, 1, 2) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] << ")" << std::endl;
        return EqualityError;
    }

    circ.rccx(0, 1, 2);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "rccx" || qubits[0] != 0 || qubits[1] != 1 || qubits[2] != 2) {
        std::cerr << "  standard gate test : rccx (0, 1, 2) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] << ")" << std::endl;
        return EqualityError;
    }

    circ.cccx(0, 1, 2, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "mcx" || qubits[0] != 0 || qubits[1] != 1 || qubits[2] != 2 || qubits[3] != 3) {
        std::cerr << "  standard gate test : mcx (0, 1, 2, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] <<  ", " << qubits[3] << ")" << std::endl;
        return EqualityError;
    }

    circ.cccsx(0, 1, 2, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "c3sx" || qubits[0] != 0 || qubits[1] != 1 || qubits[2] != 2 || qubits[3] != 3) {
        std::cerr << "  standard gate test : c3sx (0, 1, 2, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] <<  ", " << qubits[3] << ")" << std::endl;
        return EqualityError;
    }

    circ.rcccx(0, 1, 2, 3);
    op = circ[count++];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "rcccx" || qubits[0] != 0 || qubits[1] != 1 || qubits[2] != 2 || qubits[3] != 3) {
        std::cerr << "  standard gate test : rcccx (0, 1, 2, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] <<  ", " << qubits[2] <<  ", " << qubits[3] << ")" << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_measure(void) {
    uint_t num_qubits = 4;
    auto qr = QuantumRegister(num_qubits);
    auto cr = ClassicalRegister(num_qubits);
    auto circ = QuantumCircuit(qr, cr);

    circ.h(0);
    circ.measure(qr, cr);

    for (uint_t i = 0; i < num_qubits; i++) {
        CircuitInstruction op = circ[i + 1];
        if (op.instruction().name() != "measure" || op.qubits()[0] != i || op.clbits()[0] != i) {
            std::cerr << "  measure test : measure ( " << i << ", " << i << ") != " << op.instruction().name() << " (" << op.qubits()[0] << ", " << op.clbits()[0] << ")" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_append(void) {
    uint_t num_qubits = 4;
    auto circ = QuantumCircuit(num_qubits, num_qubits);
    CircuitInstruction op;
    std::string name;
    reg_t qubits;
    std::vector<double> params;

    circ.h(0);

    circ.append(CXGate(), reg_t({2, 3}));
    op = circ[1];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "cx" || qubits[0] != 2 || qubits[1] != 3) {
        std::cerr << "  append test : cx (2, 3) != " << name << " (" << qubits[0] << ", " << qubits[1] << ")" << std::endl;
        return EqualityError;
    }

    RZGate rz;
    rz.set_params({0.5});
    circ.append(rz, 1);

    op = circ[2];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rz" || qubits[0] != 1) {
        std::cerr << "  append test : rz 0.5 (1) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_compose(void) {
    uint_t num_qubits = 4;
    auto circ = QuantumCircuit(num_qubits, num_qubits);
    CircuitInstruction op;
    std::string name;
    reg_t qubits;
    std::vector<double> params;

    circ.h(0);
    for (uint_t i = 1; i < num_qubits; i++) {
        circ.cx(0, i);
    }

    auto sub = QuantumCircuit(2, 2);
    sub.sx(0);
    sub.rz(0.5, 1);

    uint_t num_inst = circ.num_instructions();
    uint_t total = num_inst + sub.num_instructions();

    circ.compose(sub, reg_t({2, 0}), reg_t({2, 0}));

    if (circ.num_instructions() != total) {
        std::cerr << "  compose test : number of instruction " << total << " != " << circ.num_instructions() << std::endl;
        return EqualityError;
    }

    op = circ[num_inst];
    name = op.instruction().name();
    qubits = op.qubits();
    if (name != "sx" || qubits[0] != 2) {
        std::cerr << "  compose test : sx (2) != " << name << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    op = circ[num_inst + 1];
    name = op.instruction().name();
    qubits = op.qubits();
    params = op.instruction().params();
    if (name != "rz" || qubits[0] != 0) {
        std::cerr << "  compose test : rz 0.5 (0) != " << name << " " << params[0] << " (" << qubits[0] << ")" << std::endl;
        return EqualityError;
    }

    return Ok;
}


extern "C" int test_circuit(void) {
    int num_failed = 0;
    num_failed += RUN_TEST(test_standard_gates);
    num_failed += RUN_TEST(test_measure);
    num_failed += RUN_TEST(test_append);
    num_failed += RUN_TEST(test_compose);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
