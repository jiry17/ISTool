//
// Created by pro on 2022/9/18.
//

#include "istool/basic/config.h"
#include "istool/incre/parser/incre_from_json.h"
#include "istool/incre/language/incre.h"
#include "istool/incre/analysis/incre_instru_info.h"

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
    std::string path = config::KSourcePath + "/tests/test.json";
    auto prog = incre::file2program(path);

    auto* info = incre::buildIncreInfo(prog);
    for (int i = 1; i <= 5; ++i) {
        info->example_pool->generatorExample();
    }
    /*invoke("head", {tl}, ctx);
    invoke("tail", {tl}, ctx);
    invoke("length", {tl}, ctx);
    invoke("concat", {tl, tl}, ctx);
    invoke("streaming", {tl}, ctx);
    invoke("dac", {tl}, ctx);*/
}