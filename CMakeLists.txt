cmake_minimum_required(VERSION 3.3)

project(SwarmSynchro)

# Compiler Flags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static-libgcc -static-libstdc++")

# Output Directories
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Swarm Common Cmake
include(ExternalProject)
find_package(Git REQUIRED)
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(SwarmOptions)
include(SwarmAddDependency)
include(SwarmAddBoost)
include(SwarmDefinePackage)
include(SwarmDefineTest)


###############################################################
# PACKAGE CONFIGURATION
###############################################################

set (SYNCHRO_SOURCES
        src/engine/SwarmSynchro.h
        src/engine/synchro/lockable.cpp
)
SwarmAddPackageDependency(Common)
SwarmAddBoost(thread system chrono)
SwarmDefinePackage(Synchro 0.1 ${SYNCHRO_SOURCES})
SwarmDefineInstallFiles(
        src/engine/SwarmSynchro.h
)

# Package Test
include_directories(src/engine)
SwarmDefineTest(Synchro tst/engine/main_test.cpp)