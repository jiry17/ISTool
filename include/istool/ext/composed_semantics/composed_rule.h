//
// Created by pro on 2022/8/1.
//

#ifndef ISTOOL_COMPOSED_RULE_H
#define ISTOOL_COMPOSED_RULE_H

#include "istool/basic/grammar.h"

class ComposedRule: public Rule {
    PProgram composed_sketch;
public:
    ComposedRule(const PProgram& sketch, const NTList& param_list);
    virtual PProgram buildProgram(const ProgramList& sub_list);
    virtual std::string toString() const;
    virtual Rule* clone(const NTList& new_param_list);
    virtual ~ComposedRule() = default;
};

namespace ext::grammar {
    Grammar* rewriteComposedRule(Grammar* g);
}

#endif //ISTOOL_COMPOSED_RULE_H
