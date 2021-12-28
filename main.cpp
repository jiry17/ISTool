#include <iostream>

#include "z3++.h"
#include "istool/basic/config.h"
#include "istool/sygus/sygus.h"
#include "istool/solver/solver.h"
#include "istool/solver/component/component_based_solver.h"
#include "istool/solver/component/linear_encoder.h"
#include "istool/solver/enum/enum_solver.h"

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

int main() {
    std::string file = config::KSourcePath + "/max2.sl";
    auto* spec = parser::getSyGuSSpecFromFile(file);
    auto* guard = new TimeGuard(200);
    auto res = CBSInvoker(spec, guard);
    std::cout << res.toString() << std::endl;
    std::cout << "Time Cost: " << guard->getPeriod() << " seconds";
    return 0;
}
