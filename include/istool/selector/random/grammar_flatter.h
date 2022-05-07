//
// Created by pro on 2022/5/2.
//

#ifndef ISTOOL_GRAMMAR_FLATTER_H
#define ISTOOL_GRAMMAR_FLATTER_H

#include "istool/basic/grammar.h"
#include "istool/solver/maxflash/topdown_context_graph.h"

class FlattenGrammar {
    TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const;
public:
    std::vector<std::pair<PType, PProgram>> param_list;
    TopDownContextGraph* graph;
    std::unordered_map<std::string, int> param_map;
    std::vector<std::vector<int>> param_index_list;
    bool is_indexed;
    void buildParamIndex();

    TopDownGraphMatchStructure* getMatchStructure(const PProgram& program);
    FlattenGrammar(const std::vector<std::pair<PType, PProgram>>& _param_list, TopDownContextGraph* _grammar);
    ~FlattenGrammar();
};

namespace selector {
    FlattenGrammar* getFlattenGrammar(TopDownContextGraph* g, int size_limit, const std::function<bool(Program*)>& validator);
}

#endif //ISTOOL_GRAMMAR_FLATTER_H
