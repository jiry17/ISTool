#include <iostream>

#include "z3++.h"
#include "istool/basic/config.h"
#include "istool/sygus/sygus.h"
#include "istool/solver/solver.h"
#include "istool/solver/component/component_based_solver.h"
#include "istool/solver/component/linear_encoder.h"
#include "istool/solver/enum/enum_solver.h"
#include "istool/solver/vsa/vsa_solver.h"
#include "istool/solver/stun/eusolver.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/witness/theory_witness.h"
#include "glog/logging.h"

FunctionContext CBSInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);

    std::unordered_map<std::string, Z3GrammarEncoder*> encoder_map;
    for (const auto& info: spec->info_list) {
        encoder_map[info->name] = new LinearEncoder(info->grammar, spec->env);
    }
    auto* cbs = new ComponentBasedSynthesizer(spec, encoder_map);
    auto* solver = new CEGISSolver(cbs, v);

    auto res = solver->synthesis(guard);
    return res;
}

FunctionContext BasicEnumInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);
    auto* solver = new BasicEnumSolver(spec, v);
    auto res = solver->synthesis(guard);
    return res;
}

FunctionContext OBEInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);
    ProgramChecker runnable = [](Program* p) {return true;};

    auto* obe = new OBESolver(spec, v, runnable);
    auto* solver = new CEGISSolver(obe, v);

    auto res = solver->synthesis(guard);
    return res;
}

FunctionContext EuSolverInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env);
    auto* solver = new EuSolver(spec, stun_info.first, stun_info.second);
    auto* cegis = new CEGISSolver(solver, v);

    return cegis->synthesis(guard);
}

FunctionContext BasicVSAInvoker(Specification* spec, TimeGuard* guard) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "VSASolver can only synthesize a single program";
    }
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
    auto* builder = new BFSVSABuilder(info->grammar, pruner, spec->env);
    // auto* builder = new DFSVSABuilder(info->grammar, pruner, spec->env);

    sygus::loadSyGuSTheories(spec->env, theory::loadWitnessFunction);

    auto prepare = [](Specification* spec, const IOExample& io_example) {
        auto* env = spec->env; auto* g = spec->info_list[0]->grammar;
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

    auto res = cegis->synthesis(guard);
    return res;
}

int main() {
    std::string file = config::KSourcePath + "/tests/array_search_4.sl";
    // std::string file = config::KSourcePath + "/tests/phone-1.sl";
    auto* spec = parser::getSyGuSSpecFromFile(file);
    auto* guard = new TimeGuard(100);
    auto res = EuSolverInvoker(spec, guard);
    std::cout << res.toString() << std::endl;
    std::cout << "Time Cost: " << guard->getPeriod() << " seconds";
    return 0;
}
