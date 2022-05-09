//
// Created by pro on 2022/5/1.
//

#include "istool/selector/random/random_semantics_scorer_tester.h"
#include "istool/selector/random/random_semantics_scorer.h"
#include "istool/selector/random/grammar_flatter.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/parser/parser.h"
#include "istool/ext/vsa/top_down_model.h"
#include "glog/logging.h"

DataStorage readExample(int m) {
    int n; scanf("%d", &n);
    DataStorage inp_storage;
    for (; n; n--) {
        DataList inp;
        for (int i = 0; i < m; ++i) {
            int k; scanf("%d", &k);
            inp.push_back(BuildData(Int, k));
        }
        inp_storage.push_back(inp);
    }
    return inp_storage;
}

int getParamNum(Grammar* g) {
    int param_num = 0;
    for (auto* node: g->symbol_list) {
        for (auto* rule: node->rule_list) {
            auto* ps = dynamic_cast<ParamSemantics*>(rule->semantics.get());
            if (ps) param_num = std::max(param_num, ps->id + 1);
        }
    }
    return param_num;
}

int main() {
    std::string benchmark_name = " /tmp/tmp.wHOuYKwdWN/tests/x.sl";
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    auto* grammar = spec->info_list[0]->grammar;
    grammar = grammar::generateHeightLimitedGrammar(grammar, 5);
    int param_num = getParamNum(grammar);
    LOG(INFO) << "Param num " << param_num;
    grammar->print();
    auto* model = ext::vsa::getSizeModel();
    auto* graph = new TopDownContextGraph(grammar, model, ProbModelType::NORMAL_PROB);
    auto* fg = selector::getFlattenGrammar(graph, 200, [](Program* p) {return true;});
    fg->graph->print();
    double KO = 10.0;
    auto* scorer = new RandomSemanticsScorer(spec->env.get(), fg, KO);

    // Construct Example
    auto inp_storage = readExample(param_num);

    // Verify Double Result
    //LOG(INFO) << "Standard pair res " << selector::test::getPairGroundTruth(spec->env.get(), fg, inp_storage, KO);
    LOG(INFO) << "Selector pair res " << scorer->getPairScore(inp_storage);


    // Verify Triple Result
    auto x = program::buildParam(0, nullptr);
    auto y = program::buildParam(1, nullptr);
    auto plus = std::make_shared<Program>(spec->env->getSemantics("+"), (ProgramList){x, x});
    auto pplus = std::make_shared<Program>(spec->env->getSemantics("+"), (ProgramList){plus, x});
    auto p = plus;

    //LOG(INFO) << "Standard triple res " << selector::test::getTripleGroundTruth(spec->env.get(), fg, p, inp_storage, KO);
    LOG(INFO) << "Selector triple res " << scorer->getTripleScore(p, inp_storage);
    return 0;
}