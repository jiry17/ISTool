//
// Created by pro on 2022/5/8.
//

#include <cassert>
#include "istool/basic/specification.h"
#include "istool/invoker/invoker.h"
#include "istool/sygus/parser/parser.h"
#include "istool/selector/samplesy/samplesy.h"
#include "istool/selector/split/finite_splitor.h"
#include "istool/dsl/samplesy/samplesy_dsl.h"
#include "istool/solver/vsa/vsa_builder.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include <unordered_set>
#include "istool/selector/finite_random_selector.h"
#include "istool/selector/samplesy/finite_equivalence_checker.h"
#include "istool/selector/samplesy/vsa_seed_generator.h"
#include "istool/selector/samplesy/different_program_generator.h"
#include "istool/selector/random/random_semantics_selector.h"

typedef std::pair<int, FunctionContext> SynthesisResult;

int getGrammarDepth(Specification* spec) {
    auto theory = sygus::getSyGuSTheory(spec->env.get());
    if (theory == TheoryToken::STRING) return 6;
    else if (theory == TheoryToken::CLIA) return 4;
    LOG(FATAL) << "GrammarDepth undefined for the current theory";
}

void prepareSampleSy(Specification* spec) {
    int depth = getGrammarDepth(spec);
    auto& info = spec->info_list[0];
    samplesy::registerSampleSyBasic(spec->env.get());
    samplesy::registerSampleSyWitness(spec->env.get());
    if (sygus::getSyGuSTheory(spec->env.get()) == TheoryToken::STRING) {
        info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(),
                                                 dynamic_cast<FiniteIOExampleSpace *>(spec->example_space.get()));
    }
    info->grammar = grammar::generateHeightLimitedGrammar(info->grammar, depth);
}

auto KStringPrepare = [](Grammar* g, Env* env, const IOExample& io_example) {
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

    env->setConst(theory::clia::KWitnessIntMinName, BuildData(Int, -int_max));
    env->setConst(theory::string::KStringConstList, string_const_list);
    env->setConst(theory::string::KStringInputList, string_input_list);
    env->setConst(theory::clia::KWitnessIntMaxName, BuildData(Int, int_max));
};

Splitor* getSplitor(Specification* spec) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) return new FiniteSplitor(fio);
    LOG(FATAL) << "Unsupported type of ExampleSpace for the Splitor";
}

EquivalenceChecker* getEquivalenceChecker(Specification* spec, const PVSABuilder& builder) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) {
        auto* diff_gen = new VSADifferentProgramGenerator(builder);
        return new FiniteEquivalenceChecker(diff_gen, fio);
    }
    LOG(FATAL) << "Unsupported type of ExampleSpace for the EquivalenceChecker";
}

SeedGenerator* getSeedGenerator(Specification* spec, const PVSABuilder& builder) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) {
        auto* diff_gen = new VSADifferentProgramGenerator(builder);
        return new FiniteVSASeedGenerator(builder, new VSASizeBasedSampler(spec->env.get()), diff_gen, fio);
    }
    LOG(FATAL) << "Unsupported type of ExampleSpace for the SeedGenerator";
}

SynthesisResult invokeSampleSy(Specification* spec, TimeGuard* guard) {
    auto* pruner = new TrivialPruner();
    auto& info = spec->info_list[0];
    auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
    vsa_ext->setEnvSetter(KStringPrepare);
    auto builder = std::make_shared<DFSVSABuilder>(info->grammar, pruner, spec->env.get());
    info->grammar->print();
    auto* splitor = getSplitor(spec);
    auto* checker = getEquivalenceChecker(spec, builder);
    auto* gen = getSeedGenerator(spec, builder);
    auto* solver = new SampleSy(spec, splitor, gen, checker);
    auto res = solver->synthesis(guard);
    return {solver->example_count, res};
}

SynthesisResult invokeRandomSy(Specification* spec, TimeGuard* guard) {
    auto* pruner = new TrivialPruner();
    auto& info = spec->info_list[0];
    auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
    vsa_ext->setEnvSetter(KStringPrepare);
    auto builder = std::make_shared<DFSVSABuilder>(info->grammar, pruner, spec->env.get());
    auto* checker = getEquivalenceChecker(spec, builder);
    auto* solver = new FiniteRandomSelector(spec, checker);
    auto res = solver->synthesis(nullptr);
    return {solver->example_count, res};
}

FlattenGrammar* getFlattenGrammar(Specification* spec, TopDownContextGraph* graph, int num) {
    auto theory = sygus::getSyGuSTheory(spec->env.get());
    if (theory == TheoryToken::STRING) {
        auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
        vsa_ext->setEnvSetter(KStringPrepare);
        auto* example_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
        IOExampleList io_list;
        for (auto& example: example_space->example_space) {
            io_list.push_back(example_space->getIOExample(example));
        }
        auto* g = spec->info_list[0]->grammar;
        auto validator = [vsa_ext, io_list, g](Program* p) -> bool {
            for (auto& example: io_list) {
                if (!ext::vsa::isConsideredByVSA(p, vsa_ext, g, example)) return false;
            }
            return true;
        };
        return selector::getFlattenGrammar(graph, num, validator);
    } else {
        return selector::getFlattenGrammar(graph, num, [](Program *p) { return true; });
    }
}

SynthesisResult invokeRandomSemanticsSelector(Specification* spec, int num, TimeGuard* guard) {
    auto* pruner = new TrivialPruner();
    auto& info = spec->info_list[0];
    auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
    vsa_ext->setEnvSetter(KStringPrepare);
    auto builder = std::make_shared<DFSVSABuilder>(info->grammar, pruner, spec->env.get());
    auto* checker = getEquivalenceChecker(spec, builder);

    auto model = ext::vsa::getSizeModel();
    auto* graph = new TopDownContextGraph(info->grammar, model, ProbModelType::NORMAL_PROB);
    auto* fg = getFlattenGrammar(spec, graph, num);
    auto* scorer = new RandomSemanticsScorer(spec->env.get(), fg, 5);

    auto* diff_gen = new VSADifferentProgramGenerator(builder);
    auto* solver = new FiniteCompleteRandomSemanticsSelector(spec, checker, scorer, diff_gen);
    auto res = solver->synthesis(guard);
    return {solver->example_count, res};
}

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    std::string benchmark_name, output_name, solver_name;
    if (argc == 4) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
    } else {
        solver_name = "random2000";
        benchmark_name = " /tmp/tmp.wHOuYKwdWN/tests/string/phone-3.sl";
        output_name = "/tmp/629453237.out";
    }
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    spec->env->random_engine.seed(time(0));
    prepareSampleSy(spec);
    SynthesisResult result;
    auto* guard = new TimeGuard(1e9);
    if (solver_name == "samplesy") {
        result = invokeSampleSy(spec, guard);
    } else if (solver_name == "randomsy") {
        result = invokeRandomSy(spec, guard);
    } else if (solver_name == "random200") {
        result = invokeRandomSemanticsSelector(spec, 200, guard);
    } else if (solver_name == "random2000") {
        result = invokeRandomSemanticsSelector(spec, 2000, guard);
    }
    std::cout << result.first << " " << result.second.toString() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}