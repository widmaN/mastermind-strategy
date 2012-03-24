# Set include directory.
include_directories("${CMAKE_SOURCE_DIR}/lib")

# Create executable: mmserve.
add_executable(mmserve mmserve.cpp)
target_link_libraries(mmserve mastermind)

# Create executable: mmstrat.
add_executable(mmstrat mmstrat.cpp)
target_link_libraries(mmstrat mastermind)

