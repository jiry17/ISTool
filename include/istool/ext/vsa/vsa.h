//
// Created by pro on 2021/12/29.
//

#ifndef ISTOOL_VSA_H
#define ISTOOL_VSA_H

#include "istool/basic/grammar.h"
#include "istool/ext/vsa/witness_value.h"

class VSANode;
typedef std::shared_ptr<VSANode> PVSANode;
typedef std::vector<PVSANode> VSANodeList;

class VSAEdge {
public:
    PSemantics semantics;
    VSANodeList node_list;
    VSAEdge(const PSemantics& _semantics, const VSANodeList& _node_list);
    virtual ~VSAEdge() = default;
};

typedef std::shared_ptr<VSAEdge> PVSAEdge;
typedef std::vector<PVSAEdge> VSAEdgeList;

class VSANode {
public:
    NonTerminal* symbol;
    int example_num;
    int id;
    VSAEdgeList edge_list;
    VSANode(NonTerminal* _symbol, int _example_num);
    virtual ~VSANode() = default;
};

class SingleVSANode: public VSANode {
public:
    WitnessData oup;
    SingleVSANode(NonTerminal* _symbol, const WitnessData& _oup);
    virtual ~SingleVSANode() = default;
};

class MultiExampleVSANode: public VSANode {
public:
    PVSANode l, r;
    MultiExampleVSANode(const PVSANode& l, const PVSANode& r);
};

namespace ext {
    namespace vsa {
        void cleanUpVSA(const PVSANode& root);
        int indexVSANode(VSANode* root);
    }
}

#endif //ISTOOL_VSA_H
