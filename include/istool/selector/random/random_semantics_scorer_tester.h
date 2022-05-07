//
// Created by pro on 2022/4/30.
//

#ifndef ISTOOL_RANDOM_SEMANTICS_SCORER_TESTER_H
#define ISTOOL_RANDOM_SEMANTICS_SCORER_TESTER_H

#include "istool/selector/random/grammar_flatter.h"

namespace selector::test {
    double getPairGroundTruth(Env* env, FlattenGrammar* graph, const DataStorage& inp_list, double KO);
    double getTripleGroundTruth(Env* env, FlattenGrammar* graph, const PProgram& p, const DataStorage& inp_list, double KO);
}

#endif //ISTOOL_RANDOM_SEMANTICS_SCORER_TESTER_H
