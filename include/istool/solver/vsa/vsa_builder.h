//
// Created by pro on 2021/12/29.
//

#ifndef ISTOOL_VSA_BUILDER_H
#define ISTOOL_VSA_BUILDER_H

#include "istool/ext/vsa/vsa_extension.h"
#include "istool/ext/vsa/vsa.h"
#include "istool/basic/specification.h"
#include "istool/basic/time_guard.h"

class Pruner {
public:
    virtual bool isPrune(VSANode* node) = 0;
    virtual void clear() = 0;
    virtual ~Pruner() = default;
};

class TrivialPruner {
public:
    virtual bool isPrune(VSANode* node);
    virtual void clear();
    virtual ~TrivialPruner() = default;
};

class SizeLimitPruner {
public:
    int size_limit, remain;
    SizeLimitPruner(int _size_limit);
    virtual bool isPrune(VSANode* node);
    virtual void clear();
    virtual ~SizeLimitPruner() = default;
};

class VSABuilder {
public:
    Grammar* g;
    Pruner* pruner;
    VSAExtension* ext;
    VSABuilder(Grammar* _g, Pruner* _pruner);
    virtual PVSANode buildVSA(const Data& oup, const DataList& inp_list) = 0;
    virtual PVSANode mergeVSA(const PVSANode& l, const PVSANode& r) = 0;
    ~VSABuilder();
};

class DFSVSABuilder: public VSABuilder {
    PVSANode buildVSA(NonTerminal* nt, const WitnessData& oup, const DataList& inp_list, TimeGuard* guard, std::unordered_map<std::string, PVSANode>& cache);
    PVSANode mergeVSA(const PVSANode& l, const PVSANode& r, TimeGuard* guard, std::unordered_map<std::string, PVSANode>& cache);
public:
    virtual PVSANode buildVSA(const Data& oup, const DataList& inp_list, TimeGuard* guard);
    virtual PVSANode mergeVSA(const PVSANode& l, const PVSANode& r, TimeGuard* guard);
};

class BFSBSABuilder: public VSABuilder {
public:
    virtual PVSANode buildVSA(const Data& oup, const DataList& inp_list, TimeGuard* guard);
    virtual PVSANode mergeVSA(const PVSANode& l, const PVSANode& r, TimeGuard* guard);
};

#endif //ISTOOL_VSA_BUILDER_H
