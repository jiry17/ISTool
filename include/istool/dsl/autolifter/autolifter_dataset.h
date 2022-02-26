//
// Created by pro on 2022/2/23.
//

#ifndef ISTOOL_AUTOLIFTER_DATASET_H
#define ISTOOL_AUTOLIFTER_DATASET_H

#include "istool/solver/autolifter/basic/lifting_problem.h"

namespace dsl::autolifter {
    LiftingTask* getLiftingTask(const std::string& name);
}

#endif //ISTOOL_AUTOLIFTER_DATASET_H
