//
// Created by pro on 2022/9/18.
//


#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/autolabel/incre_autolabel_constraint_solver.h"
#include "istool/incre/autolifter/incre_nonscalar_autolifter.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/grammar/incre_component_collector.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    std::string path, label_path, target;
    bool is_autolabel = true;
    if (argv <= 1) {
        std::string name = "nonscalar/parallel_suffix_sum";
        path = config::KSourcePath + "incre-tests/" + name + ".f";
        label_path = config::KSourcePath + "tests/incre/label-res/" + name + ".f";
        target = config::KSourcePath + "tests/incre/optimize-res/" + name + ".f";
        is_autolabel = false;
    } else {
        path = std::string(argc[1]);
        label_path = std::string(argc[2]);
        target = std::string(argc[3]);
    }

    TimeGuard* global_guard = new TimeGuard(1e9);

    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());

    auto input_program = incre::parseFromF(path, false);
    // init_program = incre::removeGlobal(init_program.get());

    IncreProgram labeled_program;

    if (is_autolabel) {
        global::recorder.start("label");
        auto *label_solver = new autolabel::AutoLabelZ3Solver(input_program);
        labeled_program = label_solver->label();
        global::recorder.end("label");
    } else {
        labeled_program = input_program;
    }
    incre::applyConfig(labeled_program.get(), env.get());

    labeled_program = incre::eliminateNestedAlign(labeled_program.get());
    incre::printProgram(labeled_program, label_path);
    incre::printProgram(labeled_program);
    env->setConst(incre::grammar::collector::KCollectMethodName, BuildData(Int, incre::grammar::ComponentCollectorType::SOURCE));
    env->setConst(theory::clia::KINFName, BuildData(Int, 50000));

    auto* info = incre::buildIncreInfo(labeled_program, env.get());

    for (auto& align_info: info->align_infos) {
        align_info->print();
        if (align_info->getId() >= info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[align_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }

    auto* ns_solver = new autolifter::IncreNonScalarSolver(info, env);
    auto solution = ns_solver->solve();


   /* auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();*/
    solution.print();
    // LOG(INFO) << "After execute time " << global::recorder.query("execute");

    auto full_res = incre::rewriteWithIncreSolution(info->program.get(), solution, env.get());
    full_res = incre::eliminateUnusedLet(full_res.get());

    incre::printProgram(full_res, target);
    incre::printProgram(full_res);

    global::recorder.printAll();
    std::cout << global_guard->getPeriod() << std::endl;
    std::cout << "Success" << std::endl;
}