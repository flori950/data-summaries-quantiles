cmake_minimum_required(VERSION 3.20)
project(Data_Summaries CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to an optimized build if the caller did not pick one.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Dependencies:
#   - Threads (required): std::thread parallelizes DDSketch ingest
#   - TBB (optional): backend for the parallel std::sort in ExactQuantiles.
#     Without it (e.g. Apple libc++), the code falls back to a serial sort.
find_package(Threads REQUIRED)
find_package(TBB QUIET)

# Library with the core data structures, shared by the benchmark and the tests.
add_library(data_summaries_core STATIC
        ddsketch/ddsketch.cpp
        ddsketch/dense_store.cpp
        ddsketch/logarithmic_mapping.cpp
        exact_quantiles/exact_quantiles.cpp
        histogram_sketch/histogram_sketch.cpp
        tdigest/tdigest.cpp
        kll/kll.cpp)

target_include_directories(data_summaries_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(data_summaries_core PUBLIC Threads::Threads)
if(TBB_FOUND)
    target_link_libraries(data_summaries_core PUBLIC TBB::tbb)
    message(STATUS "TBB found - ExactQuantiles will use the parallel sort")
else()
    message(STATUS "TBB not found - ExactQuantiles falls back to a serial sort")
endif()

add_executable(Data_Summaries main.cpp)
target_link_libraries(Data_Summaries PRIVATE data_summaries_core)

# Aggressive optimization for the benchmark, scoped to this target only.
# NOTE: -march=native/-mtune=native tune for the *build* machine; drop them for
# portable binaries.
target_compile_options(Data_Summaries PRIVATE
        $<$<CONFIG:Release>:-O3>
        -march=native -mtune=native -Wall)

# ---- tests (ctest) ----
enable_testing()
add_executable(unit_tests tests/test_main.cpp)
target_link_libraries(unit_tests PRIVATE data_summaries_core)
target_compile_options(unit_tests PRIVATE -Wall -Wextra)
add_test(NAME unit_tests COMMAND unit_tests)
