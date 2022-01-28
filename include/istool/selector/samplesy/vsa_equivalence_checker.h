//
// Created by pro on 2022/1/28.
//

#ifndef ISTOOL_VSA_EQUIVALENCE_CHECKER_H
#define ISTOOL_VSA_EQUIVALENCE_CHECKER_H

#include "samplesy.h"
#include "istool/solver/vsa/vsa_solver.h"
#include <stack>

namespace selector::samplesy {
    class CollectRes {
    public:
        PProgram p;
        Data oup;
        CollectRes(const PProgram& _p, const Data& _oup);
    };
}

class VSAEquivalenceChecker: public EquivalenceChecker {
    std::vector<std::vector<selector::samplesy::CollectRes>> res_pool;
    std::vector<VSANode*> node_list;
    VSAExtension* ext;

    bool extendResPool(int size, ExecuteInfo* info);
    selector::samplesy::CollectRes getRes(const VSAEdge& edge, const std::vector<int>& id_list, ExecuteInfo* info);
    bool isValidWitness(const VSAEdge& edge, const Data& oup, const std::vector<int>& id_list, const DataList& params);
    std::pair<PProgram, PProgram> isAllEquivalent(const IOExample& example);
public:
    PVSABuilder builder;
    VSANode* root;
    FiniteIOExampleSpace* example_space;
    VSAEquivalenceChecker(const PVSABuilder& _builder, FiniteIOExampleSpace* _example_space);
    virtual void addExample(const IOExample& example);
    virtual std::pair<PProgram, PProgram> isAllEquivalent();
    virtual ~VSAEquivalenceChecker() = default;
};

#endif //ISTOOL_VSA_EQUIVALENCE_CHECKER_H
