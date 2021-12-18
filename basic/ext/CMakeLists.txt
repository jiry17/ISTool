cmake_minimum_required(VERSION 3.5)
project(ISToolExt)

file(GLOB z3_files "z3/*.cpp" "z3/*.h" "z3/semantics/*.cpp" "z3/semantics/*.h")
add_library(basic_ext_lib STATIC ${z3_files})

