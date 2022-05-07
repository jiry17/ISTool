cmake_minimum_required(VERSION 3.5)
project(ISToolExecutor)
cmake_policy(SET CMP0079 NEW)
aux_source_directory(invoker INVOKER_SRC)
add_library(invoker_lib STATIC ${INVOKER_SRC} invoker/invoker.cpp)
set(ALL_LIBS ${SRC_LIBS} invoker_lib)

add_executable(run_cbs_icse10_2 run_cbs_icse10.cpp)
target_link_libraries(run_cbs_icse10_2 ${ALL_LIBS} ${ALL_LIBS} ${THIRDPARTY_LIBS})

add_executable(run_sygus run_sygus.cpp)
target_link_libraries(run_sygus ${ALL_LIBS} ${ALL_LIBS}  ${THIRDPARTY_LIBS})

add_executable(run_splitor2 run_splitor.cpp)
target_link_libraries(run_splitor2 ${ALL_LIBS} ${ALL_LIBS} ${THIRDPARTY_LIBS})

add_executable(run_autolifter run_autolifter.cpp)
target_link_libraries(run_autolifter ${ALL_LIBS} ${ALL_LIBS} ${THIRDPARTY_LIBS})

add_executable(test test.cpp)
target_link_libraries(test ${ALL_LIBS} ${ALL_LIBS} ${THIRDPARTY_LIBS})