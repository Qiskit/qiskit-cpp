> [!WARNING]
> This code is experimental and its API may change without prior warning. Use it at your own risk.

# Qiskit C++

Qiskit C++ provides a modern C++ (version C++ 11 or later) interface of Qiskit for circuit building, (transpilation and returning samples of quantum circuit outputs ... to be added in the future release), as same as Qiskit Python interfaces.
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

```shell-session
$ cd samples
$ mkdir build
$ cd build
$ cmake -DQISKIT_ROOT=Path_to_qiskit ..
$ make
```

If you want to build sampler or transpiler example, you will need one of qiskit-ibm-runtime C or QRMI or SQC.

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

To run the sample example with SQC, you also need to set the library options to the environment variable `SQC_LIBS`, which is automatically set by a provided script (see [here](https://github.com/jhpc-quantum/documents/blob/main/SQC_JHPC_Quantum_user_guide.md)), before running cmake. In SQC 0.10.0, `backend_setup.sh` configures `SQC_LIBS` as follows:

```
SQC_LIBS="-lsqc_api -lsqc_rpc ... (omitted) ... -pthread"
```

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
