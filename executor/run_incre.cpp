//
// Created by pro on 2022/9/18.
//

#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/language/incre.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "glog/logging.h"

using namespace incre;

Term buildList(const std::vector<std::pair<int, int>>& A) {
    auto res = Data(std::make_shared<VInductive>("nil", Data()));
    for (int i = A.size(); i; --i) {
        auto iv = A[i - 1];
        auto w = Data(std::make_shared<VTuple>(DataList{BuildData(Int, iv.first), BuildData(Int, iv.second)}));
        auto vt = Data(std::make_shared<VTuple>(DataList {w, res}));
        res = Data(std::make_shared<VInductive>("cons", vt));
    }
    return std::make_shared<TmValue>(res);
}

void invoke(const std::string& name, const TermList& ts, Context* ctx) {
    Term t = std::make_shared<TmVar>(name);
    for (int i = 0; i < ts.size(); ++i) t = std::make_shared<TmApp>(t, ts[i]);
    auto res = incre::run(t, ctx);
    std::cout << res.toString() << std::endl;
}

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = "/home/jiry/2022A/IncreLanguage//tmp.json";
    }
    auto prog = incre::file2program(path);


    auto env = std::make_shared<Env>();
    incre::autolifter::prepareAutoLifterEnv(env.get());
    auto* info = incre::buildIncreInfo(prog, env.get());
    for (int i = 1; i <= 100; ++i) {
        info->example_pool->generateExample();
    }

    for (auto& pass_info: info->pass_infos) {
        pass_info->print();
        if (pass_info->getId() < info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[pass_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }

    incre::printProgram(prog, config::KSourcePath + "result.txt");

    auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();
    solution.print();

    auto res = incre::rewriteWithIncreSolution(info->program.get(), solution);

    // test
    auto* new_ctx = incre::run(res);
    for (int i = 0; i < 100; ++i) {
        auto start_term = info->example_pool->generator->getStartTerm();
        auto x = incre::run(start_term, info->ctx);
        auto y = incre::run(start_term, new_ctx);
        if (!(x == y)) {
            LOG(FATAL) << "Counter example " << start_term->toString() << " std: " << x.toString() << " oup: " << y.toString();
        }
    }



    incre::printProgram(res, target);

    /*invoke("head", {tl}, ctx);
    invoke("tail", {tl}, ctx);
    invoke("length", {tl}, ctx);
    invoke("concat", {tl, tl}, ctx);
    invoke("streaming", {tl}, ctx);
    invoke("dac", {tl}, ctx);*/
}