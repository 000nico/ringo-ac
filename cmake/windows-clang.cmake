set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    RINGO_TOOLCHAIN_ROOT
    RINGO_WDK_ROOT
    RINGO_SDK_ROOT
    RINGO_CRT_ROOT
    RINGO_WDK_VERSION
)

set(CMAKE_C_COMPILER clang-cl CACHE FILEPATH "clang-cl compiler")
set(CMAKE_CXX_COMPILER clang-cl CACHE FILEPATH "clang-cl compiler")
set(CMAKE_LINKER lld-link CACHE FILEPATH "lld-link linker")
set(CMAKE_AR llvm-lib CACHE FILEPATH "llvm-lib librarian")

set(RINGO_TOOLCHAIN_ROOT "$ENV{RINGO_TOOLCHAIN_ROOT}" CACHE PATH
    "Directory containing the extracted Windows SDK and WDK")
if(NOT RINGO_TOOLCHAIN_ROOT)
    set(RINGO_TOOLCHAIN_ROOT "${CMAKE_SOURCE_DIR}/../toolchains" CACHE PATH
        "Directory containing the extracted Windows SDK and WDK")
endif()

set(RINGO_WDK_ROOT "${RINGO_TOOLCHAIN_ROOT}/wdk/c" CACHE PATH "WDK root")
set(RINGO_SDK_ROOT "${RINGO_TOOLCHAIN_ROOT}/xwin-splat/sdk" CACHE PATH
    "xwin Windows SDK root")
set(RINGO_CRT_ROOT "${RINGO_TOOLCHAIN_ROOT}/xwin-splat/crt" CACHE PATH
    "xwin MSVC CRT root")
set(RINGO_WDK_VERSION "10.0.28000.0" CACHE STRING "WDK version")
set(RINGO_SDK_VERSION "10.0.26100.0" CACHE STRING "Windows SDK version")

set(RINGO_WDK_INCLUDE "${RINGO_WDK_ROOT}/Include/${RINGO_WDK_VERSION}")
set(RINGO_SDK_INCLUDE "${RINGO_SDK_ROOT}/include")
set(RINGO_CRT_INCLUDE "${RINGO_CRT_ROOT}/include")
set(RINGO_WDK_KM_LIB "${RINGO_WDK_ROOT}/Lib/${RINGO_WDK_VERSION}/km/x64")
set(RINGO_SDK_UM_LIB "${RINGO_SDK_ROOT}/lib/um/x86_64")
set(RINGO_SDK_UCRT_LIB "${RINGO_SDK_ROOT}/lib/ucrt/x86_64")
set(RINGO_CRT_LIB "${RINGO_CRT_ROOT}/lib/x86_64")
set(RINGO_BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_C_FLAGS_INIT "/nologo")
set(CMAKE_CXX_FLAGS_INIT "/nologo /std:c++20")
set(CMAKE_EXE_LINKER_FLAGS_INIT "/machine:x64")

foreach(required_path
    RINGO_WDK_INCLUDE
    RINGO_SDK_INCLUDE
    RINGO_CRT_INCLUDE
    RINGO_WDK_KM_LIB
    RINGO_SDK_UM_LIB
    RINGO_SDK_UCRT_LIB
    RINGO_CRT_LIB)
    if(NOT EXISTS "${${required_path}}")
        message(FATAL_ERROR "${required_path} no existe: ${${required_path}}")
    endif()
endforeach()

set(RINGO_CASE_OVERLAY "${RINGO_TOOLCHAIN_ROOT}/case-overlay" CACHE PATH
    "Case-corrected include overlay for case-sensitive hosts")
