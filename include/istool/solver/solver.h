//
// Created by pro on 2021/12/9.
//

#ifndef ISTOOL_SOLVER_H
#define ISTOOL_SOLVER_H

#include "istool/basic/specification.h"
#include "istool/basic/verifier.h"
#include "istool/basic/time_guard.h"

class Solver {
public:
    Specification* spec;
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr) = 0;
    Solver(Specification* _spec);
    virtual ~Solver() = default;
};

class PBESolver {
public:
    Specification* spec;
    PBESolver(Specification* _spec);
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr) = 0;
    virtual ~PBESolver() = default;
};

class CEGISSolver: public Solver {
public:
    PBESolver* pbe_solver;
    Verifier* v;
    CEGISSolver(PBESolver* _pbe_solver, Verifier* _v);
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr);
    virtual ~CEGISSolver();
};

#endif //ISTOOL_SOLVER_H
