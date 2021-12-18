//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_GRAMMAR_BUILDER_H
#define ISTOOL_GRAMMAR_BUILDER_H

#include "type_system.h"
#include "grammar.h"

struct BasicGrammarConfig {
    TypeList param_list;
    PType oup_type;
    DataList const_list;
    std::vector<PSemantics> semantics_list;
};

namespace grammar {
    Grammar* buildGrammarFromTypes(BasicTypeSystem* system, const BasicGrammarConfig& c);
}

#endif //ISTOOL_GRAMMAR_BUILDER_H
