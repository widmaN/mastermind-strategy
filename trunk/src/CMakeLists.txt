# Check for SSE2 support on the platform.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(SSE 2.0 REQUIRED)
#if(NOT SSE_FOUND)
#  message(FATAL_ERROR "SSE support is missing on this architecture.")
#else()
#  message(STATUS "Found SSE intrinsic support: version ${SSE_VERSION}")
#endif()

# Check for OpenMP support in the compiler.
find_package(OpenMP QUIET)
if(OPENMP_FOUND)
#  message(STATUS "Found OpenMP compiler switch: ${OpenMP_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
  message(STATUS "Missing OpenMP compiler support.")
  message(STATUS "Only single-threading will be built.")
endif()

# List of source files.
set(SRC_LIST CodeBreaker.cpp Engine.cpp ObviousStrategy.cpp Codeword.cpp OptimalCodeBreaker.cpp ColorEquivalence.cpp Generation.cpp StrategyTree.cpp Compare.cpp Interactive.cpp Test.cpp ConstraintEquivalence.cpp Main.cpp DummyEquivalenceFilter.cpp Mask.cpp)

# Set the build type to Release unless already set.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Set additional compiler/linker flags.
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x -Wall -Wextra -pedantic")
else()
  # Don't know what to do.
endif()

# Create executable.
add_executable(mmstrat ${SRC_LIST})
