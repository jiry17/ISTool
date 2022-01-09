//
// Created by pro on 2022/1/9.
//

#include <istool/sygus/sygus.h>
#include "istool/solver/vsa/vsa_solver.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/witness/theory_witness.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"

FunctionContext invokeVSA(Specification* spec) {
    auto info = spec->info_list[0];
    auto* pruner = new SizeLimitPruner(10000, [](VSANode* node) {
        auto* sn = dynamic_cast<SingleVSANode*>(node);
        if (sn) {
            auto* oup = dynamic_cast<DirectWitnessValue*>(sn->oup.get());
            if (oup) {
                auto* w = dynamic_cast<StringValue*>(oup->d.get());
                if (w) return w->s.length() >= 15;
            }
        }
        return false;
    });
    auto* builder = new BFSVSABuilder(info->grammar, pruner, spec->env.get());
    // auto* builder = new DFSVSABuilder(info->grammar, pruner, spec->env);

    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);

    auto prepare = [](Specification* spec, const IOExample& io_example) {
        auto* env = spec->env.get(); auto* g = spec->info_list[0]->grammar;
        std::vector<std::string> string_const_list;
        auto add_const = [&](const Data& d) {
            auto* sv = dynamic_cast<StringValue*>(d.get());
            if (sv) string_const_list.push_back(sv->s);
        };
        for (auto* symbol: g->symbol_list) {
            for (auto* rule: symbol->rule_list) {
                auto* sem = dynamic_cast<ConstSemantics*>(rule->semantics.get());
                if (sem) add_const(sem->w);
            }
        }
        for (const auto& inp: io_example.first) add_const(inp);
        add_const(io_example.second);

        DataList const_list; int int_max = 1;
        for (const auto& s: string_const_list) {
            const_list.push_back(BuildData(String, s));
            int_max = std::max(int_max, int(s.length()));
        }

        env->setConst(theory::clia::KWitnessIntMinName, BuildData(Int, -1));
        env->setConst(theory::string::KStringConstList, const_list);
        env->setConst(theory::clia::KWitnessIntMaxName, BuildData(Int, int_max));
    };

    auto* solver = new BasicVSASolver(spec, builder, prepare, ext::vsa::getSizeModel());
    auto* verifier = sygus::getVerifier(spec);
    auto* cegis = new CEGISSolver(solver, verifier);

    auto res = cegis->synthesis();
    return res;
}

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    std::string benchmark_name, output_name, solver_name;
    if (argc == 4) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
    } else {
        benchmark_name = "/tmp/tmp.hABJQGVQ89/tests/string/phone-5_short.sl";
        output_name = "/tmp/629453237.out";
        solver_name = "vsa";
    }
    auto* spec = parser::getSyGuSSpecFromFile(benchmark_name);
    FunctionContext result;
    auto* guard = new TimeGuard(1e9);
    if (solver_name == "vsa") {
        result = invokeVSA(spec);
    } else assert(0);
    std::cout << result.toString() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%s\n", result.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}