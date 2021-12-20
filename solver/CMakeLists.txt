cmake_minimum_required(VERSION 3.5)
project(ISToolSolver)

aux_source_directory(component_based_solver CB_SRC)
add_library(solver_lib STATIC solver.cpp ${CB_SRC})