//
// Created by pro on 2022/1/8.
//

#ifndef ISTOOL_POLYGEN_CEGIS_H
#define ISTOOL_POLYGEN_CEGIS_H

#include "polygen_term_solver.h"
#include "polygen_condition_solver.h"

class CEGISPolyGen: public Solver {
    PolyGenTermSolver* term_solver;
    PolyGenConditionSolver* cond_solver;
    Verifier* v;
    IOExampleSpace* io_space;
public:
    CEGISPolyGen(Specification* spec, const PSynthInfo& term_info, const PSynthInfo& unify_info,
            const PBESolverBuilder& domain_builder, const PBESolverBuilder& dnf_builder, Verifier* _v);
    ~CEGISPolyGen();
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr);
};

#endif //ISTOOL_POLYGEN_CEGIS_H
