//
// Created by pro on 2022/1/3.
//

#ifndef ISTOOL_STUN_H
#define ISTOOL_STUN_H

#include "istool/solver/solver.h"

class TermSolver {
public:
    virtual ProgramList synthesisTerms(const ExampleList& example_list, TimeGuard* guard = nullptr) = 0;
    virtual ~TermSolver() = default;
};

class Unifier {
public:
    virtual PProgram unify(const ProgramList& term_list, const ExampleList& example_list, TimeGuard* guard = nullptr) = 0;
    virtual ~Unifier() = default;
};

class STUNSolver: public PBESolver {
public:
    TermSolver* term_solver;
    Unifier* unifier;
    std::string func_name;
    STUNSolver(Specification* spec, TermSolver* _term_solver, Unifier* _unifier);
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr);
    virtual ~STUNSolver();
};

#endif //ISTOOL_STUN_H
