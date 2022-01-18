//
// Created by pro on 2022/1/2.
//

#ifndef ISTOOL_DEBUG_VSA_H
#define ISTOOL_DEBUG_VSA_H

#include "istool/ext/vsa/vsa.h"
#include "istool/basic/example_space.h"

namespace debug {
    PProgram getRandomProgram(VSANode* root);
    void testVSA(VSANode* root, const ExampleList& example_list, Env* env);
    void testVSAEdge(VSANode* node, const VSAEdge& edge);
    bool containProgram(VSANode* root, Program* program);
    void viewVSA(VSANode* node);
    void testWitness(Semantics* semantics, const WitnessData& oup, const WitnessList& res);
}


#endif //ISTOOL_DEBUG_VSA_H
