//
// Created by pro on 2021/12/27.
//

#ifndef ISTOOL_ENUM_SOLVER_H
#define ISTOOL_ENUM_SOLVER_H

#include "istool/solver/solver.h"
#include "istool/solver/enum/enum.h"
#include <unordered_set>

class BasicEnumSolver: public Solver {
public:
    Verifier* v;
    BasicEnumSolver(Specification* _spec, Verifier* _v);
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr);
    virtual ~BasicEnumSolver();
};

typedef std::function<bool(Program*)> ProgramChecker;

class OBEOptimizer: public Optimizer {
public:
    ProgramChecker is_runnable;
    std::unordered_map<std::string, ExampleList> example_pool;
    std::unordered_set<std::string> visited_set;
    OBEOptimizer(const ProgramChecker & _is_runnable, const std::unordered_map<std::string, ExampleList>& _pool);
    virtual bool isDuplicated(const std::string& name, NonTerminal* nt, const PProgram& p);
    virtual void clear();
    virtual ~OBEOptimizer() = default;
};

class OBESolver: public PBESolver {
public:
    ProgramChecker is_runnable;
    Verifier* v;
    std::unordered_map<std::string, ProgramList> invoke_map;
    OBESolver(Specification* _spec, Verifier* _v, const ProgramChecker& _is_runnable);
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr);
};

#endif //ISTOOL_ENUM_SOLVER_H
