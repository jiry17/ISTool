//
// Created by pro on 2023/1/24.
//

#include "istool/incre/autolabel/incre_autolabel_greedy_solver.h"
#include "istool/incre/autolabel/incre_func_type.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;

TmLabeledLet::TmLabeledLet(const std::string &name, const Ty &_var_type, const Term &def, const Term &content):
    TmLet(name, def, content), var_type(_var_type)  {
}

std::string TmLabeledLet::toString() const {
    std::string res = "let " + name + ": " + var_type->toString() + " = " + def->toString() + " in " + content->toString();
    return res;
}

namespace {
    std::pair<Term, Ty> _labelLet(const Term& term, TypeContext* ctx);

#define LabelLetHead(name) std::pair<Term, Ty> _labelLet(Tm ## name* term, const Term& _term, TypeContext* ctx)
#define LabelLetCase(name) return _labelLet(dynamic_cast<Tm ## name*>(term.get()), term, ctx)
#define Rec(name) auto [name ## _term, name ##_ty] = _labelLet(term->name, ctx)

    LabelLetHead(Var) {
        return {_term, ctx->lookup(term->name)};
    }
    LabelLetHead(App) {
        Rec(func); Rec(param);
        auto* ta = dynamic_cast<TyArrow*>(func_ty.get());
        assert(ta && incre::isTypeEqual(ta->source, param_ty, ctx));
        return {std::make_shared<TmApp>(func_term, param_term), ta->target};
    }
    LabelLetHead(If) {
        Rec(c); Rec(t); Rec(f);
        assert(dynamic_cast<TyBool*>(c_ty.get()) && incre::isTypeEqual(t_ty, f_ty, ctx));
        return {std::make_shared<TmIf>(c_term, t_term, f_term), t_ty};
    }
    LabelLetHead(Func) {
        std::vector<TypeContext::BindLog> log_list;
        for (auto& [name, ty]: term->param_list) {
            log_list.push_back(ctx->bind(name, ty));
        }
        if (term->rec_res_type) {
            auto full = term->rec_res_type;
            for (int i = int(term->param_list.size()) - 1; i >= 0; --i) {
                full = std::make_shared<TyArrow>(term->param_list[i].second, full);
            }
            log_list.push_back(ctx->bind(term->func_name, full));
        }
        Rec(content);
        for (int i = int(log_list.size()) - 1; i >= 0; --i) {
            ctx->cancelBind(log_list[i]);
        }

        auto res = content_ty;
        for (int i = int(term->param_list.size()) - 1; i >= 0; --i) {
            res = std::make_shared<TyArrow>(term->param_list[i].second, res);
        }
        return {std::make_shared<TmFunc>(term->func_name, term->rec_res_type, term->param_list, content_term),res};
    }
    LabelLetHead(Proj) {
        Rec(content);
        auto* tt = dynamic_cast<TyTuple*>(content_ty.get());
        assert(tt && term->id <= tt->fields.size());
        return {std::make_shared<TmProj>(content_term, term->id), tt->fields[term->id - 1]};
    }
    LabelLetHead(Tuple) {
        TermList sub_terms; TyList sub_types;
        for (auto& field: term->fields) {
            auto [sub_term, sub_type] = _labelLet(field, ctx);
            sub_terms.push_back(sub_term); sub_types.push_back(sub_type);
        }
        return {std::make_shared<TmTuple>(sub_terms), std::make_shared<TyTuple>(sub_types)};
    }
    LabelLetHead(Match) {
        Rec(def);
        std::vector<std::pair<Pattern, Term>> cases;
        TyList ty_list;
        for (auto &[pt, sub_term]: term->cases) {
            auto logs = incre::bindPattern(pt, def_ty, ctx);
            auto [res_term, res_type] = _labelLet(sub_term, ctx);
            for (int i = int(logs.size()) - 1; i >= 0; --i) {
                ctx->cancelBind(logs[i]);
            }
            cases.emplace_back(pt, res_term); ty_list.push_back(res_type);
        }
        for (int i = 1; i < ty_list.size(); ++i) {
            assert(incre::isTypeEqual(ty_list[0], ty_list[i], ctx));
        }
        return {std::make_shared<TmMatch>(def_term, cases), ty_list[0]};
    }
    LabelLetHead(Let) {
        Rec(def); auto log = ctx->bind(term->name, def_ty);
        Rec(content); ctx->cancelBind(log);
        return {std::make_shared<TmLabeledLet>(term->name, def_ty, def_term, content_term), content_ty};
    }

    std::pair<Term, Ty> _labelLet(const Term& term, TypeContext* ctx) {
        switch (term->getType()) {
            case TermType::VAR: LabelLetCase(Var);
            case TermType::APP: LabelLetCase(App);
            case TermType::IF: LabelLetCase(If);
            case TermType::ALIGN:
            case TermType::ABS:
            case TermType::FIX:
            case TermType::LABEL:
            case TermType::UNLABEL:
                LOG(FATAL) << "Unexpected TermType: " << term->toString();
            case TermType::WILDCARD: LabelLetCase(Func);
            case TermType::PROJ: LabelLetCase(Proj);
            case TermType::TUPLE: LabelLetCase(Tuple);
            case TermType::MATCH: LabelLetCase(Match);
            case TermType::VALUE: LabelLetCase(Var);
            case TermType::LET: LabelLetCase(Let);
        }
    }
}

IncreProgram GreedyAutoLabelSolver::prepareLetType() {
    auto* ctx = new TypeContext();
    CommandList command_list;
    for (const auto& command: task.program->commands) {
        switch (command->getType()) {
            case CommandType::IMPORT: break;
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDefInductive*>(command.get());
                auto pt = incre::clearCompress(cd->_type);
                auto* it = dynamic_cast<TyInductive*>(pt.get());
                ctx->bind(it->name, pt);
                for (auto& [cname, cty]: it->constructors) {
                    auto inp_type = incre::subst(cty, it->name, pt);
                    auto full_type = std::make_shared<TyArrow>(inp_type, pt);
                    ctx->bind(cname, full_type);
                }
                command_list.push_back(command);
                break;
            }
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                auto bind = cb->binding;
                switch (bind->getType()) {
                    case BindingType::VAR: {
                        command_list.push_back(command);
                        break;
                    }
                    case BindingType::TYPE: {
                        auto* tb = dynamic_cast<TypeBinding*>(bind.get());
                        ctx->bind(cb->name, incre::clearCompress(tb->type));
                        command_list.push_back(command);
                        break;
                    }
                    case BindingType::TERM: {
                        auto* tb = dynamic_cast<TermBinding*>(bind.get());
                        auto [term, ty] = _labelLet(tb->term, ctx);
                        ctx->bind(cb->name, ty);
                        auto new_bind = std::make_shared<TermBinding>(term);
                        command_list.push_back(std::make_shared<CommandBind>(cb->name, new_bind));
                        break;
                    }
                }
            }
        }
    }
    delete ctx;
    return std::make_shared<ProgramData>(command_list);
}