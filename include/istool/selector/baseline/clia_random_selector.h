//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_CLIA_RANDOM_SELECTOR_H
#define ISTOOL_CLIA_RANDOM_SELECTOR_H

#include "istool/ext/z3/z3_verifier.h"

class CLIARandomSelector: public Z3Verifier {
public:
    int KRandomRange;
    Z3IOExampleSpace* io_space;
    CLIARandomSelector(Env* env, Z3IOExampleSpace* example_space);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
};

namespace selector {
    extern const std::string KCLIARandomRangeName;
}

#endif //ISTOOL_CLIA_RANDOM_SELECTOR_H
