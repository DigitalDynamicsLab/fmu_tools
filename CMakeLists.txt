cmake_minimum_required(VERSION 3.21)

cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction i.e. CMAKE_MSVC_RUNTIME_LIBRARY

project(fmu_tools)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# Set the minimum required C++ standard to C++14, allow C++17 if available
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

mark_as_advanced(FORCE CMAKE_INSTALL_PREFIX)

# -------------------------------------------------

add_subdirectory(FMI2/demos/cosimulation)
