cmake_minimum_required(VERSION 3.5)
project(ISTool)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)
add_definitions(-DDEBUG)
INCLUDE(FindPkgConfig)
include_directories("${CMAKE_SOURCE_DIR}")

#Z3
set(Z3_PATH /home/jiry/2021A/PolyGen/recommend/my-euphony/thirdparty/z3)
link_directories(${Z3_PATH}/build)
set(Z3_FILE libz3.so)
include_directories(${Z3_PATH}/src/api/c++)
include_directories(${Z3_PATH}/src/api)

#jsoncpp
find_package(Jsoncpp)
include_directories(${Jsoncpp_INCLUDE_DIR})

# libs
set(THIRDPARTY_LIBS glog gflags ${Jsoncpp_LIBRARY} ${Z3_FILE})

# subdic
add_subdirectory(basic)
add_subdirectory(solver)

add_executable(ISTool main.cpp)
target_link_libraries(ISTool solver_lib ${BASIC_LIBS} ${THIRDPARTY_LIBS} ${BASIC_LIBS})