> [!WARNING]
> This code is experimental and its API may change without prior warning. Use it at your own risk.

# Qiskit C++

Qiskit C++ provides a modern C++ (version C++ 11 or later) interface of Qiskit for circuit building, transpilation, sampler jobs, and QRMI-backed estimator jobs, as same as Qiskit Python interfaces.
This interface is based on Qiskit C-API introduced in Qiskit 2.1.

## Supported OS

* Linux
  * AlmaLinux 9
  * Amazon Linux 2023
  * RedHat Enterprise Linux 8
  * RedHat Enterprise Linux 9
  * SuSE 15
  * Ubuntu 22.04
  * Ubuntu 24.04

* macOS
  * Sequoia 15.1 or above

* Windows


## Jump To:

* [API Docs](https://quantum.cloud.ibm.com/docs/en/api/qiskit-c)
* [Getting Started](#getting-started)
* [Contributing](#contributing)
* [License](#license)

## Getting Started

### Preparing Qiskit and Qiskit C extension

Before building Qiskit C++, install Qiskit 2.2 or later and build Qiskit C extension library https://github.com/Qiskit/qiskit/tree/main/crates/cext

```shell-session
$ git clone git@github.com:Qiskit/qiskit.git
$ cd qiskit
$ make c
```

### Preparing Interface API to Quantum hardware

Qiskit C++ requires one of the following APIs to access IBM Quantum Platform to run the quantum circuits.

- qiskit-ibm-runtime C (https://github.com/Qiskit/qiskit-ibm-runtime-c)
- QRMI (https://github.com/qiskit-community/qrmi)
- SQC 0.10.0 (https://github.com/jhpc-quantum/SQC)

Before building your application with Qiskit C++, build one of these APIs.


If you want to use QRMI, build as following.

```shell-session
$ git clone git@github.com:qiskit-community/qrmi.git
$ cd qrmi
$ cargo build --release
```

If you want to use qiskit-ibm-runtime C, build as following.
(Note, qiskit-ibm-runtime C is an early prototype so build method may be changed)

```shell-session
$ git clone git@github.com:Qiskit/qiskit-ibm-runtime-c
$ cd qiskit-ibm-runtime-c
$ mkdir build
$ cd build
$ cmake ..
$ make
```

If you want to use [SQC C API](https://github.com/jhpc-quantum/SQC), follow [the SQC documentation](https://github.com/jhpc-quantum/documents/blob/main/SQC_JHPC_Quantum_user_guide.md).
Basically, SQC is pre-built and available in the environment, and thus you only need to set up authentication credentials (JWT token) and environment variables based on the provided scripts.

### Building Qiskit C++

Qiskit C++ only has C++ header files. There is nothing to do to build the SDK.

### Running examples

Examples are available in [this directory](./samples).

#### Building examples

To build application using Qiskit C++, add path to Qiskit C-API, include path to `qiskit.h` (installed in `dist/c/include`) and library `libqiskit.so` (installed in `dist/c/lib`) or `qiskit_cext.dll.lib` (generated in `target/release`) for Windows.
See ./samples/CMakeLists.txt for example to make test program.

You also need to link **zlib**. Primitive PUB circuits are submitted to Qiskit Runtime as zlib-compressed QPY, so serializing a circuit requires it. The sample and test CMake configurations locate it with `find_package(ZLIB REQUIRED)`.

```shell-session
$ cd samples
$ mkdir build
$ cd build
$ cmake -DQISKIT_ROOT=Path_to_qiskit ..
$ make
```

If you want to build sampler or transpiler example, you will need one of qiskit-ibm-runtime C or QRMI or SQC. The estimator example currently requires QRMI.

Then example can be built by setting `QISKIT_IBM_RUNTIME_C_ROOT` or `QRMI_ROOT` or `SQC_ROOT` to cmake.

```shell-session
$ cd samples
$ mkdir build
$ cd build
$ cmake -DQISKIT_ROOT=Path_to_qiskit -DQISKIT_IBM_RUNTIME_C_ROOT="path to qiskit-ibm-runtime C" or -DQRMI_ROOT="path to QRMI" or -DSQC_ROOT="path to SQC" ..
$ make
```

To run sampler example, set your account information in `$HOME/.qiskit/qiskit-ibm.json` (see https://github.com/Qiskit/qiskit-ibm-runtime?tab=readme-ov-file#save-your-account-on-disk) or setting following environment variables to access Quantum hardware.

```
QISKIT_IBM_TOKEN=<your API key>
QISKIT_IBM_INSTANCE=<your CRN>
```

The estimator interface is available through `primitives::BackendEstimatorV2` and currently requires QRMI or another backend that implements `BackendV2::run(std::vector<EstimatorPub>&, double)`. Estimator PUB circuits must be unmeasured and can use Pauli labels, Pauli-label maps, Pauli term vectors, or `quantum_info::SparseObservable` inputs. Empty observables are rejected before backend submission, and malformed estimator result cardinality is rejected while reading provider results. Estimator job cancellation is not currently supported, so `BackendEstimatorJob::cancel()` returns `false`. (`providers::QRMIJob::cancel()` does request cancellation from QRMI, but the primitive job wrappers do not yet call it.)

#### Parameterized circuits in primitive PUBs

A parameterized circuit is submitted with its parameter values, which the quantum computer binds at run time. Pass the values alongside the circuit; they must be given in the order the circuit's parameters are bound, one value per parameter:

```cpp
QuantumCircuit circ(1, 1);
auto theta = Parameter("theta");
circ.rx(theta, 0);
circ.measure(0, 0);

auto pub = SamplerPub(circ, std::vector<double>({0.5}), shots);
```

Several value sets can be submitted as one sweep, with the server running the circuit once per set:

```cpp
std::vector<std::vector<double>> sweep;
sweep.push_back(std::vector<double>({0.0}));
sweep.push_back(std::vector<double>({1.57}));
auto pub = SamplerPub(circ, sweep, shots);
```

The same applies to the estimator through `EstimatorPub::set_parameter_values()`.

Values are matched to parameters **by position**, not by name. Qiskit orders a circuit's parameters alphabetically by name, which is the order `QuantumCircuit::parameter_symbols()` returns — use that list to work out which position each value fills. Insertion order into the circuit does not determine it.

The number of values is checked against `QuantumCircuit::num_parameters()` when the values are set, again when the PUB is serialized, and once more by the QRMI backend before submission. A mismatch raises `std::invalid_argument` where the values were given, so it is reported before any job is sent. An unparameterized circuit needs no values.

#### Circuit serialization in primitive PUBs

Qiskit Runtime accepts a PUB circuit either as an OpenQASM 3 string or as a QPY payload. qiskit-cpp sends **QPY** by default when built against a Qiskit providing the QPY C-API (2.6.0 or later), because QPY round-trips the circuit losslessly, including symbolic parameters, while the OpenQASM 3 exporter has to recover parameter names from expression strings. The circuit is emitted as `{"__type__": "QuantumCircuit", "__value__": <base64 of zlib-compressed QPY>}`.

To send OpenQASM 3 instead for a given pub:

```cpp
auto pub = SamplerPub(circ, shots);
pub.set_serialization(CircuitSerialization::qasm3);
```

The QPY format version is pinned to 17, the only version both Qiskit's C-API (which will not write below 17) and Qiskit Runtime (which rejects above 17) accept.

#### QRMI resource types

The QRMI interfaces require **QRMI 0.13.0 or later**. QRMI 0.13 added the `ibm-quantum-system` and `pasqal-local` resource types, which renumbered the `QrmiResourceType` enum, so older QRMI releases are not supported.

`QiskitRuntimeService` connects through `QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE` by default. Call `set_resource_type()` to select another, and `set_url()` / `set_iam_url()` to point at a different endpoint:

| Resource type | Environment variable prefix |
| ---- | ---- |
| `QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE` | `<backend>_QRMI_IBM_QRS_` |
| `QRMI_RESOURCE_TYPE_IBM_QUANTUM_SYSTEM` | `<backend>_QRMI_IBM_QS_` |
| `QRMI_RESOURCE_TYPE_IBM_DIRECT_ACCESS` | `<backend>_QRMI_IBM_DA_` (deprecated by QRMI) |

The `ENDPOINT`, `IAM_ENDPOINT`, `IAM_APIKEY` and `SERVICE_CRN` variables are set for you from the account information above. `SESSION_MODE` is set only for Qiskit Runtime Service, which is the only type with a session concept. The AWS/S3 variables that IBM Quantum System uses to deliver results (`AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY`, `S3_ENDPOINT`, `S3_BUCKET`, `S3_REGION`) have no counterpart in `QiskitRuntimeService` and must be exported yourself. The Pasqal resource types are not supported through `QiskitRuntimeService`.

Jobs now run inside an acquired QRMI session, so `set_session_mode("batch")` or `set_session_mode("dedicated")` takes effect. The session is opened when the first job is submitted — constructing a backend only to transpile does not open one — and closed when the backend and all of its jobs are released. `QRMIBackend::acquire_session()` and `release_session()` control it explicitly if needed.

To run the sample example with SQC, you also need to set the library options to the environment variable `SQC_LIBS`, which is automatically set by a provided script (see [here](https://github.com/jhpc-quantum/documents/blob/main/SQC_JHPC_Quantum_user_guide.md)), before running cmake. In SQC 0.10.0, `backend_setup.sh` configures `SQC_LIBS` as follows:

```
SQC_LIBS="-lsqc_api -lsqc_rpc ... (omitted) ... -pthread"
```

### Tutorials

See [tutorial](tutorials/readme.md) for more examples.

### Making your own interface to Quantum hardware

Qiskit C++ offers an abstract interface to access Quantum hardware. You can make your own interface to the hardware
by overriding `providers::Job` and `providers::BackendV2` and make your own service provider to make backend object like
`service::QiskitRuntimeService` class.
Refer to `./src/providers/qrmi_backend.hpp` and `./src/providers/qrmi_job.hpp` for details.

## Contributing

Regardless if you are part of the core team or an external contributor, welcome and thank you for contributing to Qiskit C++

### Running unit test

Contributor must add test cases under ./test directory if you have added new features to Qiskit C++.
Also all the test cases running on the github repository should be passed.

### Checking API document

Contributor can generate API document from source.
```shell-session
$ doxygen Doxyfile
```
API document will be created under `html` directory.


## License

[Apache License 2.0](https://github.com/Qiskit/qiskit-cpp/blob/main/LICENSE.txt)
