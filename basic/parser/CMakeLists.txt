cmake_minimum_required(VERSION 3.5)
project(ISToolSyGuS)

include_directories(/tmp/tmp.WY6AmQ8CxJ/)
aux_source_directory(sygus SYGUS_PARSER_SRC)
file(GLOB parser_files json_util.cpp json_util.h ${SYGUS_PARSER_SRC})
add_library(basic_sygus_lib STATIC ${parser_files})
