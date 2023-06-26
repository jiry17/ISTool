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
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    std::string path, label_path, target;
    if (argv <= 1) {
        std::string name = "dp/15-4";
        //std::string name = "synduce/nested_list/pyramid_intervals";
        //std::string name = "/synduce/ptree/sum";
        // std::string name = "autolifter/single-pass/3rd-min";
        //std::string name = "fusion/shortcut/page8";
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
    // init_program = incre::removeGlobal(init_program.get());

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

    /*auto* ctx = incre::run(res);
    auto* env_ctx = incre::envRun(res.get());
    for (int i = 1; i <= 10; ++i) {
        auto [term, global] = info->example_pool->generateStart();
        env_ctx->initGlobal(global);
        for (auto& [name, v]: global) {
            ctx->addBinding(name, std::make_shared<TmValue>(v));
        }
        auto v1 = incre::run(term, ctx), v2 = incre::envRun(term, env_ctx->start, env_ctx->holder);
        LOG(INFO) << v1.toString() << " " << v2.toString() << " " << term->toString();
        assert(v1 == v2);
    }
    return 0;*/

    for (auto& align_info: info->align_infos) {
        align_info->print();
        if (align_info->getId() >= info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[align_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }
    
    /*TyList final_type_list = {std::make_shared<TyTuple>((TyList){std::make_shared<TyInt>(), std::make_shared<TyInt>()})};
    for (int i = 0; i < info->align_infos.size(); ++i) {
        auto [param_list, grammar] = buildFinalGrammar(info, i, final_type_list);
        LOG(INFO) << "Hole grammar for #" << i;
        for (auto& param: param_list) {
            std::cout << param << " ";
        }
        std::cout << std::endl;
        grammar->print();
    }
    int kk; std::cin >> kk;*/

    // LOG(INFO) << "Pre execute time " << global::recorder.query("execute");

    auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();
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