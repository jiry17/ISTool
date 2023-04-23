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
#include <iostream>
#include "istool/solver/autolifter/composed_sf_solver.h"

using namespace incre;

const std::unordered_map<std::string, int> KComposeNumConfig = {
        {"lsp/page21", 3}, {"lsp/page22-1", 4}, {"lsp/page22-2", 4},
        {"synduce/constraints/alist/most_frequent", 4},
        {"synduce/constraints/alist/most_frequent_clear", 4},
        {"synduce/constraints/bst/sum_between", 4},
        {"synduce/constraints/bst/most_frequent", 4},
        {"synduce/nested_list/pyramid_intervals", 5},
        {"synduce/tree/poly", 4},
        {"synduce/tree/poly2", 4},
        {"synduce/treepaths/mips_2", 4}
};
const std::unordered_map<std::string, int> KVerifyBaseNumConfig = {
        /*{"lsp/page22-1", 1000}*/
};
const std::unordered_map<std::string, int> KSizeLimitConfig = {
        {"synduce/constraints/ensures/maxsegstrip_noe", 20},
        {"synduce/constraints/ensures/maxsegstrip_full_noe", 20}
};

int main(int argv, char** argc) {
    //std::string name = "synduce/constraints/ensures/bal_2";
    std::string name = "synduce/ptree/maxsum";
    std::string path = config::KSourcePath + "incre-tests/" + name + ".f";
    std::string label_path = config::KSourcePath + "tests/incre/label-res/" + name + ".f";
    std::string target = config::KSourcePath + "tests/incre/optimize-res/" + name + ".f";
    auto init_program = incre::parseFromF(path, true);

    global::recorder.start("label");
    auto* label_solver = new autolabel::AutoLabelZ3Solver(init_program);
    auto res = label_solver->label();
    global::recorder.end("label");

    res = incre::eliminateNestedAlign(res.get());
    incre::printProgram(res, label_path);
    incre::printProgram(res);

    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());
    if (name.find("deepcoder") != std::string::npos) {
        env->setConst(incre::grammar::collector::KCollectMethodName, BuildData(Int, incre::grammar::ComponentCollectorType::LABEL));
    } else {
        env->setConst(incre::grammar::collector::KCollectMethodName, BuildData(Int, incre::grammar::ComponentCollectorType::SOURCE));
    }

    if (KComposeNumConfig.count(name) > 0) {
        int compose_num = KComposeNumConfig.find(name)->second;
        env->setConst(solver::autolifter::KComposedNumName, BuildData(Int, compose_num));
    }
    if (KVerifyBaseNumConfig.count(name) > 0) {
        int verify_base_num = KVerifyBaseNumConfig.find(name)->second;
        env->setConst(solver::autolifter::KOccamExampleNumName, BuildData(Int, verify_base_num));
    }
    if (KSizeLimitConfig.count(name) > 0) {
        int size_limit = KSizeLimitConfig.find(name)->second;
        env->setConst(incre::KDataSizeLimitName, BuildData(Int, size_limit));
    }

    auto* info = incre::buildIncreInfo(res, env.get());

    for (int i = 1; i <= 100; ++i) {
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