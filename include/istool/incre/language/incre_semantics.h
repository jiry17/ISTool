//
// Created by pro on 2022/9/17.
//

#ifndef ISTOOL_INCRE_SEMANTICS_H
#define ISTOOL_INCRE_SEMANTICS_H

#include "incre_term.h"
#include "incre_context.h"
#include "incre_value.h"
#include "incre_program.h"

namespace incre {
    Term subst(const Term& x, const std::string& name, const Term& y);
    bool isMatch(const Data& data, const Pattern& pt);
    std::vector<std::pair<std::string, Term>> bindPattern(const Data& data, const Pattern& pt);
    Data run(const Term& term, Context* ctx);
    void run(const Command& command, Context* ctx);
    void run(const Program& program, Context* ctx);
    Context* run(const Program& program);
}

#endif //ISTOOL_INCRE_SEMANTICS_H
