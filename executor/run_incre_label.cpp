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
#include "glog/logging.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

#ifdef LINUX
    const std::string KLanguagePath = "/home/jiry/2023S/IncreLanguage/";
#else
    const std::string KLanguagePath = "/Users/pro/Desktop/work/2023S/ISTool/tests/";
#endif

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        auto name = "mts_label_test2";
        path = KLanguagePath + "/jsonfiles/" + name + ".json";
        target = KLanguagePath + "/labelres/" + name + ".f";
    }
    auto init_program = incre::file2program(path);


    auto* label_solver = new autolabel::AutoLabelZ3Solver(init_program);
    auto res = label_solver->label();

    res = incre::eliminateNestedAlign(res.get());

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

    incre::printProgram(full_res, target);
}