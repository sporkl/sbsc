cmake_minimum_required(VERSION 3.18)

project(sbsc VERSION 0.0.1 LANGUAGES C CXX)

add_subdirectory(extern/sc)

add_subdirectory(src)
add_subdirectory(apps)
