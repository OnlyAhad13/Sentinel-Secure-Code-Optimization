cmake_minimum_required(VERSION 3.15)
project(sentinel_core VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# C++ Standard and Compiler Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position-independent code for shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================================
# Build Options
# ============================================================================

option(SENTINEL_USE_AVX2 "Enable AVX2 SIMD optimizations" ON)
option(SENTINEL_BUILD_TESTS "Build test executables" OFF)

# ============================================================================
# Compiler Flags
# ============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    if(SENTINEL_USE_AVX2)
        # AVX2 and FMA for SIMD acceleration
        add_compile_options(-mavx2 -mfma)
    endif()
    
    # Optimize for native architecture in Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    endif()
    
elseif(MSVC)
    add_compile_options(/W4)
    
    if(SENTINEL_USE_AVX2)
        add_compile_options(/arch:AVX2)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    endif()
endif()

# ============================================================================
# Find pybind11
# ============================================================================

# Try to find pybind11 via Python first (most common case)
find_package(Python REQUIRED COMPONENTS Interpreter Development)
execute_process(
    COMMAND ${Python_EXECUTABLE} -m pybind11 --cmakedir
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE pybind11_RESULT
)

if(pybind11_RESULT EQUAL 0)
    set(pybind11_DIR ${pybind11_DIR})
    find_package(pybind11 CONFIG REQUIRED)
else()
    # Fallback: try system installation
    find_package(pybind11 CONFIG REQUIRED)
endif()

# ============================================================================
# Core Library (static, for internal use)
# ============================================================================

add_library(sentinel_core_lib STATIC
    core/src/vector_store.cpp
)

target_include_directories(sentinel_core_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

# ============================================================================
# Python Module
# ============================================================================

pybind11_add_module(sentinel_core
    bindings/bindings.cpp
)

target_link_libraries(sentinel_core PRIVATE sentinel_core_lib)

# ============================================================================
# Installation
# ============================================================================

# Install the Python module to the Python site-packages
install(TARGETS sentinel_core
    LIBRARY DESTINATION .
)

# ============================================================================
# Testing (Optional)
# ============================================================================

if(SENTINEL_BUILD_TESTS)
    enable_testing()
    
    add_executable(test_vector_store
        tests/test_vector_store_cpp.cpp
    )
    target_link_libraries(test_vector_store PRIVATE sentinel_core_lib)
    
    add_test(NAME VectorStoreTest COMMAND test_vector_store)
endif()

# ============================================================================
# Build Info
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS " Sentinel Core Configuration")
message(STATUS "========================================")
message(STATUS " C++ Standard:    C++${CMAKE_CXX_STANDARD}")
message(STATUS " AVX2 Enabled:    ${SENTINEL_USE_AVX2}")
message(STATUS " Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS " Python:          ${Python_EXECUTABLE}")
message(STATUS " pybind11:        ${pybind11_VERSION}")
message(STATUS "========================================")
message(STATUS "")
