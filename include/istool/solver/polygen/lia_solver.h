//
// Created by pro on 2022/1/4.
//

#ifndef ISTOOL_LIA_SOLVER_H
#define ISTOOL_LIA_SOLVER_H

#include "istool/solver/iterative_solver.h"
#include "istool/ext/z3/z3_extension.h"
#include "gurobi_c++.h"

struct LIAResult {
public:
    bool status;
    int c_val;
    std::vector<int> param_list;
    LIAResult();
    LIAResult(const std::vector<int>& _param_list, int _c_val);
    Data run(const Example& example) const;
    std::string toString() const;
};

class LIASolver: public PBESolver, public IterativeSolver {
public:
    LIASolver(Specification* _spec, const ProgramList& _program_list);
    Z3Extension* ext;
    IOExampleSpace* io_example_space;
    ProgramList program_list;
    PSynthInfo info;
    GRBEnv env;
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr);
    virtual void* relax(TimeGuard* guard);

    // configure
    int KTermIntMax, KConstIntMax, KMaxCost;
    double KRelaxTimeLimit = 0.1;
};

namespace solver {
    namespace lia {
        extern const std::string KTermIntMaxName;
        extern const std::string KConstIntMaxName;
        extern const std::string KMaxCostName;
        LIAResult solveLIA(GRBEnv& env, const std::vector<IOExample>& example_list, Z3Extension* ext, int t_max, int c_max, int cost_limit, TimeGuard* guard = nullptr);
        PProgram adjustLIAResultIntoGrammar(const PProgram& x, Grammar* grammar);
        LIASolver* liaSolverBuilder(Specification* spec);
    }
}


#endif //ISTOOL_LIA_SOLVER_H
