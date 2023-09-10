cmake_minimum_required(VERSION 3.5)
project(tdoku VERSION 1.0)
set(CMAKE_CXX_STANDARD 14)

set(ARGS "" CACHE STRING "Additional compiler args")
# e.g., cmake . -DOPT=O2
set(OPT "O3" CACHE STRING "Optimization level")
# e.g., cmake . -DARCH=sandybridge
set(ARCH "native" CACHE STRING "Target architecture")
# these options specify maximum SSE level and override ARCH
option(SSE2        "Compile with SSE2 support"         OFF)
option(SSSE3       "Compile with SSSE3 support"        OFF)
option(SSE4_1      "Compile with SSE4.1 support"       OFF)
option(SSE4_2      "Compile with SSE4.2 support"       OFF)
option(AVX         "Compile with AVX support"          OFF)
option(AVX2        "Compile with AVX2 support"         OFF)
option(AVX512      "Compile with AVX512BITALG support" OFF)

get_filename_component(CCOMPILER "$ENV{CC}" NAME)
if(EXISTS "${CMAKE_SOURCE_DIR}/other/module_rust_sudoku/${CCOMPILER}/libsudoku.so")
    set(HAVE_RUST_SUDOKU_LIB ON)
else()
    set(HAVE_RUST_SUDOKU_LIB OFF)
endif()

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -${OPT} ${ARGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -${OPT} ${ARGS}")

if(AVX512)
    set(ArchFlags "-mavx512vl -mavx512bitalg")
elseif(AVX2)
    set(ArchFlags "-mavx2")
elseif(AVX)
    set(ArchFlags "-mavx")
elseif(SSE4_2)
    set(ArchFlags "-msse4.2")
elseif(SSE4_1)
    set(ArchFlags "-msse4.1")
elseif(SSSE3)
    set(ArchFlags "-mssse3")
elseif(SSE2)
    set(ArchFlags "-msse2")
else()
    set(ArchFlags "-march=${ARCH}")
endif()

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${ArchFlags}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ArchFlags}")

configure_file (
    "${CMAKE_SOURCE_DIR}/src/build_info.h.in"
    "${CMAKE_SOURCE_DIR}/src/build_info.h")

# a gcc-linkable library with just the fast solver
add_library(tdoku_object OBJECT src/solver_dpll_triad_simd.cc src/util.cc src/generate.cc src/solve.cc)
target_compile_options(tdoku_object PUBLIC -fno-exceptions -fno-rtti -fpic)

add_library(tdoku_static STATIC $<TARGET_OBJECTS:tdoku_object>)
add_library(tdoku_shared SHARED $<TARGET_OBJECTS:tdoku_object>)

set(BENCHMARK_SOLVER_SOURCES
        src/solver_dpll_triad_simd.cc)

add_executable(run_tests test/run_tests.cc src/util.cc ${BENCHMARK_SOLVER_SOURCES})
#add_executable(generate src/generate.cc src/util.cc ${GENERATE_SOLVER_SOURCES})

add_library(grid_lib STATIC src/grid_lib.cc)
target_compile_options(grid_lib PUBLIC -fno-exceptions -fno-rtti -fpic)
target_include_directories(grid_lib PUBLIC include)
target_link_libraries(grid_lib tdoku_static)

add_executable(grid_tools src/grid_tools.cc)
target_include_directories(grid_tools PUBLIC include)
target_link_libraries(grid_tools grid_lib)
target_link_libraries(grid_tools tdoku_static)
