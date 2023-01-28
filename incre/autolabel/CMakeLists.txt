cmake_minimum_required(VERSION 3.5)
project(ISToolIncreLanguageAutoLabel)

file(GLOB SRC "*.cpp" "greedy/*.cpp" "solver/*.cpp")

set(AUTOLABEL_SRC ${SRC} PARENT_SCOPE)