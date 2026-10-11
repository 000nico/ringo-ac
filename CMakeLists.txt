cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0091 NEW)
project(ringo_ac LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "ringo_ac se compila solo en Windows (MSVC + WDK). Usá scripts/build-all.ps1.")
endif()

option(RINGO_BUILD_DRIVER "Build the kernel-mode driver" ON)
option(RINGO_BUILD_CLIENT "Build the user-mode client" ON)
option(RINGO_BUILD_SERVER "Build the server" ON)

set(RINGO_BIN_DIR "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Directorio de salida de los binarios")
file(MAKE_DIRECTORY "${RINGO_BIN_DIR}")

# Permite que FindWDK encuentre un WDK bajado localmente por scripts/fetch-wdk.ps1.
if(NOT DEFINED ENV{WDKContentRoot})
    set(RINGO_LOCAL_WDK "${CMAKE_CURRENT_SOURCE_DIR}/toolchains/wdk/c")
    if(EXISTS "${RINGO_LOCAL_WDK}/Include")
        set(ENV{WDKContentRoot} "${RINGO_LOCAL_WDK}")
        message(STATUS "Usando WDK local: ${RINGO_LOCAL_WDK}")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Salida plana (sin subcarpeta por configuracion) para todos los generadores.
function(ringo_set_output_dir target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${RINGO_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${RINGO_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${RINGO_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${RINGO_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${RINGO_BIN_DIR}")
endfunction()

if(RINGO_BUILD_DRIVER)
    add_subdirectory(driver)
endif()

if(RINGO_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(RINGO_BUILD_SERVER)
    add_subdirectory(server)
endif()
