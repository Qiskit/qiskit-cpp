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
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
using namespace Qiskit;
using namespace Qiskit::circuit;

static int test_standard_gates(void) {
    auto circ = QuantumCircuit(4, 4);
    uint_t count = 0;
    CircuitInstruction op;
    std::string name;
    reg_t qubits;
    std::vector<Parameter> params;

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
    if (name != "sx" || qubits[0] != 2) {
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
    std::vector<Parameter> params;

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
    std::vector<Parameter> params;

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

static int test_delay(void) {
    auto circ = QuantumCircuit(2, 0);

    if (!circ.delay(100, (uint_t)0)) {
        std::cerr << "  delay test : delay in dt was rejected" << std::endl;
        return RuntimeError;
    }
    if (!circ.delay(250.5, (uint_t)1, DelayUnit::ns)) {
        std::cerr << "  delay test : delay in ns was rejected" << std::endl;
        return RuntimeError;
    }
    if (circ.num_instructions() != 2) {
        std::cerr << "  delay test : " << circ.num_instructions()
            << " instructions, expected 2" << std::endl;
        return EqualityError;
    }

    // Read the instructions back and confirm the duration, unit and qubit all
    // round-trip through the C-API.
    CircuitInstruction op = circ[0];
    if (op.instruction().name() != "delay" || op.qubits()[0] != 0
        || op.instruction().delay_duration() != 100.0
        || op.instruction().delay_unit() != QkDelayUnit_DT) {
        std::cerr << "  delay test : instruction 0 is \"" << op.instruction().name()
            << "\" of " << op.instruction().delay_duration() << " on qubit "
            << op.qubits()[0] << ", expected a 100 dt delay on qubit 0" << std::endl;
        return EqualityError;
    }

    op = circ[1];
    if (op.instruction().name() != "delay" || op.qubits()[0] != 1
        || op.instruction().delay_duration() != 250.5
        || op.instruction().delay_unit() != QkDelayUnit_NS) {
        std::cerr << "  delay test : instruction 1 is \"" << op.instruction().name()
            << "\" of " << op.instruction().delay_duration() << " on qubit "
            << op.qubits()[0] << ", expected a 250.5 ns delay on qubit 1" << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_delay_all_qubits(void) {
    // Omitting the qubit delays every qubit, as in Qiskit.
    auto circ = QuantumCircuit(3, 0);
    if (!circ.delay(64)) {
        std::cerr << "  delay_all_qubits test : delay was rejected" << std::endl;
        return RuntimeError;
    }
    if (circ.num_instructions() != 3) {
        std::cerr << "  delay_all_qubits test : " << circ.num_instructions()
            << " instructions, expected 3" << std::endl;
        return EqualityError;
    }
    for (uint_t i = 0; i < 3; i++) {
        CircuitInstruction op = circ[i];
        if (op.instruction().name() != "delay" || op.qubits()[0] != i
            || op.instruction().delay_duration() != 64.0) {
            std::cerr << "  delay_all_qubits test : instruction " << i << " is \""
                << op.instruction().name() << "\" on qubit " << op.qubits()[0]
                << ", expected a 64 dt delay on qubit " << i << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_delay_register(void) {
    auto qreg1 = QuantumRegister(1, std::string("a"));
    auto qreg2 = QuantumRegister(2, std::string("b"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg1, qreg2}),
                        std::vector<ClassicalRegister>());

    // The second register occupies circuit qubits 1 and 2.
    if (!circ.delay(32, qreg2)) {
        std::cerr << "  delay_register test : delay was rejected" << std::endl;
        return RuntimeError;
    }
    if (circ.num_instructions() != 2) {
        std::cerr << "  delay_register test : " << circ.num_instructions()
            << " instructions, expected 2" << std::endl;
        return EqualityError;
    }
    for (uint_t i = 0; i < 2; i++) {
        CircuitInstruction op = circ[i];
        if (op.qubits()[0] != 1 + i) {
            std::cerr << "  delay_register test : instruction " << i << " is on qubit "
                << op.qubits()[0] << ", expected " << (1 + i) << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_delay_unit_names(void) {
    auto circ = QuantumCircuit(1, 0);
    const char *names[] = {"dt", "s", "ms", "us", "ns", "ps"};
    const QkDelayUnit expected[] = {QkDelayUnit_DT, QkDelayUnit_S, QkDelayUnit_MS,
                                    QkDelayUnit_US, QkDelayUnit_NS, QkDelayUnit_PS};

    for (uint_t i = 0; i < 6; i++) {
        if (!circ.delay(8, (uint_t)0, std::string(names[i]))) {
            std::cerr << "  delay_unit_names test : unit \"" << names[i]
                << "\" was rejected" << std::endl;
            return RuntimeError;
        }
        CircuitInstruction op = circ[i];
        if (op.instruction().delay_unit() != expected[i]) {
            std::cerr << "  delay_unit_names test : unit \"" << names[i]
                << "\" round-tripped as " << op.instruction().delay_unit() << std::endl;
            return EqualityError;
        }
    }

    // An unrecognized unit name is rejected without adding an instruction.
    if (circ.delay(8, (uint_t)0, std::string("fortnights"))) {
        std::cerr << "  delay_unit_names test : an unknown unit was accepted" << std::endl;
        return EqualityError;
    }
    if (circ.num_instructions() != 6) {
        std::cerr << "  delay_unit_names test : " << circ.num_instructions()
            << " instructions, expected 6" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_delay_invalid_duration(void) {
    auto circ = QuantumCircuit(1, 0);

    // A negative duration is rejected.
    if (circ.delay(-1.0, (uint_t)0)) {
        std::cerr << "  delay_invalid_duration test : a negative duration was accepted"
            << std::endl;
        return EqualityError;
    }
    // A fractional dt duration cannot be represented, since dt counts whole
    // backend cycles, so it is rejected rather than truncated.
    if (circ.delay(1.5, (uint_t)0)) {
        std::cerr << "  delay_invalid_duration test : a fractional dt duration was accepted"
            << std::endl;
        return EqualityError;
    }
    // The same value is fine in a time unit.
    if (!circ.delay(1.5, (uint_t)0, DelayUnit::us)) {
        std::cerr << "  delay_invalid_duration test : a fractional us duration was rejected"
            << std::endl;
        return RuntimeError;
    }
    if (circ.num_instructions() != 1) {
        std::cerr << "  delay_invalid_duration test : " << circ.num_instructions()
            << " instructions, expected 1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_delay_append(void) {
    auto circ = QuantumCircuit(2, 0);

    // Appending a Delay instruction must behave as calling delay() directly,
    // including through CircuitInstruction, which stores the instruction by
    // value and so relies on the duration surviving a copy to the base class.
    circ.append(Delay(16, DelayUnit::us), (uint_t)0);
    circ.append(CircuitInstruction(Delay(48, DelayUnit::ms), reg_t({1})));

    if (circ.num_instructions() != 2) {
        std::cerr << "  delay_append test : " << circ.num_instructions()
            << " instructions, expected 2" << std::endl;
        return EqualityError;
    }

    CircuitInstruction op = circ[0];
    if (op.instruction().delay_duration() != 16.0
        || op.instruction().delay_unit() != QkDelayUnit_US || op.qubits()[0] != 0) {
        std::cerr << "  delay_append test : appended instruction 0 is "
            << op.instruction().delay_duration() << " unit "
            << op.instruction().delay_unit() << " on qubit " << op.qubits()[0]
            << ", expected 16 us on qubit 0" << std::endl;
        return EqualityError;
    }

    op = circ[1];
    if (op.instruction().delay_duration() != 48.0
        || op.instruction().delay_unit() != QkDelayUnit_MS || op.qubits()[0] != 1) {
        std::cerr << "  delay_append test : appended instruction 1 is "
            << op.instruction().delay_duration() << " unit "
            << op.instruction().delay_unit() << " on qubit " << op.qubits()[0]
            << ", expected 48 ms on qubit 1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_delay_compose(void) {
    auto sub = QuantumCircuit(1, 0);
    sub.delay(120, (uint_t)0, DelayUnit::ns);

    auto circ = QuantumCircuit(2, 0);
    circ.h(0);
    circ.compose(sub, reg_t({1}), reg_t());

    if (circ.num_instructions() != 2) {
        std::cerr << "  delay_compose test : " << circ.num_instructions()
            << " instructions, expected 2" << std::endl;
        return EqualityError;
    }
    CircuitInstruction op = circ[1];
    if (op.instruction().name() != "delay" || op.qubits()[0] != 1
        || op.instruction().delay_duration() != 120.0
        || op.instruction().delay_unit() != QkDelayUnit_NS) {
        std::cerr << "  delay_compose test : composed instruction is \""
            << op.instruction().name() << "\" of " << op.instruction().delay_duration()
            << " unit " << op.instruction().delay_unit() << " on qubit " << op.qubits()[0]
            << ", expected a 120 ns delay on qubit 1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_delay(void) {
    auto circ = QuantumCircuit(2, 0);
    circ.delay(100, (uint_t)0);
    circ.delay(250.5, (uint_t)1, DelayUnit::ns);
    circ.h(0);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[2] q;\n"
        "delay[100dt] q[0];\n"
        "delay[250.5ns] q[1];\n"
        "h q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_delay test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_delay_picoseconds(void) {
    // OpenQASM 3 has no picosecond literal, so a ps duration is emitted in
    // nanoseconds, as Qiskit's own exporter does.
    auto circ = QuantumCircuit(1, 0);
    circ.delay(2000, (uint_t)0, DelayUnit::ps);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[1] q;\n"
        "delay[2ns] q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_delay_picoseconds test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_delay_with_parameter(void) {
    // A delay's duration is a numeric literal, so it must not be mistaken for a
    // parameter symbol: parameter_symbols() cross-checks its harvested names
    // against the Rust-side symbol count and throws on a mismatch.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);
    circ.delay(250.5, (uint_t)0, DelayUnit::ns);

    const auto symbols = circ.parameter_symbols();
    if (symbols.size() != 1 || symbols[0] != "theta") {
        std::cerr << "  to_qasm3_delay_with_parameter test : harvested " << symbols.size()
            << " symbols, expected just \"theta\"" << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] theta;\n"
        "qubit[1] q;\n"
        "rx(theta) q[0];\n"
        "delay[250.5ns] q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_delay_with_parameter test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_multi_regs(void) {
    auto qreg1 = QuantumRegister(2, std::string("q1"));
    auto qreg2 = QuantumRegister(1, std::string("q2"));
    auto creg1 = ClassicalRegister(2, std::string("c1"));
    auto creg2 = ClassicalRegister(1, std::string("c2"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg1, qreg2}), std::vector<ClassicalRegister>({creg1, creg2}));

    circ.measure(qreg2, creg2);
    circ.measure(qreg1, creg1);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[3] q;\n"
        "bit[2] c1;\n"
        "bit[1] c2;\n"
        "c2[0] = measure q[2];\n"
        "c1[0] = measure q[0];\n"
        "c1[1] = measure q[1];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_multi_regs test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_parameterized(void) {
    auto qreg = QuantumRegister(2, std::string("q"));
    auto creg = ClassicalRegister(1, std::string("c"));
    QuantumCircuit circ(qreg, creg);
    auto theta = Parameter("theta");
    auto phi = Parameter("phi");
    auto expr = theta + phi;

    circ.rx(theta, 0);
    circ.rz(expr, 1);
    circ.measure(0, 0);

    if (circ.num_parameters() != 2) {
        std::cerr << "  to_qasm3_parameterized test : number of parameters 2 != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] phi;\n"
        "input float[64] theta;\n"
        "qubit[2] q;\n"
        "bit[1] c;\n"
        "rx(theta) q[0];\n"
        "rz(phi + theta) q[1];\n"
        "c[0] = measure q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_parameterized test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_parameter_name_conflicts(void) {
    auto qreg = QuantumRegister(1, std::string("q"));
    auto creg = ClassicalRegister(1, std::string("c"));
    QuantumCircuit circ(qreg, creg);
    auto qparam = Parameter("q");
    auto cparam = Parameter("c");
    auto rzparam = Parameter("rz");
    auto sinparam = Parameter("sin");
    auto measure_param = Parameter("measure");

    circ.rx(qparam, 0);
    circ.ry(cparam, 0);
    circ.rz(rzparam, 0);
    circ.p(sinparam, 0);
    circ.rx(measure_param, 0);
    circ.measure(0, 0);

    if (circ.num_parameters() != 5) {
        std::cerr << "  to_qasm3_parameter_name_conflicts test : number of parameters 5 != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] c;\n"
        "input float[64] measure_0;\n"
        "input float[64] q;\n"
        "input float[64] rz_0;\n"
        "input float[64] sin;\n"
        "qubit[1] q_0;\n"
        "bit[1] c_0;\n"
        "rx(q) q_0[0];\n"
        "ry(c) q_0[0];\n"
        "rz(rz_0) q_0[0];\n"
        "p(sin) q_0[0];\n"
        "rx(measure_0) q_0[0];\n"
        "c_0[0] = measure q_0[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_parameter_name_conflicts test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_stdgate_name_conflicts(void) {
    auto qreg = QuantumRegister(1, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    const std::vector<std::string> names = {
        "CX", "cphase", "crx", "cry", "crz", "cswap",
        "cu", "id", "phase", "u1", "u2", "u3"
    };

    for (const auto &name : names) {
        auto param = Parameter(name);
        circ.rx(param, 0);
    }

    if (circ.num_parameters() != names.size()) {
        std::cerr << "  to_qasm3_stdgate_name_conflicts test : number of parameters " << names.size()
            << " != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] CX_0;\n"
        "input float[64] cphase_0;\n"
        "input float[64] crx_0;\n"
        "input float[64] cry_0;\n"
        "input float[64] crz_0;\n"
        "input float[64] cswap_0;\n"
        "input float[64] cu_0;\n"
        "input float[64] id_0;\n"
        "input float[64] phase_0;\n"
        "input float[64] u1_0;\n"
        "input float[64] u2_0;\n"
        "input float[64] u3_0;\n"
        "qubit[1] q;\n"
        "rx(CX_0) q[0];\n"
        "rx(cphase_0) q[0];\n"
        "rx(crx_0) q[0];\n"
        "rx(cry_0) q[0];\n"
        "rx(crz_0) q[0];\n"
        "rx(cswap_0) q[0];\n"
        "rx(cu_0) q[0];\n"
        "rx(id_0) q[0];\n"
        "rx(phase_0) q[0];\n"
        "rx(u1_0) q[0];\n"
        "rx(u2_0) q[0];\n"
        "rx(u3_0) q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_stdgate_name_conflicts test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_keyword_and_constant_name_conflicts(void) {
    auto qreg = QuantumRegister(1, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    const std::vector<std::string> names = {
        "I", "array", "false", "im", "pi", "qreg", "readonly", "switch", "true"
    };

    for (const auto &name : names) {
        auto param = Parameter(name);
        circ.rx(param, 0);
    }

    if (circ.num_parameters() != names.size()) {
        std::cerr << "  to_qasm3_keyword_and_constant_name_conflicts test : number of parameters "
            << names.size() << " != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] I_0;\n"
        "input float[64] array_0;\n"
        "input float[64] false_0;\n"
        "input float[64] im_0;\n"
        "input float[64] pi_0;\n"
        "input float[64] qreg_0;\n"
        "input float[64] readonly_0;\n"
        "input float[64] switch_0;\n"
        "input float[64] true_0;\n"
        "qubit[1] q;\n"
        "rx(I_0) q[0];\n"
        "rx(array_0) q[0];\n"
        "rx(false_0) q[0];\n"
        "rx(im_0) q[0];\n"
        "rx(pi_0) q[0];\n"
        "rx(qreg_0) q[0];\n"
        "rx(readonly_0) q[0];\n"
        "rx(switch_0) q[0];\n"
        "rx(true_0) q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_keyword_and_constant_name_conflicts test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_classical_register_name_conflicts(void) {
    auto qreg = QuantumRegister(1, std::string("q"));
    auto stdgate_creg = ClassicalRegister(1, std::string("cu"));
    auto keyword_creg = ClassicalRegister(1, std::string("creg"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>({stdgate_creg, keyword_creg}));

    circ.measure(qreg, stdgate_creg);
    circ.measure(qreg, keyword_creg);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[1] q;\n"
        "bit[1] cu_0;\n"
        "bit[1] creg_0;\n"
        "cu_0[0] = measure q[0];\n"
        "creg_0[0] = measure q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_classical_register_name_conflicts test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_custom_gate_name_conflicts(void) {
    auto qreg = QuantumRegister(2, std::string("q"));
    auto creg = ClassicalRegister(1, std::string("rzz"));
    QuantumCircuit circ(qreg, creg);
    auto rzzparam = Parameter("rzz");

    circ.rzz(rzzparam, 0, 1);
    circ.measure(0, 0);

    if (circ.num_parameters() != 1) {
        std::cerr << "  to_qasm3_custom_gate_name_conflicts test : number of parameters 1 != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] rzz_0;\n"
        "gate rzz(p0) _gate_q_0, _gate_q_1 {\n"
        "  cx _gate_q_0, _gate_q_1;\n"
        "  rz(p0) _gate_q_1;\n"
        "  cx _gate_q_0, _gate_q_1;\n"
        "}\n"
        "qubit[2] q;\n"
        "bit[1] rzz_1;\n"
        "rzz(rzz_0) q[0], q[1];\n"
        "rzz_1[0] = measure q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_custom_gate_name_conflicts test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_cu1_definition(void) {
    auto qreg = QuantumRegister(2, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    auto theta = Parameter("theta");

    circ.cu1(theta, 0, 1);

    if (circ.num_parameters() != 1) {
        std::cerr << "  to_qasm3_cu1_definition test : number of parameters 1 != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] theta;\n"
        "gate cu1(p0) _gate_q_0, _gate_q_1 {\n"
        "  cp(p0) _gate_q_0, _gate_q_1;\n"
        "}\n"
        "qubit[2] q;\n"
        "cu1(theta) q[0], q[1];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_cu1_definition test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_cu3_definition(void) {
    auto qreg = QuantumRegister(2, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    auto theta = Parameter("theta");
    auto phi = Parameter("phi");
    auto lam = Parameter("lam");

    circ.cu3(theta, phi, lam, 0, 1);

    if (circ.num_parameters() != 3) {
        std::cerr << "  to_qasm3_cu3_definition test : number of parameters 3 != " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] lam;\n"
        "input float[64] phi;\n"
        "input float[64] theta;\n"
        "gate cu3(p0, p1, p2) _gate_q_0, _gate_q_1 {\n"
        "  cu(p0, p1, p2, 0) _gate_q_0, _gate_q_1;\n"
        "}\n"
        "qubit[2] q;\n"
        "cu3(theta, phi, lam) q[0], q[1];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_cu3_definition test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_global_phase(void) {
    // The global phase is a property of the circuit, set through
    // qk_circuit_set_global_phase, so the exporter reads it from the circuit
    // rather than finding a global_phase instruction to iterate over. It is
    // emitted after the register declarations and before the operations.
    auto qreg = QuantumRegister(1, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>(), 0.5);

    circ.h(0);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[1] q;\n"
        "gphase(0.5);\n"
        "h q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_global_phase test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_global_phase_zero(void) {
    // A zero phase is the identity, so nothing is emitted for it.
    auto qreg = QuantumRegister(1, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>(), 0.0);

    circ.h(0);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[1] q;\n"
        "h q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_global_phase_zero test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    // A circuit built without an explicit phase must behave the same way.
    QuantumCircuit implicit(1, 0);
    implicit.h(0);
    if (implicit.to_qasm3() != expected) {
        std::cerr << "  to_qasm3_global_phase_zero test : \n    expected:\n" << expected
            << "\n    actual:\n" << implicit.to_qasm3() << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_to_qasm3_global_phase_setter(void) {
    // Setting the phase after construction must replace it rather than
    // accumulate, since it is a property and not an appended gate.
    QuantumCircuit circ(1, 0, 0.25);
    circ.global_phase(0.75);
    circ.h(0);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[1] q;\n"
        "gphase(0.75);\n"
        "h q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_global_phase_setter test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    if (circ.global_phase() != 0.75) {
        std::cerr << "  to_qasm3_global_phase_setter test : the cached phase is "
            << circ.global_phase().as_str() << ", expected 0.75" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_global_phase_parameter(void) {
    // An unbound phase has no numeric value, so it is always emitted and is
    // declared as an input alongside the gate parameters.
    QuantumCircuit circ(1, 0);
    auto theta = Parameter("theta");
    circ.global_phase(theta);
    circ.h(0);

    const auto symbols = circ.parameter_symbols();
    if (symbols.size() != 1 || symbols[0] != "theta") {
        std::cerr << "  to_qasm3_global_phase_parameter test : harvested " << symbols.size()
            << " symbols, expected just \"theta\"" << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] theta;\n"
        "qubit[1] q;\n"
        "gphase(theta);\n"
        "h q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_global_phase_parameter test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_global_phase_gate(void) {
    // A GlobalPhaseGate appended as an instruction is still emitted in
    // sequence, which is a separate path from the circuit property above.
    QuantumCircuit circ(1, 0);
    circ.h(0);

    auto gate = GlobalPhaseGate();
    gate.set_params(std::vector<Parameter>({Parameter(0.5)}));
    circ.append(gate, reg_t());

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "qubit[1] q;\n"
        "h q[0];\n"
        "gphase(0.5);\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_global_phase_gate test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_to_qasm3_physical_qubits(void) {
    auto circ = QuantumCircuit(156, 0);
    circ.h(21);
    circ.cx(21, 22);

    std::vector<std::uint32_t> layout({21, 22});
    circ.set_qiskit_circuit(circ.get_rust_circuit(), layout);

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "h $21;\n"
        "cx $21, $22;\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_physical_qubits test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_parameter_symbols(void) {
    auto qreg = QuantumRegister(2, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    auto theta = Parameter("theta");
    auto phi = Parameter("phi");
    auto qparam = Parameter("q");
    auto sinparam = Parameter("sin");
    auto measure_param = Parameter("measure");

    circ.rx(theta, 0);
    circ.rz(theta + phi, 1);   // reuse theta; function-style names must not leak
    circ.p((phi * 2.0).sin(), 0);
    circ.rx(qparam, 0);
    circ.ry(sinparam, 1);
    circ.rz(measure_param, 0);

    const auto symbols = circ.parameter_symbols();

    // Names are original, unique, sorted, and exclude QASM math function tokens.
    const std::vector<std::string> expected = {"measure", "phi", "q", "sin", "theta"};
    if (symbols != expected) {
        std::cerr << "  parameter_symbols test : unexpected list, got:";
        for (const auto &s : symbols) std::cerr << " " << s;
        std::cerr << std::endl;
        return EqualityError;
    }

    // The derived list must stay consistent with the Rust-side symbol count.
    if (symbols.size() != circ.num_parameters()) {
        std::cerr << "  parameter_symbols test : size " << symbols.size()
            << " != num_parameters " << circ.num_parameters() << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_parameter_symbols_unsupported_name(void) {
    auto qreg = QuantumRegister(1, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    auto param = Parameter("a-b");

    circ.rx(param, 0);

    try {
        circ.parameter_symbols();
    } catch (const std::runtime_error &err) {
        const std::string message = err.what();
        if (message.find("parameter_symbols cannot derive") == std::string::npos) {
            std::cerr << "  parameter_symbols_unsupported_name test : unexpected error: "
                << message << std::endl;
            return EqualityError;
        }
        return Ok;
    }

    std::cerr << "  parameter_symbols_unsupported_name test : expected runtime_error"
        << std::endl;
    return EqualityError;
}

// Documented limitation (per maintainer review on PR #159): qiskit-cpp cannot
// disambiguate distinct Parameter objects that share a name (no UUID through the
// C-API), so they collapse onto a single shared symbol of that name. This case
// is deliberately not detected or rejected -- preflight-copying the circuit on
// every parameterized gate just to detect it is too expensive. The exported
// OpenQASM 3 stays valid; the colliding gates simply bind the same input. Use a
// unique name per distinct parameter to avoid this.
static int test_parameter_symbols_duplicate_name_limitation(void) {
    // Two distinct Parameter("a") objects across separate gates collapse to one
    // symbol "a"; both gates reference it and the QASM remains valid.
    {
        QuantumCircuit circ(2, 0);
        auto a0 = Parameter("a");
        auto a1 = Parameter("a");
        circ.rx(a0, 0);
        circ.ry(a1, 1);

        if (circ.num_parameters() != 1) {
            std::cerr << "  parameter_symbols_duplicate_name_limitation test : expected 1 collapsed "
                << "symbol, got " << circ.num_parameters() << std::endl;
            return EqualityError;
        }
        const auto symbols = circ.parameter_symbols();
        if (symbols != std::vector<std::string>({"a"})) {
            std::cerr << "  parameter_symbols_duplicate_name_limitation test : unexpected symbol list, got:";
            for (const auto &s : symbols) std::cerr << " " << s;
            std::cerr << std::endl;
            return EqualityError;
        }

        const std::string expected =
            "OPENQASM 3.0;\n"
            "include \"stdgates.inc\";\n"
            "input float[64] a;\n"
            "qubit[2] q;\n"
            "rx(a) q[0];\n"
            "ry(a) q[1];\n";
        if (circ.to_qasm3() != expected) {
            std::cerr << "  parameter_symbols_duplicate_name_limitation test : \n    expected:\n" << expected
                << "\n    actual:\n" << circ.to_qasm3() << std::endl;
            return EqualityError;
        }
    }

    // The same collapse happens for duplicate names within a single
    // multi-parameter gate.
    {
        QuantumCircuit circ(1, 0);
        auto b0 = Parameter("b");
        auto b1 = Parameter("b");
        circ.r(b0, b1, 0);

        if (circ.num_parameters() != 1
            || circ.parameter_symbols() != std::vector<std::string>({"b"})) {
            std::cerr << "  parameter_symbols_duplicate_name_limitation test : multi-param gate did "
                << "not collapse to a single symbol" << std::endl;
            return EqualityError;
        }
    }

    return Ok;
}

// parameter_symbols() reads the parameter expressions from the Rust circuit, so
// it recovers parameters introduced through compose() without any per-Parameter
// bookkeeping on the C++ side.
static int test_to_qasm3_compose_parameterized(void) {
    auto qreg = QuantumRegister(2, std::string("q"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}), std::vector<ClassicalRegister>());
    QuantumCircuit sub(2, 0);
    auto theta = Parameter("theta");
    auto phi = Parameter("phi");
    sub.rx(theta, 0);
    sub.ry(phi, 1);

    circ.compose(sub, reg_t({0, 1}), reg_t({}));

    const auto symbols = circ.parameter_symbols();
    if (symbols != std::vector<std::string>({"phi", "theta"})) {
        std::cerr << "  to_qasm3_compose_parameterized test : unexpected symbol list, got:";
        for (const auto &s : symbols) std::cerr << " " << s;
        std::cerr << std::endl;
        return EqualityError;
    }

    const auto actual = circ.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] phi;\n"
        "input float[64] theta;\n"
        "qubit[2] q;\n"
        "rx(theta) q[0];\n"
        "ry(phi) q[1];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_compose_parameterized test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

// parameters introduced through append() of an existing instruction are likewise
// recovered, because the symbol list is derived from the Rust circuit.
static int test_to_qasm3_append_parameterized(void) {
    QuantumCircuit src(1, 0);
    auto theta = Parameter("theta");
    src.rx(theta, 0);

    QuantumCircuit dst(1, 0);
    dst.append(src[0]);

    const auto symbols = dst.parameter_symbols();
    if (symbols != std::vector<std::string>({"theta"})) {
        std::cerr << "  to_qasm3_append_parameterized test : unexpected symbol list, got:";
        for (const auto &s : symbols) std::cerr << " " << s;
        std::cerr << std::endl;
        return EqualityError;
    }

    const auto actual = dst.to_qasm3();
    const std::string expected =
        "OPENQASM 3.0;\n"
        "include \"stdgates.inc\";\n"
        "input float[64] theta;\n"
        "qubit[1] q;\n"
        "rx(theta) q[0];\n";
    if (actual != expected) {
        std::cerr << "  to_qasm3_append_parameterized test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }

    return Ok;
}

#if QISKIT_CPP_HAS_QPY
// The QPY file header is big-endian and laid out as the b"QISKIT" magic
// string, a format version byte, the 3-byte Qiskit version that wrote it, and
// the number of serialized programs as a u64. See the QPY specification for the
// rest of the layout.
static const char qpy_magic[] = {'Q', 'I', 'S', 'K', 'I', 'T'};
static const std::size_t qpy_version_offset = sizeof(qpy_magic);
static const std::size_t qpy_num_programs_offset = qpy_version_offset + 1 + 3;
static const std::size_t qpy_header_size = qpy_num_programs_offset + 8;

static int check_qpy_header(const char *test, const std::vector<std::uint8_t> &payload,
                            const std::uint8_t expected_version,
                            const std::uint64_t expected_num_circuits) {
    if (payload.size() < qpy_header_size) {
        std::cerr << "  " << test << " test : payload is too short to hold a QPY header ("
            << payload.size() << " bytes)" << std::endl;
        return EqualityError;
    }
    for (std::size_t i = 0; i < sizeof(qpy_magic); i++) {
        if (payload[i] != static_cast<std::uint8_t>(qpy_magic[i])) {
            std::cerr << "  " << test << " test : payload does not start with the QPY magic"
                << std::endl;
            return EqualityError;
        }
    }
    const std::uint8_t version = payload[qpy_version_offset];
    if (expected_version != 0 && version != expected_version) {
        std::cerr << "  " << test << " test : QPY version " << static_cast<unsigned>(expected_version)
            << " != " << static_cast<unsigned>(version) << std::endl;
        return EqualityError;
    }
    if (version < QuantumCircuit::qpy_write_min_version()) {
        std::cerr << "  " << test << " test : QPY version " << static_cast<unsigned>(version)
            << " is below the writable minimum "
            << static_cast<unsigned>(QuantumCircuit::qpy_write_min_version()) << std::endl;
        return EqualityError;
    }

    std::uint64_t num_programs = 0;
    for (std::size_t i = 0; i < 8; i++) {
        num_programs = (num_programs << 8) | payload[qpy_num_programs_offset + i];
    }
    if (num_programs != expected_num_circuits) {
        std::cerr << "  " << test << " test : QPY holds " << num_programs << " circuits, expected "
            << expected_num_circuits << std::endl;
        return EqualityError;
    }
    return Ok;
}

static QuantumCircuit make_bell_circuit(void) {
    auto circ = QuantumCircuit(2, 2);
    circ.h(0);
    circ.cx(0, 1);
    circ.measure(0, 0);
    circ.measure(1, 1);
    return circ;
}

static int test_to_qpy(void) {
    auto circ = make_bell_circuit();
    const auto payload = circ.to_qpy();
    return check_qpy_header("to_qpy", payload, 0, 1);
}

static int test_to_qpy_with_version(void) {
    auto circ = make_bell_circuit();
    const std::uint8_t version = QuantumCircuit::qpy_write_min_version();
    const auto payload = circ.to_qpy(version);
    return check_qpy_header("to_qpy_with_version", payload, version, 1);
}

static int test_to_qpy_parameterized(void) {
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    // QPY carries symbolic parameters, unlike the OpenQASM 3 exporter which has
    // to recover their names from the parameter expressions.
    const auto payload = circ.to_qpy();
    return check_qpy_header("to_qpy_parameterized", payload, 0, 1);
}

static int test_to_qpy_multiple_circuits(void) {
    std::vector<QuantumCircuit> circuits;
    circuits.push_back(make_bell_circuit());
    circuits.push_back(QuantumCircuit(3, 3));

    const auto payload = QuantumCircuit::to_qpy(circuits);
    return check_qpy_header("to_qpy_multiple_circuits", payload, 0, 2);
}

static int test_to_qpy_empty_list(void) {
    std::vector<QuantumCircuit> circuits;
    const auto payload = QuantumCircuit::to_qpy(circuits);
    return check_qpy_header("to_qpy_empty_list", payload, 0, 0);
}

static int test_export_qpy(void) {
    auto circ = make_bell_circuit();
    const std::string filename = "test_export_qpy.qpy";
    std::remove(filename.c_str());

    circ.export_qpy(filename);

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file) {
        std::cerr << "  export_qpy test : \"" << filename << "\" was not created" << std::endl;
        return RuntimeError;
    }
    const std::vector<std::uint8_t> payload(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::remove(filename.c_str());

    const int result = check_qpy_header("export_qpy", payload, 0, 1);
    if (result != Ok) {
        return result;
    }

    // The file and the in-memory payload are produced by the same serializer,
    // so they must agree byte for byte.
    if (payload != circ.to_qpy()) {
        std::cerr << "  export_qpy test : file contents differ from to_qpy()" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_export_qpy_multiple_circuits(void) {
    std::vector<QuantumCircuit> circuits;
    circuits.push_back(make_bell_circuit());
    circuits.push_back(QuantumCircuit(3, 3));
    const std::string filename = "test_export_qpy_multi.qpy";
    std::remove(filename.c_str());

    QuantumCircuit::export_qpy(circuits, filename);

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file) {
        std::cerr << "  export_qpy_multiple_circuits test : \"" << filename
            << "\" was not created" << std::endl;
        return RuntimeError;
    }
    const std::vector<std::uint8_t> payload(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    std::remove(filename.c_str());

    return check_qpy_header("export_qpy_multiple_circuits", payload, 0, 2);
}

static int test_export_qpy_bad_path(void) {
    auto circ = make_bell_circuit();
    // A path whose parent directory does not exist cannot be written.
    const std::string filename = "qpy_missing_directory/circuit.qpy";

    try {
        circ.export_qpy(filename);
    } catch (const std::runtime_error &err) {
        const std::string message = err.what();
        if (message.find("QPY export failed") == std::string::npos) {
            std::cerr << "  export_qpy_bad_path test : unexpected error: " << message << std::endl;
            return EqualityError;
        }
        if (message.find(filename) == std::string::npos) {
            std::cerr << "  export_qpy_bad_path test : error does not name the path: "
                << message << std::endl;
            return EqualityError;
        }
        return Ok;
    }

    std::cerr << "  export_qpy_bad_path test : expected runtime_error" << std::endl;
    return RuntimeError;
}

static int test_from_qpy_round_trip(void) {
    auto circ = make_bell_circuit();
    const auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());

    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_round_trip test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }
    if (loaded[0].num_qubits() != circ.num_qubits()
        || loaded[0].num_clbits() != circ.num_clbits()) {
        std::cerr << "  from_qpy_round_trip test : loaded " << loaded[0].num_qubits()
            << " qubits / " << loaded[0].num_clbits() << " clbits, expected "
            << circ.num_qubits() << " / " << circ.num_clbits() << std::endl;
        return EqualityError;
    }
    // operator== compares the instruction sequence through the C-API.
    if (!(loaded[0] == circ)) {
        std::cerr << "  from_qpy_round_trip test : loaded circuit differs from the original"
            << std::endl;
        return EqualityError;
    }
    // Re-exporting the imported circuit must reproduce the payload, which only
    // holds if the import preserved everything the exporter writes.
    QuantumCircuit reloaded = loaded[0];
    if (reloaded.to_qpy() != circ.to_qpy()) {
        std::cerr << "  from_qpy_round_trip test : re-exported payload differs" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_from_qpy_registers(void) {
    auto qr0 = QuantumRegister(2, std::string("alpha"));
    auto qr1 = QuantumRegister(3, std::string("beta"));
    auto cr0 = ClassicalRegister(2, std::string("gamma"));
    auto cr1 = ClassicalRegister(3, std::string("delta"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qr0, qr1}),
                        std::vector<ClassicalRegister>({cr0, cr1}));
    circ.h(0);
    circ.cx(0, 4);

    const auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());
    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_registers test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }
    const QuantumCircuit &imported = loaded[0];

    if (imported.num_qregs() != 2 || imported.num_cregs() != 2) {
        std::cerr << "  from_qpy_registers test : loaded " << imported.num_qregs()
            << " qregs / " << imported.num_cregs() << " cregs, expected 2 / 2" << std::endl;
        return EqualityError;
    }

    const char *expected_qregs[] = {"alpha", "beta"};
    const uint_t expected_qsizes[] = {2, 3};
    const uint_t expected_qbases[] = {0, 2};
    for (uint_t i = 0; i < 2; i++) {
        const auto &reg = imported.qregs()[i];
        if (reg.name() != expected_qregs[i] || reg.size() != expected_qsizes[i]
            || reg.base_index() != expected_qbases[i]) {
            std::cerr << "  from_qpy_registers test : qreg " << i << " is \"" << reg.name()
                << "\" size " << reg.size() << " base " << reg.base_index()
                << ", expected \"" << expected_qregs[i] << "\" size " << expected_qsizes[i]
                << " base " << expected_qbases[i] << std::endl;
            return EqualityError;
        }
    }

    const char *expected_cregs[] = {"gamma", "delta"};
    const uint_t expected_csizes[] = {2, 3};
    const uint_t expected_cbases[] = {0, 2};
    for (uint_t i = 0; i < 2; i++) {
        const auto &reg = imported.cregs()[i];
        if (reg.name() != expected_cregs[i] || reg.size() != expected_csizes[i]
            || reg.base_index() != expected_cbases[i]) {
            std::cerr << "  from_qpy_registers test : creg " << i << " is \"" << reg.name()
                << "\" size " << reg.size() << " base " << reg.base_index()
                << ", expected \"" << expected_cregs[i] << "\" size " << expected_csizes[i]
                << " base " << expected_cbases[i] << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_from_qpy_register_bits_addressable(void) {
    // A register recovered from QPY must address the same circuit bits as the
    // original. Bit::global_index() resolves through the register a bit points
    // at, so this covers both the recovered base index and the bit wiring.
    auto qr0 = QuantumRegister(2, std::string("a"));
    auto qr1 = QuantumRegister(2, std::string("b"));
    auto cr0 = ClassicalRegister(2, std::string("c"));
    auto cr1 = ClassicalRegister(2, std::string("d"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qr0, qr1}),
                        std::vector<ClassicalRegister>({cr0, cr1}));
    circ.h(0);

    auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());
    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_register_bits_addressable test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }

    std::vector<QuantumRegister> qregs = loaded[0].qregs();
    std::vector<ClassicalRegister> cregs = loaded[0].cregs();
    if (qregs.size() != 2 || cregs.size() != 2) {
        std::cerr << "  from_qpy_register_bits_addressable test : expected 2 qregs and 2 cregs"
            << std::endl;
        return EqualityError;
    }
    // The second register of each kind starts at circuit bit 2.
    for (uint_t i = 0; i < qregs[1].size(); i++) {
        const uint32_t qubit = static_cast<uint32_t>(qregs[1][i]);
        const uint32_t clbit = static_cast<uint32_t>(cregs[1][i]);
        if (qubit != 2 + i || clbit != 2 + i) {
            std::cerr << "  from_qpy_register_bits_addressable test : bit " << i
                << " of the second registers maps to qubit " << qubit << " / clbit " << clbit
                << ", expected " << (2 + i) << " for both" << std::endl;
            return EqualityError;
        }
    }

    // Using the recovered registers to build on the imported circuit must
    // target those same bits.
    QuantumCircuit imported = loaded[0];
    imported.measure(qregs[1], cregs[1]);
    const auto &measured = imported.get_measure_map();
    if (measured.size() != 2) {
        std::cerr << "  from_qpy_register_bits_addressable test : recorded " << measured.size()
            << " measurements, expected 2" << std::endl;
        return EqualityError;
    }
    for (uint_t i = 0; i < 2; i++) {
        if (measured[i].first != 2 + i || measured[i].second != 2 + i) {
            std::cerr << "  from_qpy_register_bits_addressable test : measurement " << i
                << " is (" << measured[i].first << ", " << measured[i].second
                << "), expected (" << (2 + i) << ", " << (2 + i) << ")" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_from_qpy_global_phase(void) {
    auto circ = QuantumCircuit(1, 0, 0.5);
    circ.h(0);

    const auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());
    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_global_phase test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }
    // operator== compares the cached global phase as well as the instructions.
    if (!(loaded[0] == circ)) {
        std::cerr << "  from_qpy_global_phase test : loaded circuit differs from the original"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_from_qpy_measure_map(void) {
    auto circ = make_bell_circuit();
    const auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());
    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_measure_map test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }
    if (!loaded[0].has_measurements()) {
        std::cerr << "  from_qpy_measure_map test : measurements were not recovered" << std::endl;
        return EqualityError;
    }
    if (loaded[0].get_measure_map() != circ.get_measure_map()) {
        std::cerr << "  from_qpy_measure_map test : measure map differs from the original"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_from_qpy_parameterized(void) {
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    const auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());
    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_parameterized test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }
    if (loaded[0].num_parameters() != circ.num_parameters()) {
        std::cerr << "  from_qpy_parameterized test : loaded " << loaded[0].num_parameters()
            << " parameters, expected " << circ.num_parameters() << std::endl;
        return EqualityError;
    }
    // Unlike the OpenQASM 3 exporter, QPY preserves the symbol itself.
    const auto symbols = loaded[0].parameter_symbols();
    if (symbols.size() != 1 || symbols[0] != "theta") {
        std::cerr << "  from_qpy_parameterized test : parameter symbols were not recovered"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_from_qpy_multiple_circuits(void) {
    std::vector<QuantumCircuit> circuits;
    circuits.push_back(make_bell_circuit());
    circuits.push_back(QuantumCircuit(3, 3));

    const auto loaded = QuantumCircuit::from_qpy(QuantumCircuit::to_qpy(circuits));
    if (loaded.size() != 2) {
        std::cerr << "  from_qpy_multiple_circuits test : loaded " << loaded.size()
            << " circuits, expected 2" << std::endl;
        return EqualityError;
    }
    // The payload order must be preserved.
    for (std::size_t i = 0; i < circuits.size(); i++) {
        if (!(loaded[i] == circuits[i])) {
            std::cerr << "  from_qpy_multiple_circuits test : circuit " << i
                << " differs from the original" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_import_qpy_round_trip(void) {
    auto circ = make_bell_circuit();
    const std::string filename = "test_import_qpy.qpy";
    std::remove(filename.c_str());

    circ.export_qpy(filename);
    const auto loaded = QuantumCircuit::import_qpy(filename);
    std::remove(filename.c_str());

    if (loaded.size() != 1) {
        std::cerr << "  import_qpy_round_trip test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }
    if (!(loaded[0] == circ)) {
        std::cerr << "  import_qpy_round_trip test : loaded circuit differs from the original"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_import_qpy_missing_file(void) {
    const std::string filename = "qpy_missing_directory/absent.qpy";

    try {
        QuantumCircuit::import_qpy(filename);
    } catch (const std::runtime_error &err) {
        const std::string message = err.what();
        if (message.find("QPY import failed") == std::string::npos) {
            std::cerr << "  import_qpy_missing_file test : unexpected error: " << message
                << std::endl;
            return EqualityError;
        }
        if (message.find(filename) == std::string::npos) {
            std::cerr << "  import_qpy_missing_file test : error does not name the path: "
                << message << std::endl;
            return EqualityError;
        }
        return Ok;
    }

    std::cerr << "  import_qpy_missing_file test : expected runtime_error" << std::endl;
    return RuntimeError;
}

static int test_from_qpy_invalid_payload(void) {
    const std::vector<std::uint8_t> garbage(32, 0x7f);

    try {
        QuantumCircuit::from_qpy(garbage);
    } catch (const std::runtime_error &err) {
        if (std::string(err.what()).find("QPY import failed") == std::string::npos) {
            std::cerr << "  from_qpy_invalid_payload test : unexpected error: " << err.what()
                << std::endl;
            return EqualityError;
        }
        return Ok;
    }

    std::cerr << "  from_qpy_invalid_payload test : expected runtime_error" << std::endl;
    return RuntimeError;
}

static int test_from_qpy_empty_payload(void) {
    const std::vector<std::uint8_t> empty;

    try {
        QuantumCircuit::from_qpy(empty);
    } catch (const std::runtime_error &err) {
        if (std::string(err.what()).find("QPY import failed") == std::string::npos) {
            std::cerr << "  from_qpy_empty_payload test : unexpected error: " << err.what()
                << std::endl;
            return EqualityError;
        }
        return Ok;
    }

    std::cerr << "  from_qpy_empty_payload test : expected runtime_error" << std::endl;
    return RuntimeError;
}

static int test_from_qpy_no_circuits(void) {
    // A payload holding zero circuits is valid QPY and must import as an empty
    // list rather than an error.
    std::vector<QuantumCircuit> none;
    const auto loaded = QuantumCircuit::from_qpy(QuantumCircuit::to_qpy(none));
    if (!loaded.empty()) {
        std::cerr << "  from_qpy_no_circuits test : loaded " << loaded.size()
            << " circuits, expected 0" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_from_qpy_qasm3_agrees(void) {
    // The imported circuit must be usable by the rest of qiskit-cpp, not just
    // hold the right C-API handle.
    auto circ = make_bell_circuit();
    const auto loaded = QuantumCircuit::from_qpy(circ.to_qpy());
    if (loaded.size() != 1) {
        std::cerr << "  from_qpy_qasm3_agrees test : loaded " << loaded.size()
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }

    QuantumCircuit imported = loaded[0];
    const auto actual = imported.to_qasm3();
    const auto expected = circ.to_qasm3();
    if (actual != expected) {
        std::cerr << "  from_qpy_qasm3_agrees test : \n    expected:\n" << expected
            << "\n    actual:\n" << actual << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_qpy_min_versions(void) {
    // Writing is only supported for formats at least as new as the oldest
    // readable one, so the write floor cannot be below the read floor.
    if (QuantumCircuit::qpy_write_min_version() < QuantumCircuit::qpy_read_min_version()) {
        std::cerr << "  qpy_min_versions test : write minimum "
            << static_cast<unsigned>(QuantumCircuit::qpy_write_min_version())
            << " is below read minimum "
            << static_cast<unsigned>(QuantumCircuit::qpy_read_min_version()) << std::endl;
        return EqualityError;
    }
    return Ok;
}
#endif // QISKIT_CPP_HAS_QPY

#if defined(_WIN32)
int test_circuit(int argc, char** const argv) {
#else
int test_circuit(int argc, char** argv) {
#endif
    int num_failed = 0;
    num_failed += RUN_TEST(test_standard_gates);
    num_failed += RUN_TEST(test_measure);
    num_failed += RUN_TEST(test_append);
    num_failed += RUN_TEST(test_compose);
    num_failed += RUN_TEST(test_delay);
    num_failed += RUN_TEST(test_delay_all_qubits);
    num_failed += RUN_TEST(test_delay_register);
    num_failed += RUN_TEST(test_delay_unit_names);
    num_failed += RUN_TEST(test_delay_invalid_duration);
    num_failed += RUN_TEST(test_delay_append);
    num_failed += RUN_TEST(test_delay_compose);
    num_failed += RUN_TEST(test_to_qasm3_delay);
    num_failed += RUN_TEST(test_to_qasm3_delay_picoseconds);
    num_failed += RUN_TEST(test_to_qasm3_delay_with_parameter);
    num_failed += RUN_TEST(test_to_qasm3_multi_regs);
    num_failed += RUN_TEST(test_parameter_symbols);
    num_failed += RUN_TEST(test_parameter_symbols_unsupported_name);
    num_failed += RUN_TEST(test_parameter_symbols_duplicate_name_limitation);
    num_failed += RUN_TEST(test_to_qasm3_compose_parameterized);
    num_failed += RUN_TEST(test_to_qasm3_append_parameterized);
    num_failed += RUN_TEST(test_to_qasm3_parameterized);
    num_failed += RUN_TEST(test_to_qasm3_parameter_name_conflicts);
    num_failed += RUN_TEST(test_to_qasm3_stdgate_name_conflicts);
    num_failed += RUN_TEST(test_to_qasm3_keyword_and_constant_name_conflicts);
    num_failed += RUN_TEST(test_to_qasm3_classical_register_name_conflicts);
    num_failed += RUN_TEST(test_to_qasm3_custom_gate_name_conflicts);
    num_failed += RUN_TEST(test_to_qasm3_cu1_definition);
    num_failed += RUN_TEST(test_to_qasm3_cu3_definition);
    num_failed += RUN_TEST(test_to_qasm3_global_phase);
    num_failed += RUN_TEST(test_to_qasm3_global_phase_zero);
    num_failed += RUN_TEST(test_to_qasm3_global_phase_setter);
    num_failed += RUN_TEST(test_to_qasm3_global_phase_parameter);
    num_failed += RUN_TEST(test_to_qasm3_global_phase_gate);
    num_failed += RUN_TEST(test_to_qasm3_physical_qubits);
#if QISKIT_CPP_HAS_QPY
    num_failed += RUN_TEST(test_to_qpy);
    num_failed += RUN_TEST(test_to_qpy_with_version);
    num_failed += RUN_TEST(test_to_qpy_parameterized);
    num_failed += RUN_TEST(test_to_qpy_multiple_circuits);
    num_failed += RUN_TEST(test_to_qpy_empty_list);
    num_failed += RUN_TEST(test_export_qpy);
    num_failed += RUN_TEST(test_export_qpy_multiple_circuits);
    num_failed += RUN_TEST(test_export_qpy_bad_path);
    num_failed += RUN_TEST(test_qpy_min_versions);
    num_failed += RUN_TEST(test_from_qpy_round_trip);
    num_failed += RUN_TEST(test_from_qpy_registers);
    num_failed += RUN_TEST(test_from_qpy_register_bits_addressable);
    num_failed += RUN_TEST(test_from_qpy_global_phase);
    num_failed += RUN_TEST(test_from_qpy_measure_map);
    num_failed += RUN_TEST(test_from_qpy_parameterized);
    num_failed += RUN_TEST(test_from_qpy_multiple_circuits);
    num_failed += RUN_TEST(test_import_qpy_round_trip);
    num_failed += RUN_TEST(test_import_qpy_missing_file);
    num_failed += RUN_TEST(test_from_qpy_invalid_payload);
    num_failed += RUN_TEST(test_from_qpy_empty_payload);
    num_failed += RUN_TEST(test_from_qpy_no_circuits);
    num_failed += RUN_TEST(test_from_qpy_qasm3_agrees);
#endif

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
