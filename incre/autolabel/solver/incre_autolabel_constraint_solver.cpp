//
// Created by pro on 2023/1/27.
//

#include "istool/incre/autolabel/incre_autolabel_constraint_solver.h"
#include "glog/logging.h"
#include <iostream>

using namespace incre;
using namespace incre::autolabel;

autolabel::AutoLabelZ3Solver::AutoLabelZ3Solver(const IncreProgram &init_program):
    AutoLabelSolver(init_program), ctx(new Z3Context()) {
}

autolabel::AutoLabelZ3Solver::~AutoLabelZ3Solver() {
    delete ctx;
}

IncreProgram AutoLabelZ3Solver::label() {
    initZ3Context(init_program.get(), ctx);
    collectAlignConstraint(init_program.get(), ctx);

    z3::optimize solver(ctx->ctx);
    auto obj = collectMinimalAlignConstraint(init_program.get(), ctx);
    solver.minimize(obj);

    solver.add(ctx->cons_list);

    assert(solver.check() == z3::sat);
    auto model = solver.get_model();

    /*for (auto& [term, free_var]: ctx->flip_map) {
        LOG(INFO) << "is free " << term->toString() << " " << model.eval(free_var).bool_value();
    }*/

    LOG(INFO) << "objective " << model.eval(obj);

    return constructLabel(init_program.get(), model, ctx);
}
