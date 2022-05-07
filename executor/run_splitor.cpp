//
// Created by pro on 2022/2/15.
//

#include "istool/selector/split/splitor.h"
#include "istool/selector/split/finite_splitor.h"
#include "istool/selector/split/z3_splitor.h"
#include "istool/selector/split/split_selector.h"
#include "istool/invoker/invoker.h"
#include "istool/sygus/sygus.h"
#include "istool/sygus/parser/parser.h"
#include "istool/ext/z3/z3_example_space.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/dsl/samplesy/samplesy_dsl.h"
#include "istool/ext/vsa/vsa_extension.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/selector/random/random_semantics_selector.h"
#include <ctime>

int KTryNum = 1000, KIntMax = 20;

Data _generateData(Type* type, Env* env) {
    if (dynamic_cast<TBool*>(type)) {
        std::bernoulli_distribution dis;
        return BuildData(Bool, dis(env->random_engine));
    } else {
        assert(dynamic_cast<TInt*>(type));
        std::uniform_int_distribution<int> dis(-KIntMax, KIntMax);
        return BuildData(Int, dis(env->random_engine));
    }
}

class RandomVerifier: public Verifier {
public:
    Verifier* v;
    ExampleSpace* space;
    Env* env;
    RandomVerifier(Verifier* _v, ExampleSpace* _space, Env* _env): v(_v), space(_space), env(_env) {
        assert(space);
    }
    virtual bool verify(const FunctionContext& info, Example* counter_example) {
        auto* finite_space = dynamic_cast<FiniteIOExampleSpace*>(space);
        auto p = info.begin()->second;
        if (finite_space) {
            std::uniform_int_distribution<int> dis(0, int(finite_space->example_space.size()) - 1);
            for (int _ = 0; _ < KTryNum; ++_) {
                auto example = finite_space->example_space[dis(env->random_engine)];
                if (!space->satisfyExample(info, example)) {
                    *counter_example = example; return false;
                }
            }
        } else {
            auto* z3_space = dynamic_cast<Z3IOExampleSpace*>(space);
            assert(z3_space);
            for (int _ = 0; _ < KTryNum; ++_) {
                Example example;
                for (auto& type: z3_space->type_list) example.push_back(_generateData(type.get(), env));
                if (!space->satisfyExample(info, example)) {
                    *counter_example = example; return false;
                }
            }
        }
        return v->verify(info, counter_example);
    }
};

Verifier* _buildRandomVerifier(Specification* spec, Verifier* v) {
    return new RandomVerifier(v, spec->example_space.get(), spec->env.get());
}

class VSAOptimizer: public Optimizer {
public:
    Grammar* g;
    VSAExtension* ext;
    IOExampleList example_list;
    VSAOptimizer(Specification* spec) {
        g = spec->info_list[0]->grammar;
        ext = ext::vsa::getExtension(spec->env.get());
        auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
        for (auto& example: fio->example_space) {
            example_list.push_back(fio->getIOExample(example));
        }
    }
    virtual bool isDuplicated(const std::string& name, NonTerminal* nt, const PProgram& p) {
        for (auto& example: example_list) {
            if (!ext::vsa::isConsideredByVSA(p.get(), ext, g, example)) {
                return true;
            }
        }
        return false;
    }
    virtual void clear() {}
};

class NonDuplicatedVerifier: public Verifier {
public:
    std::unordered_set<std::string> cache;
    virtual bool verify(const FunctionContext& info, Example* counter_example) {
        auto p = info.begin()->second;
        auto feature = p->toString();
        if (cache.find(feature) != cache.end()) return false;
        cache.insert(feature);
        return true;
    }
};


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

Verifier* _buildSplitVerifier(Specification* spec, int num, const std::string& solver_name) {
    Splitor* splitor = nullptr;
    Optimizer* o = nullptr;
    Verifier* v = nullptr;
    if (solver_name == "vsa" || solver_name == "maxflash") {
        auto* ext = ext::vsa::getExtension(spec->env.get());
        ext->setEnvSetter(KStringPrepare);
        o = new VSAOptimizer(spec);
        v = new NonDuplicatedVerifier();
    }
    if (dynamic_cast<Z3ExampleSpace*>(spec->example_space.get())) {
        splitor = new Z3Splitor(spec->example_space.get(), spec->info_list[0]->oup_type,
                                spec->info_list[0]->inp_type_list);
    } else {
        assert(dynamic_cast<FiniteExampleSpace*>(spec->example_space.get()));
        splitor = new FiniteSplitor(spec->example_space.get());
    }
    return new SplitSelector(splitor, spec->info_list[0], num, spec->env.get(), v, o);
}


Verifier* _buildRandomsSemanticsVerifier(Specification* spec, int num, const std::string& name) {
    auto* model = ext::vsa::getSizeModel();
    auto* graph = new TopDownContextGraph(grammar::generateHeightLimitedGrammar(spec->info_list[0]->grammar, 10), model, ProbModelType::NORMAL_PROB);
    FlattenGrammar* fg = nullptr;
    if (name == "maxflash" || name == "vsa") {
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
        fg = selector::getFlattenGrammar(graph, num, validator);
    } else {
        fg = selector::getFlattenGrammar(graph, num, [](Program *p) { return true; });
    }
    auto* scorer = new RandomSemanticsScorer(spec->env.get(), fg, 5);
    if (dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get())) {
        return new Z3RandomSemanticsSelector(spec, scorer, 10);
    } else {
        return new FiniteRandomSemanticsSelector(spec, scorer);
    }
}

int main(int argc, char** argv) {
    assert(argc == 5 || argc == 1);
    std::string benchmark_name, output_name, solver_name, verifier_name;
    if (argc == 5) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
        verifier_name = argv[4];
    } else {
        solver_name = "maxflash";
        benchmark_name = " /tmp/tmp.wHOuYKwdWN/tests/string/phone-1.sl";
        //benchmark_name = "/tmp/tmp.wHOuYKwdWN/tests/clia/array_search_5.sl";
        output_name = "/tmp/629453237.out";
        verifier_name = "random2000";
    }
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    if (sygus::getSyGuSTheory(spec->env.get()) == TheoryToken::STRING) {
        samplesy::registerSampleSyBasic(spec->env.get());
        samplesy::registerSampleSyWitness(spec->env.get());
        auto& info = spec->info_list[0];
        info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(), dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get()));
    }
    auto* v = sygus::getVerifier(spec);
    if (verifier_name == "default") ;
    else if (verifier_name == "random") {
        v = _buildRandomVerifier(spec, v);
    } else if (verifier_name == "splitor50") {
        v = _buildSplitVerifier(spec, 500, solver_name);
    } else if (verifier_name == "splitor200") {
        v = _buildSplitVerifier(spec, 200, solver_name);
    } else if (verifier_name == "random200") {
        v = _buildRandomsSemanticsVerifier(spec, 200, solver_name);
    } else if (verifier_name == "random2000") {
        v = _buildRandomsSemanticsVerifier(spec, 2000, solver_name);
    }
    spec->env->random_engine.seed(time(0));
    auto solver_token = invoker::string2TheoryToken(solver_name);
    auto* guard = new TimeGuard(1e9);
    auto result = invoker::getExampleNum(spec, v, solver_token, guard);
    std::cout << result.first << " " << result.second.toString() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}