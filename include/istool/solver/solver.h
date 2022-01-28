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

class VerifiedSolver: public Solver {
public:
    Verifier* v;
    VerifiedSolver(Specification* spec, Verifier* _v);
    virtual ~VerifiedSolver();
};

class PBESolver {
public:
    Specification* spec;
    PBESolver(Specification* _spec);
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr) = 0;
    virtual ~PBESolver() = default;
};

class CEGISSolver: public VerifiedSolver {
public:
    PBESolver* pbe_solver;
    CEGISSolver(PBESolver* _pbe_solver, Verifier* _v);
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr);
    virtual ~CEGISSolver();
};

typedef std::function<Solver*(Specification*, Verifier*)> SolverBuilder;
typedef std::function<PBESolver*(Specification*)> PBESolverBuilder;

#endif //ISTOOL_SOLVER_H
