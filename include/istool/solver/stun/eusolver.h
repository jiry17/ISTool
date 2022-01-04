//
// Created by pro on 2022/1/4.
//

#ifndef ISTOOL_EUSOLVER_H
#define ISTOOL_EUSOLVER_H

#include "stun.h"

class EuTermSolver: public TermSolver {
public:
    PSynthInfo tg;
    ExampleSpace* example_space;
    EuTermSolver(const PSynthInfo& _tg, ExampleSpace* _example_space);
    virtual ProgramList synthesisTerms(const ExampleList& example_list, TimeGuard* guard = nullptr);
    virtual ~EuTermSolver() = default;
};

class EuUnifier: public Unifier {
public:
    PSynthInfo cg;
    ExampleSpace* example_space;
    EuUnifier(const PSynthInfo& _cg, ExampleSpace* _example_space);
    virtual PProgram unify(const ProgramList& term_list, const ExampleList& example_list, TimeGuard* guard = nullptr);
    virtual ~EuUnifier() = default;
};

class EuSolver: public STUNSolver {
public:
    EuSolver(Specification* _spec, const PSynthInfo& tg, const PSynthInfo& cg);
};

#endif //ISTOOL_EUSOLVER_H
