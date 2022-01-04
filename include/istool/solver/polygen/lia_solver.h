//
// Created by pro on 2022/1/4.
//

#ifndef ISTOOL_LIA_SOLVER_H
#define ISTOOL_LIA_SOLVER_H

#include "istool/solver/iterative_solver.h"
#include "istool/ext/z3/z3_extension.h"

class LIASolver: public PBESolver, public IterativeSolver {
public:
    LIASolver(Specification* _spec);
    int term_int_max, const_int_max;
    Z3Extension* ext;
    IOExampleSpace* io_example_space;
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr);
    virtual bool relax();
};

namespace solver {
    namespace lia {
        extern const std::string KTermIntMaxName;
        extern const std::string KConstIntMaxName;
    }
}


#endif //ISTOOL_LIA_SOLVER_H
