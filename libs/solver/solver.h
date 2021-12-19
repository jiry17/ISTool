//
// Created by pro on 2021/12/9.
//

#ifndef ISTOOL_SOLVER_H
#define ISTOOL_SOLVER_H

#include "basic/specification.h"

class Solver {
public:
    virtual PProgram synthesis(Specification* spec) = 0;
    virtual ~Solver() = default;
};

class PBESolver {
public:
    virtual PProgram synthesis(Specification* spec, const std::vector<PExample>& example_list) = 0;
    virtual ~PBESolver() = default;
};

class CEGISSolver: public Solver {
public:
    PBESolver* pbe_solver;
    CEGISSolver(PBESolver* _pbe_solver);
    virtual PProgram synthesis(Specification* spec);
    virtual ~CEGISSolver();
};


#endif //ISTOOL_SOLVER_H
