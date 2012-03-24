cmake_minimum_required(VERSION 2.8)

project(Mastermind)

# Check for OpenMP support in the compiler.
find_package(OpenMP QUIET)
if(OPENMP_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
  message(STATUS "Missing OpenMP compiler support.")
  message(STATUS "Only single-threading will be built.")
endif()

# Set additional compiler/linker flags.
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x -Wall -Wextra -pedantic")
else()
  # Don't know what to do.
endif()

# Set the build type to Release unless already set.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(lib)
add_subdirectory(src bin)
add_subdirectory(wiki man)
add_subdirectory(tests EXCLUDE_FROM_ALL)
