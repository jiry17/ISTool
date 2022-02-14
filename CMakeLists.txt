cmake_minimum_required(VERSION 3.5)
project(ISTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Ofast")
add_definitions(-DDEBUG)
INCLUDE(FindPkgConfig)
include_directories(include)

#Z3
set(Z3_PATH /home/jiry/z3)
link_directories(${Z3_PATH}/build)
set(Z3_FILE libz3.so)
include_directories(${Z3_PATH}/src/api/c++)
include_directories(${Z3_PATH}/src/api)

#jsoncpp
find_package(Jsoncpp)
include_directories(${Jsoncpp_INCLUDE_DIR})

#gurobi
set(GUROBI_PATH /home/jiry/gurobi912/linux64)
include_directories(${GUROBI_PATH}/include)
link_directories(${GUROBI_PATH}/lib)
set(GUROBI_FILE libgurobi_g++5.2.a libgurobi91.so)
 
# libs
set(THIRDPARTY_LIBS glog gflags ${Jsoncpp_LIBRARY} ${Z3_FILE} ${GUROBI_FILE})

# subdic
add_subdirectory(basic)
add_subdirectory(ext)
add_subdirectory(sygus)
add_subdirectory(solver)
add_subdirectory(selector)
add_subdirectory(dsl)
aux_source_directory(debug_tool DEBUG_TOOL_SRC)
add_library(debug_lib ${DEBUG_TOOL_SRC})
 
set(SRC_LIBS basic_lib ext_lib ${SyGuS_LIBS} solver_lib selector_lib dsl_lib debug_lib)

#add_executable(ISTool main.cpp)
#target_link_libraries(ISTool ${SRC_LIBS} ${SRC_LIBS} ${THIRDPARTY_LIBS})

add_executable(run run.cpp)
target_link_libraries(run ${SRC_LIBS} ${SRC_LIBS} ${THIRDPARTY_LIBS})

add_subdirectory(executor)