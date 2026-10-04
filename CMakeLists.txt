cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0091 NEW)
project(ringo_ac LANGUAGES CXX)

option(RINGO_BUILD_KM "Build the kernel-mode driver" ON)
option(RINGO_BUILD_UM "Build the user-mode application" ON)

if(RINGO_BUILD_KM)
    add_subdirectory(km)
endif()

if(RINGO_BUILD_UM)
    add_subdirectory(um)
endif()
