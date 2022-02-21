//
// Created by pro on 2022/1/9.
//

#include "istool/basic/config.h"
#include <istool/sygus/sygus.h>
#include <istool/solver/polygen/lia_solver.h>
#include <istool/solver/polygen/dnf_learner.h>
#include <istool/solver/stun/stun.h>
#include <istool/solver/polygen/polygen_cegis.h>
#include <istool/selector/split/z3_splitor.h>
#include <istool/selector/split/split_selector.h>
#include <istool/selector/samplesy/finite_equivalence_checker.h>
#include <istool/selector/samplesy/vsa_seed_generator.h>
#include "istool/solver/vsa/vsa_solver.h"
#include "istool/solver/maxflash/maxflash.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/witness/theory_witness.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include "istool/selector/baseline/clia_random_selector.h"
#include "istool/selector/split/finite_splitor.h"
#include "istool/ext/composed_semantics/composed_witness.h"
#include "istool/dsl/samplesy/samplesy_dsl.h"
#include "istool/selector/samplesy/samplesy.h"
#include "istool/selector/finite_random_selector.h"
#include "istool/solver/enum/enum_solver.h"

typedef std::pair<int, FunctionContext> SynthesisResult;

SynthesisResult invokeBasicPolyGen(Specification* spec, bool is_random=false) {
    auto *v = is_random ? new CLIARandomSelector(spec->env.get(), dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get())) : sygus::getVerifier(spec);
    auto *sv = new DirectSelector(v);
    auto domain_builder = solver::lia::liaSolverBuilder;
    auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* solver = new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, sv);

    return {sv->example_count, solver->synthesis(nullptr)};
}

SynthesisResult invokeSplitPolyGen(Specification* spec, bool is_100ms=false) {
    if (is_100ms) selector::setSplitorTimeOut(spec->env.get(), 0.1);
    auto* splitor = new Z3Splitor(spec->example_space.get(), spec->info_list[0]->oup_type, spec->info_list[0]->inp_type_list);
    auto* v = new SplitSelector(splitor, spec->info_list[0], 50, spec->env.get());
    auto domain_builder = solver::lia::liaSolverBuilder;
    auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* solver = new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, v);
    auto res = solver->synthesis(nullptr);
    return {v->example_count, res};
}

auto prepare = [](Grammar* g, Env* env, const IOExample& io_example) {
    DataList string_const_list, string_input_list;
    std::unordered_set<std::string> const_set;
    for (auto* symbol: g->symbol_list) {
        for (auto* rule: symbol->rule_list) {
            auto* sem = dynamic_cast<ConstSemantics*>(rule->semantics.get());
            if (sem) {
                auto* sv = dynamic_cast<StringValue*>(sem->w.get());
                if (!sv) continue;
                if (const_set.find(sv->s) == const_set.end()) {
                    const_set.insert(sv->s);
                    string_const_list.push_back(sem->w);
                }
            }
        }
    }
    for (const auto& inp: io_example.first) {
        auto* sv = dynamic_cast<StringValue*>(inp.get());
        if (sv) string_input_list.push_back(inp);
    }

    int int_max = 1;
    for (const auto& s: string_const_list) {
        int_max = std::max(int_max, int(theory::string::getStringValue(s).length()));
    }
    for (const auto& s: string_input_list) {
        int_max = std::max(int_max, int(theory::string::getStringValue(s).length()));
    }
    for (const auto& inp: io_example.first) {
        auto* iv = dynamic_cast<IntValue*>(inp.get());
        if (iv) int_max = std::max(int_max, iv->w);
    }
    int_max = 20;

    env->setConst(theory::clia::KWitnessIntMinName, BuildData(Int, -int_max));
    env->setConst(theory::string::KStringConstList, string_const_list);
    env->setConst(theory::string::KStringInputList, string_input_list);
    env->setConst(theory::clia::KWitnessIntMaxName, BuildData(Int, int_max));
};

SynthesisResult InvokeVanillaVSA(Specification* spec) {
    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);
    auto info = spec->info_list[0];
    auto* pruner = new SizeLimitPruner(1e5, [](VSANode* node) {
        auto* sn = dynamic_cast<SingleVSANode*>(node);
        if (sn) {
            auto* oup = dynamic_cast<DirectWitnessValue*>(sn->oup.get());
            if (oup) {
                auto* w = dynamic_cast<StringValue*>(oup->d.get());
                if (w) return w->s.length() >= 20;
            }
        }
        return false;
    });

    auto* ext = ext::vsa::getExtension(spec->env.get());
    ext->setEnvSetter(prepare);
    auto builder = std::make_shared<BFSVSABuilder>(info->grammar, pruner, spec->env.get());

    auto* selector = new VSAMinimalProgramSelector(ext::vsa::getSizeModel());
    auto* solver = new BasicVSASolver(spec, builder, selector);
    auto* verifier = sygus::getVerifier(spec);
    auto* sv = new DirectSelector(verifier);
    auto* cegis = new CEGISSolver(solver, sv);

    auto res = cegis->synthesis(nullptr);
    return {sv->example_count, res};
}


SynthesisResult invokeMaxFlash(Specification* spec) {
    auto* v = sygus::getVerifier(spec);
    auto* sv = new DirectSelector(v);

    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);
    ext::vsa::registerDefaultComposedManager(ext::vsa::getExtension(spec->env.get()));

    //std::string model_path = config::KSourcePath + "ext/vsa/model.json";
    //auto* model = ext::vsa::loadDefaultNGramModel(model_path);
    auto* model = ext::vsa::getSizeModel();
    auto* solver = new MaxFlash(spec, sv, model, prepare);
    auto res = solver->synthesis(nullptr);
    return {sv->example_count, res};
}

SynthesisResult invokeSplitMaxFlash(Specification* spec) {
    auto* splitor = new FiniteSplitor(spec->example_space.get());
    auto* v = new SplitSelector(splitor, spec->info_list[0], 1000, spec->env.get());
    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);
    ext::vsa::registerDefaultComposedManager(ext::vsa::getExtension(spec->env.get()));

    std::string model_path = config::KSourcePath + "ext/vsa/model.json";
    auto* model = ext::vsa::loadDefaultNGramModel(model_path);
    auto* solver = new MaxFlash(spec, v, model, prepare);
    auto res = solver->synthesis(nullptr);
    return {v->example_count, res};
}

SynthesisResult invokeSampleSy(Specification* spec) {
    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);
    auto info = spec->info_list[0];
    samplesy::registerSampleSyBasic(spec->env.get());
    samplesy::registerSampleSyWitness(spec->env.get());
    info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(), dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get()));

    auto* ext = ext::vsa::getExtension(spec->env.get());
    ext->setEnvSetter(prepare);

    auto* pruner = new TrivialPruner();
    auto* limited_g = grammar::generateHeightLimitedGrammar(info->grammar, 6);
    auto builder = std::make_shared<DFSVSABuilder>(limited_g, pruner, spec->env.get());
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    assert(fio);

    auto* diff_gen = new VSADifferentProgramGenerator(builder);
    auto* checker = new FiniteEquivalenceChecker(diff_gen, fio);

    diff_gen = new VSADifferentProgramGenerator(builder);
    auto* gen = new FiniteVSASeedGenerator(builder, new VSASizeBasedSampler(spec->env.get()), diff_gen, fio);
    auto* splitor = new FiniteSplitor(fio);
    auto* solver = new SampleSy(spec, splitor, gen, checker);
    auto res = solver->synthesis(nullptr);
    return {solver->example_count, res};
}

SynthesisResult invokeRandomSy(Specification* spec) {
    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);
    auto info = spec->info_list[0];
    samplesy::registerSampleSyBasic(spec->env.get());
    samplesy::registerSampleSyWitness(spec->env.get());
    info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(), dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get()));

    auto* pruner = new TrivialPruner();
    auto* limited_g = grammar::generateHeightLimitedGrammar(info->grammar, 6);
    auto* ext = ext::vsa::getExtension(spec->env.get());
    ext->setEnvSetter(prepare);
    auto builder = std::make_shared<DFSVSABuilder>(limited_g, pruner, spec->env.get());
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    assert(fio);
    auto* diff_gen = new VSADifferentProgramGenerator(builder);
    auto* checker = new FiniteEquivalenceChecker(diff_gen, fio);
    auto* solver = new FiniteRandomSelector(spec, checker);
    auto res = solver->synthesis(nullptr);
    return {solver->example_count, res};
}

SynthesisResult invokeCompleteSplitor(Specification* spec) {
    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);
    auto info = spec->info_list[0];
    samplesy::registerSampleSyBasic(spec->env.get());
    samplesy::registerSampleSyWitness(spec->env.get());
    info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(), dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get()));

    auto* pruner = new TrivialPruner();
    auto* limited_g = grammar::generateHeightLimitedGrammar(info->grammar, 6);
    auto* ext = ext::vsa::getExtension(spec->env.get());
    ext->setEnvSetter(prepare);
    auto builder = std::make_shared<DFSVSABuilder>(limited_g, pruner, spec->env.get());
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    assert(fio);
    auto* diff_gen = new VSADifferentProgramGenerator(builder);
    auto* checker = new FiniteEquivalenceChecker(diff_gen, fio);
    auto* splitor = new FiniteSplitor(fio);
    auto* solver = new CompleteSplitSelector(spec, splitor, checker, 5000);
    auto res = solver->synthesis(nullptr);
    return {solver->example_count, res};
}

SynthesisResult InvokeOBE(Specification* spec) {
    auto* v = sygus::getVerifier(spec);
    auto* sv = new DirectSelector(v);
    spec->info_list[0]->grammar->print();
    ProgramChecker runnable = [](Program* p) {return true;};

    auto* obe = new OBESolver(spec, sv, runnable);
    auto* solver = new CEGISSolver(obe, sv);

    auto res = solver->synthesis(nullptr);
    return {sv->example_count, res};
}


int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    std::string benchmark_name, output_name, solver_name;
    if (argc == 4) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
    } else {
        // benchmark_name = "/tmp/tmp.i5X31IAVA3/tests/string/name-combine-3-long.sl";
        solver_name = "vsa";
        benchmark_name = "/tmp/tmp.i5X31IAVA3/tests/testfail.sl";
        //solver_name = "maxflash";
        output_name = "/tmp/629453237.out";
    }
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    SynthesisResult result;
    auto *guard = new TimeGuard(1e9);
    if (solver_name == "maxflash") {
        result = invokeMaxFlash(spec);
    } else if (solver_name == "mselect") {
        result = invokeSplitMaxFlash(spec);
    } else if (solver_name == "polygen") {
        result = invokeBasicPolyGen(spec);
    } else if (solver_name == "prand") {
        result = invokeBasicPolyGen(spec, true);
    } else if (solver_name == "pselect") {
        result = invokeSplitPolyGen(spec, false);
    } else if (solver_name == "pselect0.1") {
        result = invokeSplitPolyGen(spec, true);
    } else if (solver_name == "samplesy") {
        result = invokeSampleSy(spec);
    } else if (solver_name == "splitor") {
        result = invokeCompleteSplitor(spec);
    } else if (solver_name == "randomsy") {
        result = invokeRandomSy(spec);
    } else if (solver_name == "vsa") {
        result = InvokeVanillaVSA(spec);
    } else if (solver_name == "obe") {
        result = InvokeOBE(spec);
    }
    std::cout << result.first << " " << result.second.toString() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}