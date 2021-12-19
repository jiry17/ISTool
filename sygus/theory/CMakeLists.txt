cmake_minimum_required(VERSION 3.5)
project(ISToolSyGuSTheory)

aux_source_directory(basic/clia BASIC_CLIA_SRC)
set(BASIC_SRC ${BASIC_CLIA_SRC})

add_library(sygus_theory_lib STATIC ${BASIC_SRC} theory.cpp theory.h)