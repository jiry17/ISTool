cmake_minimum_required(VERSION 3.5)
project(ISToolRecursiveSolver)

file(GLOB SRC "*.cpp" "basic/*.cpp")
set(RECURSIVE_SRC ${SRC} PARENT_SCOPE)