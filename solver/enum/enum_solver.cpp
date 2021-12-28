//
// Created by pro on 2021/12/27.
//

#include <algorithm>
#include "istool/solver/enum/enum_solver.h"
#include "glog/logging.h"

void TrivialOptimizer::clear() {}
bool TrivialOptimizer::isDuplicated(const std::string& name, NonTerminal *nt, const PProgram &p) {
    return false;
}

BasicEnumSolver::BasicEnumSolver(Specification *_spec, Verifier *_v): Solver(_spec), v(_v) {
}
BasicEnumSolver::~BasicEnumSolver() {
    delete v;
}
FunctionContext BasicEnumSolver::synthesis(TimeGuard* guard) {
    auto* o = new TrivialOptimizer();
    EnumConfig c(v, o, guard);
    auto res = solver::enumerate(spec->info_list, c);
    if (res.empty()) {
        LOG(FATAL) << "Synthesis failed";
    }
    return res[0];
}

OBEOptimizer::OBEOptimizer(const ProgramChecker &_is_runnable, const std::unordered_map<std::string, ExampleList> &_pool):
    is_runnable(_is_runnable), example_pool(_pool) {
}

bool OBEOptimizer::isDuplicated(const std::string& name, NonTerminal *nt, const PProgram &p) {
    if (!is_runnable(p.get()) || example_pool.find(name) == example_pool.end()) return false;
    auto& example_list = example_pool[name];
    DataList res;
    for (auto& example: example_list) {
        res.push_back(program::run(p.get(), example));
    }
    std::string feature = std::to_string(nt->id) + "@" + data::dataList2String(res);
    if (visited_set.find(feature) != visited_set.end()) return true;
    visited_set.insert(feature);
    return false;
}

void OBEOptimizer::clear() {
    visited_set.clear();
}

namespace {
    void collectAllInvocation(const PProgram& program, std::unordered_map<std::string, ProgramList>& invoke_map) {
        auto* is = dynamic_cast<InvokeSemantics*>(program->semantics.get());
        if (is) {
            invoke_map[is->name].push_back(program);
        }
        for (const auto& sub: program->sub_list) {
            collectAllInvocation(sub, invoke_map);
        }
    }

    bool isGrounded(const PProgram& program) {
        auto* is = dynamic_cast<InvokeSemantics*>(program->semantics.get());
        if (is) return false;
        for (const auto& sub: program->sub_list) {
            if (!isGrounded(sub)) return false;
        }
        return true;
    }
}

OBESolver::OBESolver(Specification *_spec, Verifier* _v, const ProgramChecker &_is_runnable): PBESolver(_spec), v(_v), is_runnable(_is_runnable) {
    std::unordered_map<std::string, ProgramList> raw_invoke_map;
    collectAllInvocation(spec->example_space->cons_program, raw_invoke_map);
    for (auto& collect_info: raw_invoke_map) {
        auto name = collect_info.first;
        bool is_grounded = true;
        for (auto& p: collect_info.second) {
            for (auto& sub: p->sub_list) {
                if (!isGrounded(sub) || !is_runnable(sub.get())) {
                    is_grounded = false;
                    LOG(WARNING) << "Observational equivalence cannot be applied to function " << name <<
                                    " because invocation " << p->toString() << " is not grounded";
                    break;
                }
            }
            if (!is_grounded) break;
        }
        if (is_grounded) {
            invoke_map[name] = collect_info.second;
        }
    }
}

FunctionContext OBESolver::synthesis(const std::vector<Example> &example_list, TimeGuard* guard) {
    std::unordered_map<std::string, ExampleList> example_pool;
    for (auto& invoke_info: invoke_map) {
        std::string name = invoke_info.first;
        std::unordered_set<std::string> feature_set;
        ExampleList res;
        for (const auto& p: invoke_info.second) {
            for (const auto& example: example_list) {
                Example invoke_example;
                for (const auto& sub: p->sub_list) {
                    invoke_example.push_back(program::run(sub.get(), example));
                }
                auto feature = data::dataList2String(invoke_example);
                if (feature_set.find(feature) == feature_set.end()) {
                    res.push_back(invoke_example);
                    feature_set.insert(feature);
                }
            }
        }
        std::random_shuffle(res.begin(), res.end());
        example_pool[name] = res;
    }

    auto* obe_optimizer = new OBEOptimizer(is_runnable, example_pool);
    auto* finite_example_space = new FiniteExampleSpace(spec->example_space->cons_program, example_list);
    auto* finite_verifier = new FiniteExampleVerifier(finite_example_space);

    EnumConfig c(finite_verifier, obe_optimizer, guard);

    auto res = solver::enumerate(spec->info_list, c);
    if (res.empty()) {
        LOG(FATAL) << "Synthesis failed";
    }

    delete obe_optimizer;
    delete finite_example_space;
    delete finite_verifier;
    return res[0];
}