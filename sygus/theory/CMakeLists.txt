cmake_minimum_required(VERSION 3.5)
project(ISToolSyGuSTheory)

add_library(basic_theory_lib STATIC clia_semantics.cpp clia_semantics.h theory.cpp theory.h)