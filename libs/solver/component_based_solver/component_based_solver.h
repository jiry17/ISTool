//
// Created by pro on 2021/12/9.
//

#ifndef ISTOOL_COMPONENT_BASED_SOLVER_H
#define ISTOOL_COMPONENT_BASED_SOLVER_H

#include "grammar_encoder.h"
#include "solver/solver.h"

class ComponentBasedSynthesizer: public PBESolver {
public:
    Z3GrammarEncoderBuilder* builder;
    ComponentBasedSynthesizer(Z3GrammarEncoderBuilder* _builder);
    virtual PProgram synthesis(Specification* spec, const std::vector<PExample>& example_list);
    virtual ~ComponentBasedSynthesizer();
};


#endif //ISTOOL_COMPONENT_BASED_SOLVER_H
