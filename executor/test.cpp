//
// Created by pro on 2022/5/1.
//

#include <istool/dsl/samplesy/samplesy_dsl.h>
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/selector/random/random_semantics_scorer_tester.h"
#include "istool/selector/random/learned_scorer.h"
#include "istool/selector/random/random_semantics_scorer.h"
#include "istool/selector/random/grammar_flatter.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/parser/parser.h"
#include "istool/ext/vsa/top_down_model.h"
#include "glog/logging.h"

DataList readSingleExample(int m) {
    DataList inp;
    for (int i = 0; i < m; ++i) {
        int k; scanf("%d", &k);
        inp.push_back(BuildData(Int, k));
    }
    return inp;
}

DataStorage readExample(int m) {
    int n; scanf("%d", &n);
    DataStorage inp_storage;
    for (; n; n--) {
        inp_storage.push_back(readSingleExample(m));
    }
    return inp_storage;
}

DataList generateExample(int m) {
    DataList example;
    for (int i = 0; i < m; ++i) example.push_back(BuildData(Int, std::rand() % 5 - 2));
    return example;
}
DataStorage generateExampleList(int n, int m) {
    DataStorage res;
    for (int i = 0; i < n; ++i) res.push_back(generateExample(m));
    return res;
}

int getParamNum(Grammar* g) {
    int param_num = 0;
    for (auto* node: g->symbol_list) {
        for (auto* rule: node->rule_list) {
            auto* ps = grammar::getParamSemantics(rule);
            if (ps) param_num = std::max(param_num, ps->id + 1);
        }
    }
    return param_num;
}

PProgram generateProgram(FlattenGrammar* fg, int node_id = 0) {
    auto& node = fg->graph->node_list[node_id];
    auto& edge = node.edge_list[rand() % node.edge_list.size()];
    auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
    if (ps) return fg->param_info_list[ps->id].program;
    ProgramList sub_list;
    for (int i = 0; i < edge.v_list.size(); ++i) {
        sub_list.push_back(generateProgram(fg, edge.v_list[i]));
    }
    return std::make_shared<Program>(edge.semantics, sub_list);
}
bool isEqual(RandomSemanticsScore x, RandomSemanticsScore y) {
    return fabs(x - y) < 1e-9;
}
std::string exampleList2String(const ExampleList& example_list) {
    std::string res = "[";
    for (int i = 0; i < example_list.size(); ++i) {
        if (i) res += ",";
        res += data::dataList2String(example_list[i]);
    }
    return res + "]";
}

void testTrivialSemanticsScorer(Env* env, FlattenGrammar* fg, LearnedScorerType type, int param_num, int ti = 1) {
    for (int _ = 0; _ < ti; ++_) {
        auto* scorer = selector::random::buildTrivialScorer(type, env, fg);
        auto inp_storage = generateExampleList(5, param_num);
        int l = 0, r = -1;
        while (1) {
            ExampleList full;
            auto p = generateProgram(fg);
            for (int i = l; i <= r + 1; ++i) full.push_back(inp_storage[i]);
            LOG(INFO) << "Check for example list " << exampleList2String(full) << " and program " << p->toString();
            std::vector<RandomSemanticsModel*> model_list;
            for (int i = l; i <= r + 1; ++i) model_list.push_back(scorer->learnModel(inp_storage[i]));
            auto tx = selector::test::getLearnedGroundTruth(env, fg, p, model_list, type);
            LOG(INFO) << "Standard result = " << tx;
            auto ty = scorer->getScore(p, inp_storage[r + 1]);
            LOG(INFO) << "My result = " << ty;
            assert(isEqual(tx, ty));
            if (l + 1 == inp_storage.size()) break;
            bool is_push = rand() & 1;
            if (l > r) is_push = true; if (r + 2 == inp_storage.size()) is_push = false;
            if (is_push) {
                r++; scorer->pushExample(inp_storage[r]);
            } else {
                l++; scorer->popExample();
            }
        }
        delete scorer;
    }
}
void testOptimizedScorer(Env* env, FlattenGrammar* fg, LearnedScorerType type, int param_num, int ti = 1) {
    for (int _ = 0; _ < ti; ++_) {
        auto* scorer = selector::random::buildTrivialScorer(type, env, fg);
        auto* optimized_scorer = selector::random::buildDefaultScorer(type, env, fg);
        optimized_scorer->learner = scorer->learner;
        auto inp_storage = generateExampleList(5, param_num);
        int l = 0, r = -1;
        while (1) {
            ExampleList full;
            auto p = generateProgram(fg);
            for (int i = l; i <= r + 1; ++i) full.push_back(inp_storage[i]);
            LOG(INFO) << "Check for example list " << exampleList2String(full) << " and program " << p->toString();
            auto tx = scorer->getScore(p, inp_storage[r + 1]);
            LOG(INFO) << "My result = " << tx;
            auto ty = optimized_scorer->getScore(p, inp_storage[r + 1]);
            LOG(INFO) << "Optimized result = " << ty;
            assert(isEqual(tx, ty));
            if (l + 1 == inp_storage.size()) break;
            bool is_push = rand() & 1;
            if (l > r) is_push = true; if (r + 2 == inp_storage.size()) is_push = false;
            if (is_push) {
                r++; scorer->pushExample(inp_storage[r]); optimized_scorer->pushExample(inp_storage[r]);
            } else {
                l++; scorer->popExample(); optimized_scorer->popExample();
            }
        }
        delete scorer;
    }
}


int main() {
    std::string benchmark_name = "/tmp/tmp.wHOuYKwdWN/tests/x.sl";
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    if (sygus::getSyGuSTheory(spec->env.get()) == TheoryToken::STRING) {
        samplesy::registerSampleSyBasic(spec->env.get());
        auto& info = spec->info_list[0];
        samplesy::registerSampleSyWitness(spec->env.get());
        info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(),
                                                 dynamic_cast<FiniteIOExampleSpace *>(spec->example_space.get()));
    }
    int seed = time(0);
    srand(seed);
    auto* grammar = spec->info_list[0]->grammar;
    grammar = grammar::generateHeightLimitedGrammar(grammar, 4);
    int param_num = getParamNum(grammar);
    LOG(INFO) << "Param num " << param_num;
    auto* model = ext::vsa::getSizeModel();
    auto* graph = new TopDownContextGraph(grammar, model, ProbModelType::NORMAL_PROB);
    auto* fg = new TrivialFlattenGrammar(graph, spec->env.get(), 50, new AllValidProgramChecker());
    // auto* fg = new MergedFlattenGrammar(graph, spec->env.get(), 50, [](Program*){return true;}, spec->example_space.get());
    fg->print();

    testOptimizedScorer(spec->env.get(), fg, LearnedScorerType::TRIPLE, param_num, 1);

    return 0;
}