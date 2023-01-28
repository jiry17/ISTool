//
// Created by pro on 2023/1/21.
//

#include "istool/incre/autolabel/incre_func_type.h"
#include "istool/incre/language/incre_lookup.h"

using namespace incre;
using namespace incre::autolabel;

TmFunc::TmFunc(const std::string &_func_name, const Ty& _rec_res_type, const std::vector<FuncParamInfo> &_param_list, const Term& _content):
    TermData(TermType::WILDCARD), func_name(_func_name), param_list(_param_list), rec_res_type(_rec_res_type), content(_content) {
}
std::string TmFunc::toString() const {
    std::string res = func_name + "(";
    for (int i = 0; i < param_list.size(); ++i) {
        if (i) res += ",";
        res += param_list[i].first + ": " + param_list[i].second->toString();
    }
    res += ")";
    if (rec_res_type) res += " -> " + rec_res_type->toString() ;
    return res + content->toString();
}

Term autolabel::buildFuncTerm(const std::string& name, const Term& def) {
    Term content(def); bool is_rec = false; Ty rec_res_type;

    if (def->getType() == TermType::FIX) {
        auto* ft = dynamic_cast<TmFix*>(def.get());
        assert(ft->content->getType() == TermType::ABS);
        auto* cf = dynamic_cast<TmAbs*>(ft->content.get());
        is_rec = true;
        auto rec_term = std::make_shared<TmVar>(name);
        content = incre::subst(cf->content, cf->name, rec_term);
        rec_res_type = cf->type;
    }

    std::vector<FuncParamInfo> param_list;
    while (content->getType() == TermType::ABS) {
        auto* abs_term = dynamic_cast<TmAbs*>(content.get());
        param_list.emplace_back(abs_term->name, abs_term->type);
        content = abs_term->content;
    }
    match::MatchTask task;
    task.term_matcher = [](TermData* term, const match::MatchContext & ctx)-> bool {
        return dynamic_cast<TmFix*>(term) || dynamic_cast<TmAbs*>(term);
    };

    if (rec_res_type) {
        for (int i = 0; i < param_list.size(); ++i) {
            auto* ta = dynamic_cast<TyArrow*>(rec_res_type.get());
            rec_res_type = ta->target;
        }
    }
    return std::make_shared<TmFunc>(name, rec_res_type, param_list, content);
}

Term autolabel::unfoldFuncTerm(TmFunc *func_term) {
    Term res = func_term->content;
    for (int i = int(func_term->param_list.size()) - 1; i >= 0; --i) {
        auto& info = func_term->param_list[i];
        res = std::make_shared<TmAbs>(info.first, info.second, res);
    }
    if (func_term->rec_res_type) {
        auto full_type = func_term->rec_res_type;
        for (int i = int(func_term->param_list.size()) - 1; i >= 0; --i) {
            full_type = std::make_shared<TyArrow>(func_term->param_list[i].second, full_type);
        }
        res = std::make_shared<TmAbs>(func_term->func_name, full_type, res);
        res = std::make_shared<TmFix>(res);
    }
    return res;
}

namespace {
    bool _isFunc(TermData* term) {
        return dynamic_cast<TmFix*>(term) || dynamic_cast<TmAbs*>(term);
    }

    Binding _buildFuncTermBind(const std::string& name, const Binding& bind) {
        switch (bind->getType()) {
            case BindingType::VAR: return bind;
            case BindingType::TYPE: return bind;
            case BindingType::TERM: {
                auto* tb = dynamic_cast<TermBinding*>(bind.get());
                if (!_isFunc(tb->term.get())) return bind;
                return std::make_shared<TermBinding>(buildFuncTerm(name, tb->term));
            }
        }
    }
}

IncreProgram autolabel::buildFuncTerm(ProgramData *program) {
    std::vector<Command> command_list;
    for (auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::DEF_IND: {
                command_list.push_back(command); continue;
            }
            case CommandType::IMPORT: continue;
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                auto bind = _buildFuncTermBind(cb->name, cb->binding);
                command_list.push_back(std::make_shared<CommandBind>(cb->name, bind));
                continue;
            }
        }
    }
    return std::make_shared<ProgramData>(command_list);
}

IncreProgram autolabel::unfoldFuncTerm(ProgramData *program) {
    std::vector<Command> command_list;
    for (auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::DEF_IND: {
                command_list.push_back(command); continue;
            }
            case CommandType::IMPORT: continue;
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                if (cb->binding->getType() != BindingType::TERM) {
                    command_list.push_back(command);
                } else {
                    auto* bt = dynamic_cast<TermBinding*>(cb->binding.get());
                    if (bt->term->getType() == TermType::WILDCARD) {
                        auto* ft = dynamic_cast<TmFunc*>(bt->term.get());
                        assert(ft);
                        auto res_term = unfoldFuncTerm(ft);
                        auto res_bind = std::make_shared<TermBinding>(res_term);
                        command_list.push_back(std::make_shared<CommandBind>(cb->name, res_bind));
                    } else command_list.push_back(command);
                }
                continue;
            }
        }
    }
    return std::make_shared<ProgramData>(command_list);
}

Ty autolabel::getTypeWithFunc(const Term &term, TypeContext *ctx) {

    ExternalTypeRule rule;
    rule.func = [](const Term& term, TypeContext* ctx, const std::unordered_map<TermType, ExternalTypeRule>& ext_map) -> Ty {
        auto* tw = dynamic_cast<TmFunc*>(term.get()); assert(tw);
        std::vector<TypeContext::BindLog> log_list;
        for (auto& [name, ty]: tw->param_list) {
            log_list.push_back(ctx->bind(name, ty));
        }
        if (tw->rec_res_type) {
            Ty full_type = tw->rec_res_type;
            for (int i = int(tw->param_list.size()) - 1; i >= 0; --i) {
                full_type = std::make_shared<TyArrow>(tw->param_list[i].second, full_type);
            }
            log_list.push_back(ctx->bind(tw->func_name, full_type));
        }
        auto res = incre::getType(tw->content, ctx, ext_map);
        for (int i = int(tw->param_list.size()) - 1; i >= 0; --i) {
            res = std::make_shared<TyArrow>(tw->param_list[i].second, res);
        }
        for (int i = int(log_list.size()) - 1; i >= 0; --i) {
            ctx->cancelBind(log_list[i]);
        }
        return res;
    };

    ExternalTypeMap rule_map = {{TermType::WILDCARD, rule}};
    return incre::getType(term, ctx, rule_map);
}