//
// Created by pro on 2022/5/2.
//

#ifndef ISTOOL_GRAMMAR_FLATTER_H
#define ISTOOL_GRAMMAR_FLATTER_H

#include "istool/basic/grammar.h"
#include "istool/solver/maxflash/topdown_context_graph.h"

class FlattenGrammar {
protected:
    virtual TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const = 0;
public:
    struct ParamInfo {
        PType type;
        PProgram program;
        ParamInfo(const PType& _type, const PProgram& _program);
        ParamInfo() = default;
    };
    TopDownContextGraph* graph;
    std::unordered_map<std::string, TopDownGraphMatchStructure*> match_cache;
    std::vector<ParamInfo> param_info_list;
    Env* env;
    void print() const;
    TopDownGraphMatchStructure* getMatchStructure(const PProgram& program);
    Example getFlattenInput(const Example& input) const;
    FlattenGrammar(TopDownContextGraph* _graph, Env* _env);
    ~FlattenGrammar();
};

class TrivialFlattenGrammar: public FlattenGrammar {
protected:
    virtual TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const;
public:
    std::unordered_map<std::string, int> param_map;
    Env* env;
    TrivialFlattenGrammar(TopDownContextGraph* _g, Env* env, int flatten_num, const std::function<bool(Program*)>& validator);
    ~TrivialFlattenGrammar() = default;
};

class EquivalenceCheckTool {
public:
    virtual PProgram insertProgram(const PProgram& p) = 0;
    virtual PProgram queryProgram(const PProgram& p) = 0;
    virtual Data getConst(Program* p) = 0;
};

class MergedFlattenGrammar: public FlattenGrammar {
protected:
    virtual TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const;
public:
    std::unordered_map<std::string, int> param_map;
    Env* env;
    EquivalenceCheckTool* tool;
    MergedFlattenGrammar(TopDownContextGraph* _g, Env* env, int flatten_num, const std::function<bool(Program*)>& validator, ExampleSpace* example_space);
    ~MergedFlattenGrammar();
};

#endif //ISTOOL_GRAMMAR_FLATTER_H
