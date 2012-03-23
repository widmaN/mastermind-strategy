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

# Set include directory.
include_directories("${CMAKE_SOURCE_DIR}/lib")

# Create executable.
add_executable(mmserve mmserve.cpp)

# Set library dependencies.
target_link_libraries(mmserve mastermind)
