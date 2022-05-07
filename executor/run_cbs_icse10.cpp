//
// Created by pro on 2022/2/13.
//

#include "istool/dsl/component/component_dataset.h"
#include "istool/ext/z3/z3_verifier.h"
#include "istool/selector/selector.h"
#include "istool/invoker/invoker.h"
#include "glog/logging.h"
#include "istool/basic/bitset.h"
#include "istool/sygus/theory/basic/bv/bv.h"
#include "istool/selector/split/z3_splitor.h"
#include "istool/selector/split/split_selector.h"
#include "istool/solver/maxflash/topdown_context_graph.h"
#include "istool/selector/random/grammar_flatter.h"
#include "istool/selector/random/random_semantics_selector.h"
#include "istool/ext/vsa/top_down_model.h"
#include <unordered_set>

const int KTryNum = 1000;

Data getRandBitVec(int size) {
    Bitset res(size, 0);
    for (int i = 0; i < size; ++i) res.set(i, rand() & 1);
    return BuildData(BitVector, res);
}

Example getRandBitVec(const std::vector<int>& size_list) {
    Example res;
    for (auto size: size_list) res.push_back(getRandBitVec(size));
    return res;
}

std::vector<int> extractSizeList(Z3ExampleSpace* example_space) {
    std::vector<int> res;
    for (auto& type: example_space->type_list) {
        auto* bt = dynamic_cast<TBitVector*>(type.get());
        assert(bt);
        res.push_back(bt->size);
    }
    return res;
}

class BVRandomVerifier: public Verifier {
    Verifier* v;
    Z3ExampleSpace* example_space;
    std::vector<int> size_list;
public:
    BVRandomVerifier(Verifier* _v, Z3ExampleSpace* _example_space): v(_v), example_space(_example_space) {
        size_list = extractSizeList(example_space);
    }
    virtual bool verify(const FunctionContext& info, Example* counter_example) {
        if (!counter_example) return v->verify(info, counter_example);
        for (int _ = 0; _ < KTryNum; ++_) {
            Example example = getRandBitVec(size_list);
            if (!example_space->satisfyExample(info, example)) {
                *counter_example = example; return false;
            }
        }
        return v->verify(info, counter_example);
    }
};

const int KBitNum = 2;
class BVPriorVerifier: public Verifier {
    Verifier* v;
    Z3ExampleSpace* example_space;
    std::vector<int> size_list;
    int cnt = -1;
public:
    BVPriorVerifier(Verifier* _v, Z3ExampleSpace* _example_space): v(_v), example_space(_example_space) {
        size_list = extractSizeList(example_space);
    }
    virtual bool verify(const FunctionContext& info, Example* counter_example) {
        ++cnt;
        if (!counter_example || cnt >= (1 << KBitNum)) return v->verify(info, counter_example);
        for (int _ = 0; _ < KTryNum; ++_) {
            Example example = getRandBitVec(size_list);
            for (int i = 0; i < example.size(); ++i) {
                auto w = theory::bv::getBitVectorValue(example[i]);
                for (int i = 0; i < KBitNum; ++i) {
                    if (cnt & (1 << i)) w.set(i, 1); else w.set(i, 0);
                }
                example[i] = BuildData(BitVector, w);
            }
            if (!example_space->satisfyExample(info, example)) {
                *counter_example = example; return false;
            }
        }
        return v->verify(info, counter_example);
    }
};

bool checkUnique(Program* p, std::unordered_set<std::string>& cache) {
    auto* s = p->semantics.get();
    if (dynamic_cast<ParamSemantics*>(s) || dynamic_cast<ConstSemantics*>(s)) return true;
    if (cache.find(s->getName()) != cache.end()) return false;
    cache.insert(s->getName());
    for (auto& sub: p->sub_list) {
        if (!checkUnique(sub.get(), cache)) return false;
    }
    return true;
}

class UniqueVerifier: public Verifier {
public:
    virtual bool verify(const FunctionContext& info, Example* counter_example) {
        assert(!counter_example && info.size() == 1);
        auto* p = info.begin()->second.get();
        std::unordered_set<std::string> cache;
        return checkUnique(p, cache);
    }
};

Verifier* getSplitVerifier(Specification* spec, int num) {
    auto* splitor = new Z3Splitor(spec->example_space.get(), spec->info_list[0]->oup_type, spec->info_list[0]->inp_type_list);
    return new SplitSelector(splitor, spec->info_list[0], num, spec->env.get(), new UniqueVerifier());
}


Verifier* getRandomSemanticsVerifier(Specification* spec, int num) {
    auto* model = ext::vsa::getSizeModel();
    auto* graph = new TopDownContextGraph(grammar::generateHeightLimitedGrammar(spec->info_list[0]->grammar, 10), model, ProbModelType::NORMAL_PROB);
    auto* fg = selector::getFlattenGrammar(graph, num, [](Program *p) { return true; });
    auto* scorer = new RandomSemanticsScorer(spec->env.get(), fg, 5);
    if (dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get())) {
        return new Z3RandomSemanticsSelector(spec, scorer, 10);
    } else {
        return new FiniteRandomSemanticsSelector(spec, scorer);
    }
}

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    int benchmark_id;
    std::string output_name, verifier_name;
    if (argc == 4) {
        benchmark_id = std::atoi(argv[1]);
        output_name = argv[2];
        verifier_name = argv[3];
    } else {
        benchmark_id = 6;
        output_name = "/tmp/629453237.out";
        verifier_name = "random200";
    }

    auto* spec = dsl::component::getTask(benchmark_id);

    auto* example_space = dynamic_cast<Z3ExampleSpace*>(spec->example_space.get());
    Verifier* v = new Z3Verifier(dynamic_cast<Z3ExampleSpace*>(spec->example_space.get()));
    if (verifier_name == "random") v = new BVRandomVerifier(v, example_space);
    else if (verifier_name == "prior") v = new BVPriorVerifier(v, example_space);
    else if (verifier_name == "splitor50") v = getSplitVerifier(spec, 50);
    else if (verifier_name == "splitor200") v = getSplitVerifier(spec, 200);
    else if (verifier_name == "random200") v = getRandomSemanticsVerifier(spec, 200);
    else if (verifier_name == "random2000") v = getRandomSemanticsVerifier(spec, 2000);
    auto* guard = new TimeGuard(1e9);
    auto res = invoker::getExampleNum(spec, v, SolverToken::COMPONENT_BASED_SYNTHESIS, guard);

    LOG(INFO) << res.first << " " << res.second.toString();
    LOG(INFO) << guard->getPeriod();
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", res.first, res.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}