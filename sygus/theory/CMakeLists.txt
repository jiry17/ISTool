cmake_minimum_required(VERSION 3.5)
project(ISToolSyGuSTheory)

aux_source_directory(basic/clia BASIC_CLIA_SRC)
set(BASIC_SRC ${BASIC_CLIA_SRC} basic/theory_semantics.cpp)
aux_source_directory(z3/clia Z3_CLIA_SRC)
set(Z3_SRC ${Z3_CLIA_SRC} z3/theory_z3_semantics.cpp)

add_library(sygus_theory_lib STATIC ${BASIC_SRC} ${Z3_SRC} theory.cpp)