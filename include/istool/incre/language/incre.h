//
// Created by pro on 2022/9/17.
//

#ifndef ISTOOL_INCRE_H
#define ISTOOL_INCRE_H

#include "incre_term.h"
#include "incre_context.h"
#include "incre_value.h"
#include "incre_program.h"

namespace incre {
    bool isUsed(const Pattern& pt, const std::string& name);
    bool isMatch(const Data& data, const Pattern& pt);
    std::vector<std::pair<std::string, Term>> bindPattern(const Data& data, const Pattern& pt);

    Term subst(const Term& x, const std::string& name, const Term& y);
    Data run(const Term& term, Context* ctx);
    void run(const Command& command, Context* ctx);
    void run(const IncreProgram& program, Context* ctx);
    Context* run(const IncreProgram& program);

    struct ExternalTypeRule {
        std::function<Ty(const Term&, TypeContext*, const std::unordered_map<TermType, ExternalTypeRule>&)> func;
    };
    typedef std::unordered_map<TermType, ExternalTypeRule> ExternalTypeMap;

    Ty getType(const Term& x, Context* ctx, const ExternalTypeMap& ext = {});
    Ty getType(const Term& x, TypeContext* ctx, const ExternalTypeMap& ext = {});
    Ty unfoldType(const Ty& x, TypeContext* ctx, const std::vector<std::string>& tmp_names);
    std::vector<TypeContext::BindLog> bindPattern(const Pattern& pt, const Ty& type, TypeContext* ctx);
}

#endif //ISTOOL_INCRE_H
