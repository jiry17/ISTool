cmake_minimum_required(VERSION 3.5)
project(ISToolIncreLanguage)

aux_source_directory(language LANGUAGE_SOURCE)
aux_source_directory(io PARSER_SOURCE)
aux_source_directory(analysis ANA_SOURCE)
aux_source_directory(autolifter AUTO_INCRE_SOURCE)
#aux_source_directory(autolabel AUTO_LABEL_SOURCE)
aux_source_directory(trans TRANS_SOURCE)
message(${PARSER_SOURCE})
add_library(incre_lib STATIC ${LANGUAGE_SOURCE} ${PARSER_SOURCE} ${ANA_SOURCE} ${TRANS_SOURCE} ${AUTO_INCRE_SOURCE} incre_solver.cpp
        )

