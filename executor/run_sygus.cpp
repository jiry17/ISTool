//
// Created by pro on 2022/2/15.
//

#include "istool/selector/split/splitor.h"
#include "istool/invoker/invoker.h"
#include "istool/sygus/sygus.h"
#include "istool/sygus/parser/parser.h"
#include "istool/ext/z3/z3_example_space.h"
#include "istool/sygus/theory/z3/bv/bv_z3.h"
#include "istool/ext/composed_semantics/composed_z3_semantics.h"
#include "istool/ext/composed_semantics/composed_rule.h"
#include <ctime>

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    std::string benchmark_name, output_name, solver_name;
    if (argc == 4) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
    } else {
        solver_name = "obe";
        //benchmark_name = "/tmp/tmp.wHOuYKwdWN/tests/bv/PRE_icfp_gen_14.10.sl";
        benchmark_name = "/tmp/tmp.wHOuYKwdWN/tests/x.sl";
        output_name = "/tmp/629453237.out";
    }
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    auto* v = sygus::getVerifier(spec);
    spec->env->random_engine.seed(time(0));
    auto solver_token = invoker::string2TheoryToken(solver_name);
    auto* guard = new TimeGuard(300);

    if (solver_name == "cbs" && sygus::getSyGuSTheory(spec->env.get()) == TheoryToken::BV) {
        theory::loadZ3BV(spec->env.get());
        ext::z3::registerComposedManager(ext::z3::getExtension(spec->env.get()));
    }
    if (solver_name == "obe") {
        for (auto& info: spec->info_list) {
            info->grammar = ext::grammar::rewriteComposedRule(info->grammar);
        }
    }

    InvokeConfig config;
    if (solver_name == "cbs") config.set("encoder", std::string("Tree"));
    auto result = invoker::getExampleNum(spec, v, solver_token, guard, config);
    std::cout << result.first << " " << result.second.toString() << std::endl;
    std::cout << guard->getPeriod() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}