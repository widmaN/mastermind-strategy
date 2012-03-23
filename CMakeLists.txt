cmake_minimum_required(VERSION 2.6)

project(Mastermind)

add_subdirectory(lib)
add_subdirectory(src bin)
add_subdirectory(wiki man)
add_subdirectory(tests EXCLUDE_FROM_ALL)
