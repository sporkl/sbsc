add_executable(basic_ints basic_ints.c)
target_link_libraries(basic_ints PRIVATE sbsc)

find_package(igraph REQUIRED)

add_executable(main_ints_v2 main_ints_v2.c)
target_link_libraries(main_ints_v2 PRIVATE sbsc)
target_link_libraries(main_ints_v2 PRIVATE igraph::igraph)

add_executable(initdeg12_ints_v2 initdeg12_ints_v2.c)
target_link_libraries(initdeg12_ints_v2 PRIVATE sbsc)
target_link_libraries(initdeg12_ints_v2 PRIVATE igraph::igraph)

if(MSVC)
  target_compile_options(sbsc PRIVATE /W4 /WX)
else()
  target_compile_options(sbsc PRIVATE -Wall -Wextra -Wpedantic)
endif()
