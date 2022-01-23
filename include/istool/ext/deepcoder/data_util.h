//
// Created by pro on 2022/1/21.
//

#ifndef ISTOOL_DATA_UTIL_H
#define ISTOOL_DATA_UTIL_H

#include "istool/basic/program.h"

namespace ext::ho {
    Data polyFMap(Program* p, Type* type, const Data& data, Env* env);
    ProgramList splitTriangle(const PProgram& p);
    PProgram removeTriAccess(const PProgram& p);
    PProgram buildAccess(const PProgram& p, const std::vector<int>& trace);
    PProgram buildTriangle(const ProgramList& sub_list);
    PProgram mergeTriangle(const ProgramList& p_list);
}

#endif //ISTOOL_DATA_UTIL_H
