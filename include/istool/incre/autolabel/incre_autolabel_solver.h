//
// Created by pro on 2022/12/18.
//

#ifndef ISTOOL_INCRE_AUTOLABEL_SOLVER_H
#define ISTOOL_INCRE_AUTOLABEL_SOLVER_H

#include "incre_autolabel.h"

namespace incre::autolabel {

    class BasicIncreLabelSolver: public IncreLabelSolver {
    public:
        virtual z3::model solve(LabelContext* ctx, ProgramData* program);
    };

    class MinCoverIncreLabelSolver: public IncreLabelSolver {
    public:
        virtual z3::model solve(LabelContext* ctx, ProgramData* program);
    };

    class MinInvolvedIncreLabelSolver: public IncreLabelSolver {
    public:
        virtual z3::model solve(LabelContext* ctx, ProgramData* program);
    };

    class MinMixedIncerLbaleSolver: public IncreLabelSolver {
    public:
        virtual z3::model solve(LabelContext* ctx, ProgramData* program);
    };

}

#endif //ISTOOL_INCRE_AUTOLABEL_SOLVER_H
