//
// Created by pro on 2022/9/25.
//

#ifndef ISTOOL_INCRE_SOLVER_H
#define ISTOOL_INCRE_SOLVER_H

#include "analysis/incre_instru_info.h"

namespace incre {

    class IncreSolution {
    public:
        ProgramList f_list;
        ProgramList tau_list;
        IncreSolution(const ProgramList& _f_list, const ProgramList& _tau_list);
        virtual ~IncreSolution() = default;
    };

    class IncreSolver {
    public:
        IncreInfo* info;
        IncreSolver(IncreInfo* _info);
        virtual IncreSolution solve() = 0;
        virtual ~IncreSolver() = default;
    };
}

#endif //ISTOOL_INCRE_SOLVER_H
