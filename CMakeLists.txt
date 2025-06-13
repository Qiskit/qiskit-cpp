cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project("qiskit-cpp" LANGUAGES CXX C)

set(TARGET qiskit-cpp-test)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(LIB_PATH debug)
else()
  set(LIB_PATH release)
endif()

set(QISKIT_CPP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)
#set(QISKIT_CPP_RUST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/crates/qc_rust)
# set(QC_TRANSPILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/crates/qc_transpile)
#set(QISKIT_CPP_STATEVECTOR_SAMPLER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/crates/qc_statevector)
#set(SAMPLES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/samples/)
#set(EXTENSIONS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/demo/)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

function(add_application APP_NAME CPP_FILE)
  # source files for the client app
  set(SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CPP_FILE})

  add_executable(${APP_NAME} ${SRC_FILE})
  set_target_properties(${APP_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

  message("   Qiskit Root = ${QISKIT_ROOT}")
  # add libraries
  if (MSVC)
    target_link_directories(${APP_NAME}
      PUBLIC
        ${QISKIT_ROOT}/target/release
    )
    target_link_libraries(${APP_NAME} qiskit_cext.dll.lib)
  else()
    target_link_libraries(${APP_NAME}
      PUBLIC
        "-L${QISKIT_ROOT}/dist/c/lib -Wl,-rpath ${QISKIT_ROOT}/dist/c/lib" qiskit
    )
  endif()

  # path to headerfile
  target_include_directories(${APP_NAME}
    PRIVATE
      ${QISKIT_ROOT}/dist/c/include
      ${SAMPLES_PATH}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  install (TARGETS ${APP_NAME} DESTINATION .)
endfunction()

function(add_application_test TEST_NAME APP_NAME CPP_FILE)
  add_application(${APP_NAME} ${CPP_FILE})
  target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/external)
  add_test(NAME ${TEST_NAME} COMMAND ${APP_NAME})
endfunction()

add_application(circuit_test tests/circuit_test.cpp)
