cmake_minimum_required(VERSION 3.5)
project(ISToolSelector)

aux_source_directory(baseline BASELINE_SOURCE)
aux_source_directory(split SPLIT_SOURCE)
add_library(selector_lib STATIC ${BASELINE_SOURCE} ${SPLIT_SOURCE} selector.cpp)

