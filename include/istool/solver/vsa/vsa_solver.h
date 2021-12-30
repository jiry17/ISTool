//
// Created by pro on 2021/12/30.
//

#ifndef ISTOOL_VSA_SOLVER_H
#define ISTOOL_VSA_SOLVER_H

#include "vsa_builder.h"
#include "istool/solver/solver.h"

typedef std::function<void(Specification*, const IOExample&)> VSAEnvPreparation;

class BasicVSASolver: public PBESolver {
    PVSANode buildVSA(const ExampleList& example_list, TimeGuard* guard);
public:
    VSABuilder* builder;
    IOExampleSpace* io_space;
    VSAEnvPreparation prepare;
    std::unordered_map<std::string, PVSANode> cache;
    BasicVSASolver(Specification* spec, VSABuilder* _builder, const VSAEnvPreparation& _p);
    virtual FunctionContext synthesis(const ExampleList& example_list, TimeGuard* guard);
    virtual ~BasicVSASolver();
};

#endif //ISTOOL_VSA_SOLVER_H
