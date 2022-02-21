//
// Created by pro on 2022/2/21.
//

#ifndef ISTOOL_LIFTING_INVOKER_H
#define ISTOOL_LIFTING_INVOKER_H

#include "istool/solver/autolifter/basic/lifting_solver.h"
#include "istool/invoker/invoker.h"

namespace invoker::single {
    LiftingRes invokeAutoLifter(LiftingTask* task, TimeGuard* guard, const InvokeConfig& config);
}

#endif //ISTOOL_LIFTING_INVOKER_H
