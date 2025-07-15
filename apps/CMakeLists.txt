add_executable(basic_ints basic_ints.c)
target_link_libraries(basic_ints PRIVATE sbsc)

find_package(igraph REQUIRED)
add_executable(main_ints main_ints.c)
target_link_libraries(main_ints PRIVATE sbsc)
target_link_libraries(main_ints PRIVATE igraph::igraph)

if(MSVC)
  target_compile_options(sbsc PRIVATE /W4 /WX)
else()
  target_compile_options(sbsc PRIVATE -Wall -Wextra -Wpedantic)
endif()
