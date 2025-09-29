# This code is part of Qiskit.
#
# (C) Copyright IBM 2017.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

DIR_TEST_BUILD = build

default:
	$(MAKE) -C deps/qiskit c

	# -S specifically specifies the source path to be the current folder
	# -B specifically specifies the build path to be inside test/c/build
	cmake -S. -B$(DIR_TEST_BUILD)
	cmake --build $(DIR_TEST_BUILD)
	# -V ensures we always produce a logging output to indicate the subtests
	# -C Debug is needed for windows to work, if you don't specify Debug (or
	#  release) explicitly ctest doesn't run on windows
	ctest -V -C Debug --test-dir $(DIR_TEST_BUILD)

clean:
	rm -rf $(DIR_TEST_BUILD)
