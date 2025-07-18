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

* [API Docs](https://miniature-chainsaw-lmg9qq7.pages.github.io/qiskit_cxx/index.html)
* [Getting Started](#getting-started)
* [Contributing](#contributing)
* [License](#license)

## Getting Started

### Preparing Qiskit and Qiskit C extension

Before building Qiskit C++, install Qiskit 2.1 or later and build Qiskit C extension library https://github.com/Qiskit/qiskit/tree/main/crates/cext

```shell-session
$ git clone git@github.com:Qiskit/qiskit.git
$ cd qiskit
$ make c
```

### Building Qiskit C++

Qiskit C++ only has C++ header files. There is nothing to do to build the SDK.

### Running examples

Examples are available in [this directory](./test).

#### Building examples

To build application using Qiskit C++, add path to Qiskit C-API, include path to `qiskit.h` (installed in `dist/c/include`) and library `libqiskit.so` (installed in `dist/c/lib`) or `qiskit_cext.dll.lib` (generated in `target/release`) for Windows.
See ./CMakeLists.txt for example to make test program.

```shell-session
$ mkdir build
$ cd build
$ cmake -DPath_to_qiskit ..
$ make
```

## Contributing

Regardless if you are part of the core team or an external contributor, welcome and thank you for contributing to Qiskit C++

### Solving linting/format issues

Contributor must execute the commands below and fix any issues before submitting Pull Request.

### Running unit test

Contributor must execute the command below and fix any issues before submitting Pull Request.

### Checking API document

Contributor can generate API document from source.
```shell-session
$ doxygen Doxyfile
```
API document will be created under `html` directory.


## License

[Apache License 2.0](https://github.com/Qiskit/qiskit-cpp/blob/main/LICENSE.txt)
