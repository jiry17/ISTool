//
// Created by pro on 2022/1/7.
//

#ifndef ISTOOL_DNF_LEARNER_H
#define ISTOOL_DNF_LEARNER_H

#include "istool/solver/solver.h"

namespace polygen {

}

class DNFLearner: public PBESolver {
public:
    PSynthInfo pred_info;
    DNFLearner(Specification* spec);
    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr);
    ~DNFLearner();
};

#endif //ISTOOL_DNF_LEARNER_H
