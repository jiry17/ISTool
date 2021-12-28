//
// Created by pro on 2021/12/7.
//

#ifndef ISTOOL_Z3_VERIFIER_H
#define ISTOOL_Z3_VERIFIER_H

#include "istool/basic/verifier.h"
#include "z3_example_space.h"

class Z3Verifier: public Verifier {
    Z3EncodeRes encodeConsProgram(const PProgram& program, const FunctionContext& info, const z3::expr_vector& param_list,
            std::unordered_map<Program*, Z3EncodeRes>& cache) const;
    Z3EncodeRes encodeConsProgram(const PProgram& program, const FunctionContext& info, const z3::expr_vector& param_list) const;
public:
    Z3ExampleSpace* example_space;
    Z3Extension* ext;
    Z3Verifier(Z3ExampleSpace* _example_space);
    virtual bool verify(const FunctionContext& info, Example* counter_example) const;
    ~Z3Verifier() = default;
};

#endif //ISTOOL_Z3_VERIFIER_H
