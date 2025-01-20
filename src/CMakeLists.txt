
find_package(igraph REQUIRED)

add_library(sbsc sbsc.c ../include/sbsc/sbsc.h)
target_include_directories(sbsc PUBLIC ../include)
target_link_libraries(sbsc PUBLIC igraph::igraph)
