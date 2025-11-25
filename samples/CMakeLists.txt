cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 11)

project("qiskit-cpp" LANGUAGES CXX C)

set(TARGET qiskit-cpp-test)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(LIB_PATH debug)
else()
  set(LIB_PATH release)
endif()

set(QISKIT_CPP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)


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
        ${QRMI_ROOT}/target/release
        #${SQC_ROOT}/<TODO>
        ${QISKIT_IBM_RUNTIME_C_ROOT}/build/cargo/debug
    )
    if(QRMI_ROOT)
      target_link_libraries(${APP_NAME} qiskit_cext.dll.lib qrmi.dll.lib nlohmann_json::nlohmann_json)
      target_compile_options(${APP_NAME} PRIVATE "-DQRMI_ROOT=${QRMI_ROOT}")
    elseif(QISKIT_IBM_RUNTIME_C_ROOT)
      target_link_libraries(${APP_NAME} qiskit_cext.dll.lib qiskit_ibm_runtime.dll.lib nlohmann_json::nlohmann_json)
      set(CMAKE_CXX_FLAGS "-DQISKIT_IBM_RUNTIME_C_ROOT=${QISKIT_IBM_RUNTIME_C_ROOT}")
    #elseif(SQC_ROOT)
      #target_link_libraries(${APP_NAME} qiskit_cext.dll.lib <TODO> nlohmann_json::nlohmann_json)
      #set(CMAKE_CXX_FLAGS "-DSQC_ROOT=${SQC_ROOT}")
    else()
      target_link_libraries(${APP_NAME} qiskit_cext.dll.lib nlohmann_json::nlohmann_json)
      target_compile_options(${APP_NAME} PRIVATE "-DQISKIT_IBM_RUNTIME_C_ROOT=${QISKIT_IBM_RUNTIME_C_ROOT}")
    endif()
  else()
    if(QRMI_ROOT)
      target_link_libraries(${APP_NAME}
        PRIVATE
          "-L${QISKIT_ROOT}/dist/c/lib -L${QRMI_ROOT}/target/release -Wl,-rpath ${QISKIT_ROOT}/dist/c/lib -Wl,-rpath ${QRMI_ROOT}/target/release" qiskit qrmi nlohmann_json::nlohmann_json
      )
      target_compile_options(${APP_NAME} PRIVATE "-DQRMI_ROOT=${QRMI_ROOT}")
    elseif(QISKIT_IBM_RUNTIME_C_ROOT)
      target_link_libraries(${APP_NAME}
        PRIVATE
          "-L${QISKIT_ROOT}/dist/c/lib -L${QISKIT_IBM_RUNTIME_C_ROOT}/build/cargo/debug -Wl,-rpath ${QISKIT_ROOT}/dist/c/lib -Wl,-rpath ${QISKIT_IBM_RUNTIME_C_ROOT}/build/cargo/debug" qiskit qiskit_ibm_runtime nlohmann_json::nlohmann_json
      )
      target_compile_options(${APP_NAME} PRIVATE "-DQISKIT_IBM_RUNTIME_C_ROOT=${QISKIT_IBM_RUNTIME_C_ROOT}")
    #elseif(SQC_ROOT)
    #  target_link_libraries(${APP_NAME}
    #    PRIVATE
    #      "-L${QISKIT_ROOT}/dist/c/lib -L${SQC_ROOT}/<TODO> -Wl,-rpath ${QISKIT_ROOT}/dist/c/lib -Wl,-rpath ${QISKIT_IBM_RUNTIME_C_ROOT}/build/cargo/debug" qiskit qiskit_ibm_runtime nlohmann_json::nlohmann_json
    #  )
    #  target_compile_options(${APP_NAME} PRIVATE "-DSQC_ROOT=${SQC_ROOT}")
    else()
      target_link_libraries(${APP_NAME}
        PRIVATE
          "-L${QISKIT_ROOT}/dist/c/lib -Wl,-rpath ${QISKIT_ROOT}/dist/c/lib" qiskit nlohmann_json::nlohmann_json
      )
    endif()
  endif()

  # path to headerfile
  target_include_directories(${APP_NAME}
    PRIVATE
      ${QISKIT_ROOT}/dist/c/include
      ${QRMI_ROOT}
      ${QISKIT_IBM_RUNTIME_C_ROOT}/include
      ${SQC_ROOT}/capi/include
      ${SAMPLES_PATH}
      ${CMAKE_CURRENT_SOURCE_DIR}/../src
      nlohmann_json::nlohmann_json
  )

  install (TARGETS ${APP_NAME} DESTINATION .)
endfunction()

function(add_application_test TEST_NAME APP_NAME CPP_FILE)
  add_application(${APP_NAME} ${CPP_FILE})
  add_test(NAME ${TEST_NAME} COMMAND ${APP_NAME})
endfunction()

add_application(circuit_test circuit_test.cpp)
add_application(observable_test observable_test.cpp)

if(QRMI_ROOT OR QISKIT_IBM_RUNTIME_C_ROOT)
  add_application(sampler_test sampler_test.cpp)
  add_application(transpile_test transpile_test.cpp)
endif()

if(SQC_ROOT)
  add_application(sqc_test sqc_test.cpp)
endif()
