//
// Created by pro on 2021/12/29.
//

#ifndef ISTOOL_VSA_H
#define ISTOOL_VSA_H

#include "istool/basic/grammar.h"
#include "istool/ext/vsa/witness_value.h"

class VSANode;
typedef std::vector<VSANode*> VSANodeList;

class VSAEdge {
public:
    PSemantics semantics;
    VSANodeList node_list;
    VSAEdge(const PSemantics& _semantics, const VSANodeList& _node_list);
    virtual ~VSAEdge() = default;
};

typedef std::vector<VSAEdge> VSAEdgeList;

class VSANode {
public:
    NonTerminal* symbol;
    int example_num;
    int id;
    VSAEdgeList edge_list;
    VSANode(NonTerminal* _symbol, int _example_num);
    virtual ~VSANode() = default;
    virtual std::string toString() = 0;
};

class SingleVSANode: public VSANode {
public:
    WitnessData oup;
    SingleVSANode(NonTerminal* _symbol, const WitnessData& _oup);
    virtual ~SingleVSANode() = default;
    virtual std::string toString();
};

class MultiExampleVSANode: public VSANode {
public:
    VSANode *l, *r;
    MultiExampleVSANode(VSANode* l, VSANode* r);
    virtual std::string toString();
};

namespace ext {
    namespace vsa {
        void cleanUpVSA(VSANode* root);
        int indexVSANode(VSANode* root);
        bool isAcyclic(VSANode* root, int n = -1);
        void printVSA(VSANode* root);
        void deleteVSA(VSANode* root);
    }
}

#endif //ISTOOL_VSA_H
