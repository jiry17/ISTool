//
// Created by pro on 2021/12/9.
//

#include "solver.h"

CEGISSolver::CEGISSolver(PBESolver *_pbe_solver): pbe_solver(_pbe_solver) {
}
CEGISSolver::~CEGISSolver() {
    delete pbe_solver;
}
PProgram CEGISSolver::synthesis(Specification *spec) {
    std::vector<PExample> example_list;
    while (1) {
        PProgram program = pbe_solver->synthesis(spec, example_list);
        auto counter_example = spec->v->verify(program);
        if (!counter_example) return program;
        example_list.push_back(counter_example);
    }
}