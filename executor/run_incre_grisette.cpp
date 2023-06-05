//
// Created by 赵雨薇 on 2023/5/19.
//

#include "istool/solver/autolifter/composed_sf_solver.h"
#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/io/incre_to_haskell.h"
#include "istool/incre/io/incre_grisette.h"
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
        std::string name = "zyw/sum_pre";
        path = config::KSourcePath + "incre-tests/" + name + ".f";
        label_path = config::KSourcePath + "tests/incre/label-res/" + name + ".f";
        target = config::KSourcePath + "tests/incre/optimize-res/" + name + ".f";
    } else {
        path = std::string(argc[1]);
        label_path = std::string(argc[2]);
        target = std::string(argc[3]);
    }

    TimeGuard* global_guard = new TimeGuard(1e9);

    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());

    auto init_program = incre::parseFromF(path, true);
    global::recorder.start("label");
    auto* label_solver = new autolabel::AutoLabelZ3Solver(init_program);
    auto res = label_solver->label();
    global::recorder.end("label");
    incre::applyConfig(res.get(), env.get());

    res = incre::eliminateNestedAlign(res.get());
    env->setConst(incre::grammar::collector::KCollectMethodName, BuildData(Int, incre::grammar::ComponentCollectorType::SOURCE));
    env->setConst(theory::clia::KINFName, BuildData(Int, 50000));

    auto* info = incre::buildIncreInfo(res, env.get());

    // set context in example_pool
    info->example_pool->generateSingleExample();
    // get io pairs
    std::vector<std::pair<Term, Data>> io_pairs;
    for (int i = 0; i < 5; ++i) {
        auto term = (info->example_pool->generateStart()).first;
        auto result = incre::run(term, info->example_pool->ctx);
        io_pairs.push_back({term, result});
    }

    // get component pool
    std::cout << "zyw: print component_pool begin!" << std::endl;
    auto component_pool = info->component_pool;
    component_pool.print();
    std::cout << "zyw: print component_pool end!" << std::endl;

    // get align info
    std::vector<std::pair<std::vector<std::pair<std::string, Ty>>, Ty>> align_info_for_haskell;
    for (auto& align_info: info->align_infos) {
        std::cout << "zyw: align_info->print() begin!" << std::endl;
        align_info->print();
        align_info_for_haskell.push_back({align_info->inp_types, align_info->oup_type});
        std::cout << "zyw: align_info->print() end!" << std::endl;
    }

    auto* solver = new incre::IncreAutoLifterSolver(info, env);

    // final output
    incre::programToHaskell(res, io_pairs, info->align_infos, solver, label_path);

    global::recorder.printAll();
    std::cout << global_guard->getPeriod() << std::endl;
    std::cout << "Success" << std::endl;
    
    return 0;
}