cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0091 NEW)
project(ringo_ac LANGUAGES CXX)

# km/um se construyen con el toolchain de Windows (clang-cl + WDK/SDK).
# El server se construye de forma nativa en Linux.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(RINGO_CROSSCOMPILING TRUE)
else()
    set(RINGO_CROSSCOMPILING FALSE)
endif()

option(RINGO_BUILD_KM "Build the kernel-mode driver" ${RINGO_CROSSCOMPILING})
option(RINGO_BUILD_UM "Build the user-mode application" ${RINGO_CROSSCOMPILING})

if(NOT RINGO_CROSSCOMPILING)
    set(RINGO_BUILD_SERVER_DEFAULT ON)
else()
    set(RINGO_BUILD_SERVER_DEFAULT OFF)
endif()
set(RINGO_BUILD_SERVER ${RINGO_BUILD_SERVER_DEFAULT} CACHE BOOL "Build the server")

if(RINGO_BUILD_KM)
    add_subdirectory(km)
endif()

if(RINGO_BUILD_UM)
    add_subdirectory(um)
endif()

if(RINGO_BUILD_SERVER)
    add_subdirectory(server)
endif()
