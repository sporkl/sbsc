add_executable(basic_ints basic_ints.c)
target_link_libraries(basic_ints PRIVATE sbsc)

if(MSVC)
  target_compile_options(sbsc PRIVATE /W4 /WX)
else()
  target_compile_options(sbsc PRIVATE -Wall -Wextra -Wpedantic)
endif()
