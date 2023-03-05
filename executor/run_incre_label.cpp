//
// Created by pro on 2022/9/18.
//


#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/autolabel/incre_autolabel_constraint_solver.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/solver/polygen/dnf_learner.h"
#include "glog/logging.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    auto name = "lsp/page10";

    std::string path = config::KSourcePath + "tests/incre/benchmark/" + name + ".f";
    std::string label_path = config::KSourcePath + "tests/incre/label-res/" + name + ".f";
    std::string target = config::KSourcePath + "tests/incre/optimize-res/" + name + ".f";
    auto init_program = incre::parseFromF(path, true);


    auto* label_solver = new autolabel::AutoLabelZ3Solver(init_program);
    auto res = label_solver->label();

    res = incre::eliminateNestedAlign(res.get());
    incre::printProgram(res, label_path);

    incre::printProgram(res);

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

    auto full_res = incre::rewriteWithIncreSolution(info->program.get(), solution);
    full_res = incre::eliminateUnusedLet(full_res.get());

    incre::printProgram(full_res, target);
}