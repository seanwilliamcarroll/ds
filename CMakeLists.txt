cmake_minimum_required(VERSION 3.20)
project(ds LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Per-target warning flags — avoids leaking warnings into FetchContent dependencies.
# Call ds_configure_target(target_name) on every target we own.
option(DS_WERROR "Treat warnings as errors for our targets" OFF)

function(ds_configure_target target)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
  if(DS_WERROR)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
endfunction()

# --- Dependencies via FetchContent ---
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
)

# Prevent benchmark from building its own tests
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest googlebenchmark)

enable_testing()

# --- Topic subdirectories ---
add_subdirectory(compiler_systems/src)
add_subdirectory(fundamentals/src)
add_subdirectory(performance_oriented/src)
