//
// Created by pro on 2022/9/18.
//


#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/autolabel/incre_autolabel_constraint_solver.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/grammar/incre_component_collector.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include <iostream>

using namespace incre;

int main(int argv, char** argc) {
    std::string path, label_path, target;
    if (argv <= 1) {
        std::string name = "synduce/tree/poly";
        path = config::KSourcePath + "incre-tests/" + name + ".f";
        label_path = config::KSourcePath + "tests/incre/label-res/" + name + ".f";
        target = config::KSourcePath + "tests/incre/optimize-res/" + name + ".f";
    } else {
        path = std::string(argc[1]);
        label_path = std::string(argc[2]);
        target = std::string(argc[3]);
    }

    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());

    auto init_program = incre::parseFromF(path, true);

    global::recorder.start("label");
    auto* label_solver = new autolabel::AutoLabelZ3Solver(init_program);
    auto res = label_solver->label();
    global::recorder.end("label");
    incre::applyConfig(res.get(), env.get());

    res = incre::eliminateNestedAlign(res.get());
    incre::printProgram(res, label_path);
    incre::printProgram(res);
    env->setConst(incre::grammar::collector::KCollectMethodName, BuildData(Int, incre::grammar::ComponentCollectorType::SOURCE));
    env->setConst(theory::clia::KINFName, BuildData(Int, 50000));

    auto* info = incre::buildIncreInfo(res, env.get());

    for (int i = 1; i <= 10; ++i) {
        info->example_pool->generateSingleExample();
    }

    for (auto& align_info: info->align_infos) {
        align_info->print();
        if (align_info->getId() >= info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[align_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }

    // LOG(INFO) << "Pre execute time " << global::recorder.query("execute");

    auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();
    solution.print();
    // LOG(INFO) << "After execute time " << global::recorder.query("execute");

    auto full_res = incre::rewriteWithIncreSolution(info->program.get(), solution);
    full_res = incre::eliminateUnusedLet(full_res.get());

    incre::printProgram(full_res, target);
    incre::printProgram(full_res);

    global::recorder.printAll();
}