cmake_minimum_required(VERSION 3.5)
project(ISToolBasic)


add_subdirectory(parser)
add_subdirectory(theory)
add_subdirectory(ext)
aux_source_directory(basic BASIC_BASIC_SRC)
add_library(basic_basic_lib STATIC ${BASIC_BASIC_SRC})

set(BASIC_LIBS basic_basic_lib basic_ext_lib basic_theory_lib basic_sygus_lib PARENT_SCOPE)