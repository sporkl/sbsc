
find_package(igraph REQUIRED)

add_library(sbsc sbsc.c ../include/sbsc/sbsc.h)
target_include_directories(sbsc PUBLIC ../include)

target_link_libraries(sbsc PUBLIC igraph::igraph)
target_link_libraries(sbsc PUBLIC sc_map)

if(MSVC)
  target_compile_options(sbsc PRIVATE /W4 /WX)
else()
  target_compile_options(sbsc PRIVATE -Wall -Wextra -Wpedantic)
endif()
