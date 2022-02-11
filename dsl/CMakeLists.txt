cmake_minimum_required(VERSION 3.5)
project(ISToolDSLs)

aux_source_directory(samplesy SY_DSL_SOURCE)
add_library(dsl_lib STATIC ${SY_DSL_SOURCE})

