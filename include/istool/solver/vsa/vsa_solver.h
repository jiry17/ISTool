//
// Created by pro on 2021/12/30.
//

#ifndef ISTOOL_VSA_SOLVER_H
#define ISTOOL_VSA_SOLVER_H

#include "vsa_builder.h"
#include "istool/solver/solver.h"
#include "istool/ext/vsa/top_down_model.h"

class BasicVSASolver: public PBESolver {
    VSANode* buildVSA(const ExampleList& example_list, TimeGuard* guard);
public:
    VSABuilder* builder;
    IOExampleSpace* io_space;
    TopDownModel* model;
    VSAEnvPreparation prepare;
    std::unordered_map<std::string, VSANode*> cache;
    BasicVSASolver(Specification* spec, VSABuilder* _builder, const VSAEnvPreparation& _p, TopDownModel* _model);
    virtual FunctionContext synthesis(const ExampleList& example_list, TimeGuard* guard);
    virtual ~BasicVSASolver();
};

#endif //ISTOOL_VSA_SOLVER_H
