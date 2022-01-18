cmake_minimum_required(VERSION 3.5)
project(ISToolExt)

aux_source_directory(z3 Z3_SOURCE)
aux_source_directory(vsa VSA_SOURCE)
aux_source_directory(deepcoder DC_SOURCE)
aux_source_directory(composed_semantics CS_SOURCE)
add_library(ext_lib STATIC ${Z3_SOURCE} ${VSA_SOURCE} ${DC_SOURCE} ${CS_SOURCE})

