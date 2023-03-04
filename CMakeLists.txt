cmake_minimum_required(VERSION 3.13)
project(ISTool)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17 -Ofast")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17 -O0")
add_definitions(-DDEBUG)

INCLUDE(FindPkgConfig)
include_directories(include)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLINUX)

    #Z3
    set(Z3_PATH /home/jiry/2021A/z3)
    link_directories(${Z3_PATH}/build)
    set(Z3_FILE libz3.so)
    include_directories(${Z3_PATH}/src/api/c++)
    include_directories(${Z3_PATH}/src/api)

    #jsoncpp
    find_package(Jsoncpp)
    include_directories(${Jsoncpp_INCLUDE_DIR})

    #gurobi
    set(GUROBI_PATH /home/jiry/2021A/gurobi912/linux64)
    include_directories(${GUROBI_PATH}/include)
    link_directories(${GUROBI_PATH}/lib)
    set(GUROBI_FILE libgurobi_g++5.2.a libgurobi91.so)

    #Profiler
    #option(USE_PROFILER "option for using libprofiler" OFF)
    #if (USE_PROFILER)
    #  add_definitions(-DPROFILER)
    #  include_directories(${LIBPROFILER_INCLUDE_DIRS})
    #  set(PROFILER_FILE ${LIBPROFILER_LIBRARIES})
    #endif()
    set(THIRDPARTY_LIBS glog gflags ${Jsoncpp_LIBRARY} ${Z3_FILE} ${GUROBI_FILE} ${PROFILER_FILE})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    #z3
    set(Z3_PATH /Users/pro/Desktop/work/2023S/z3)
    link_directories(/usr/local/lib)
    set(Z3_FILE libz3.dylib)
    include_directories(${Z3_PATH}/src/api/c++)
    include_directories(${Z3_PATH}/src/api)

    #jsoncpp
    set(Jsoncpp_LIBRARY /usr/local/lib/libjsoncpp.dylib)
    include_directories(/usr/local/include)

    #glog
    include_directories(/usr/local/opt/glog/include/)
    set(GLOG_FILE /usr/local/opt/glog/lib/libglog.dylib)

    #gurobi
    include_directories(/Library/gurobi911/mac64/include)
    link_directories(/Library/gurobi911/mac64/lib)
    set(GUROBI_FILE libgurobi_c++.a libgurobi91.dylib)

    # libs
    set(THIRDPARTY_LIBS ${Jsoncpp_LIBRARY} ${Z3_FILE} ${GUROBI_FILE} ${PROFILER_FILE} ${GLOG_FILE})
else()
    message(FATAL_ERROR "Unexceptional system ${CMAKE_SYSTEM_NAME}")
endif()

# subdic
add_subdirectory(basic)
add_subdirectory(ext)
add_subdirectory(sygus)
add_subdirectory(solver)
add_subdirectory(selector)
add_subdirectory(dsl)
add_subdirectory(incre)
 
set(SRC_LIBS solver_lib  ${SyGuS_LIBS} ext_lib selector_lib basic_lib dsl_lib incre_lib)

add_subdirectory(executor)