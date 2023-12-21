//
// Created by zyw on 2023/5/19.
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
#include "istool/incre/grammar/incre_grammar_builder.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    std::string path, label_path, result_path, haskell_path;
    if (argv <= 1) {
        std::string name = "zyw/sum_pre";
        path = config::KSourcePath + "incre-tests/" + name + ".f";
        // path for label result
        label_path = config::KSourcePath + "tests/incre/label-res/" + name + ".f";
        // path for synthesis result
        result_path = config::KSourcePath + "tests/incre/optimize-res/" + name + ".f";
        // path for file of haskell
        haskell_path = config::KSourcePath + "tests/incre/haskell-res/" + name + ".f";
    } else {
        path = std::string(argc[1]);
        label_path = std::string(argc[2]);
        result_path = std::string(argc[3]);
        haskell_path = std::string(argc[4]);
    }

    TimeGuard* global_guard = new TimeGuard(1e9);

    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());

    auto init_program = incre::parseFromF(path, true);
    // label
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
    // set context in example_pool
    for (int i = 1; i <= 10; ++i) {
        info->example_pool->generateSingleExample();
    }

    // get result from AutoLabel
    auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();
    std::cout << "zyw: solution.print() begin!" << std::endl;
    solution.print();
    std::cout << "zyw: solution.print() end!" << std::endl;
    // LOG(INFO) << "After execute time " << global::recorder.query("execute");

    auto full_res = incre::rewriteWithIncreSolution(info->program.get(), solution, env.get());
    full_res = incre::eliminateUnusedLet(full_res.get());

    incre::printProgram(full_res, result_path);
    incre::printProgram(full_res);

    // get io pairs
    std::vector<std::pair<Term, Data>> io_pairs;
    for (int i = 0; i < 20; ++i) {
        auto example = info->example_pool->generateStart();
        info->example_pool->ctx->initGlobal(example.second);
        auto* start = info->example_pool->ctx->start;
        auto* holder = info->example_pool->ctx->holder;
        auto result = incre::envRun(example.first, start, holder);
        io_pairs.push_back({example.first, result});
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

    // auto* solver = new incre::IncreAutoLifterSolver(info, env);

    // final output
    TyList final_type_list = {std::make_shared<TyTuple>((TyList){std::make_shared<TyInt>(), std::make_shared<TyInt>()})};
    // TyList final_type_list = {std::make_shared<TyInt>()};
    incre::programToHaskell(res, io_pairs, info, solver, haskell_path, final_type_list);

    global::recorder.printAll();
    std::cout << global_guard->getPeriod() << std::endl;
    std::cout << "Success" << std::endl;
    
    return 0;
}