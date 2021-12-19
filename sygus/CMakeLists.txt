cmake_minimum_required(VERSION 3.5)
project(ISToolSyGuS)

add_subdirectory(theory)
aux_source_directory(parser sygus_parser_src)
add_library(sygus_parser_lib ${sygus_parser_src})
set(SyGuS_LIBS sygus_theory_lib sygus_parser_lib)