//
// Created by pro on 2022/9/26.
//

#ifndef ISTOOL_INCRE_GRAMMAR_BUILDER_H
#define ISTOOL_INCRE_GRAMMAR_BUILDER_H

#include "istool/basic/grammar.h"
#include "istool/incre/analysis/incre_instru_info.h"

namespace incre {
    Grammar* buildFGrammar(PassTypeInfoData* pass_info, const std::vector<FComponent>& component_list, Context* ctx);
}

#endif //ISTOOL_INCRE_GRAMMAR_BUILDER_H
