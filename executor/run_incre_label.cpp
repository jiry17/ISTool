//
// Created by pro on 2022/9/18.
//


#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/autolabel/incre_autolabel_constraint_solver.h"
#include "istool/incre/autolabel/incre_func_type.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "glog/logging.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = "/Users/pro/Desktop/work/2023S/ISTool/tests/test.json";
    }
    auto prog = incre::file2program(path);

    prog = incre::autolabel::buildFuncTerm(prog.get());
    auto task = incre::autolabel::initTask(prog.get());


    auto* label_solver = new autolabel::AutoLabelZ3Solver(task);
    auto res = label_solver->label();
    res = autolabel::unfoldFuncTerm(res.get());

    incre::printProgram(res);

    res = incre::eliminateNestedAlign(res.get());

    int kk; std::cin >> kk;
    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());
    auto* info = incre::buildIncreInfo(res, env.get());
    for (int i = 1; i <= 100; ++i) {
        info->example_pool->generateExample();
    }

    for (auto& align_info: info->align_infos) {
        align_info->print();
        if (align_info->getId() >= info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[align_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }

    auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();
    solution.print();
}