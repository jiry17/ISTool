//
// Created by pro on 2021/12/27.
//

#ifndef ISTOOL_ENUM_H
#define ISTOOL_ENUM_H

#include "istool/basic/verifier.h"
#include "istool/basic/time_guard.h"

class Optimizer {
public:
    virtual bool isDuplicated(const std::string& name, NonTerminal* nt, const PProgram& p) = 0;
    virtual void clear() = 0;
};

class TrivialOptimizer: public Optimizer {
public:
    virtual bool isDuplicated(const std::string& name, NonTerminal* nt, const PProgram& p);
    virtual void clear();
};

class TrivialVerifier: public Verifier {
public:
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    virtual ~TrivialVerifier() = default;
};

struct EnumConfig {
    int res_num_limit = 1;
    TimeGuard* guard;
    Verifier* v;
    Optimizer* o;
    EnumConfig(Verifier* _v, Optimizer* _o, TimeGuard* _guard = nullptr);
};

namespace solver {
    std::vector<FunctionContext> enumerate(const std::vector<PSynthInfo>& info_list, const EnumConfig& c);
}

#endif //ISTOOL_ENUM_H
