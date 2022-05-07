//
// Created by pro on 2022/4/27.
//

#ifndef ISTOOL_RANDOM_SEMANTICS_SCORER_H
#define ISTOOL_RANDOM_SEMANTICS_SCORER_H

#include "istool/selector/selector.h"
#include "istool/selector/random/grammar_flatter.h"
#include "istool/solver/maxflash/topdown_context_graph.h"
#include "istool/ext/vsa/top_down_model.h"

class RandomSemanticsScorer {
public:
    double getTripleScore(const PProgram& p, const DataStorage& inp_list);
    double getPairScore(const DataStorage& inp_list);
    DataStorage getFlattenInpStorage(const DataStorage& inp_list);
    double KOutputSize;
    FlattenGrammar* fg;
    Env* env;
    RandomSemanticsScorer(Env* _env, FlattenGrammar* _fg, double _KOutputSize);
    ~RandomSemanticsScorer();
};
#endif //ISTOOL_RANDOM_SEMANTICS_SCORER_H
