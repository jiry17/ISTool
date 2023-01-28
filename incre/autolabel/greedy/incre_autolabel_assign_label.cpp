//
// Created by pro on 2023/1/22.
//

#include "istool/incre/autolabel/incre_autolabel_greedy_solver.h"
#include "glog/logging.h"
#include "istool/incre/autolabel/incre_func_type.h"

using namespace incre;
using namespace incre::autolabel;

GreedyAutoLabelSolver::GreedyAutoLabelSolver(const AutoLabelTask &task): AutoLabelSolver(task) {
}

IncreProgram GreedyAutoLabelSolver::label() {
    auto tmp = prepareLetType();
    tmp = prepareVarLabel(tmp.get());
    return tmp;
}

namespace {
    Term _label(const Term& content, TypeContext* ctx, const Ty& origin, const Ty& target) {
        if (incre::isTypeEqual(origin, target, ctx)) return content;
        if (origin->getType() == TyType::COMPRESS) {
            auto* tc = dynamic_cast<TyCompress*>(origin.get());
            if (incre::isTypeEqual(tc->content, target, ctx)) return std::make_shared<TmUnLabel>(content);
        }
        if (target->getType() == TyType::COMPRESS) {
            auto* tc = dynamic_cast<TyCompress*>(target.get());
            if (incre::isTypeEqual(origin, tc->content, ctx)) return std::make_shared<TmLabel>(content);
        }
        LOG(FATAL) << "Cannot unify " << origin->toString() << " and " << target->toString();
    }

    void _error(const Term& term, const Ty& type) {
        LOG(FATAL) << "Cannot label " << term->toString() << " to " << type->toString();
    }

    bool _isDifferOnLabel(const Ty& x, const Ty& y, TypeContext* ctx) {
        auto x_full = incre::clearCompress(incre::unfoldBasicType(x, ctx));
        auto y_full = incre::clearCompress(incre::unfoldBasicType(y, ctx));
        return incre::isTypeEqual(x_full, y_full, ctx);
    }

    Term _assignLabel(const Term& term, TypeContext* ctx, const Ty& target_type);

#define AssignLabelHead(name) Term _assignLabel(Tm ## name* term, const Term& _term, TypeContext* ctx, const Ty& target_type)
#define AssignLabelCase(name) return _assignLabel(dynamic_cast<Tm ## name*>(term.get()), term, ctx, target_type)

    AssignLabelHead(Var) {
        return _label(_term, ctx, ctx->lookup(term->name), target_type);
    }

    AssignLabelHead(If) {
        auto c = _assignLabel(term->c, ctx, std::make_shared<TyBool>());
        auto t = _assignLabel(term->t, ctx, target_type), f = _assignLabel(term->f, ctx, target_type);
        return std::make_shared<TmIf>(c, t, f);
    }

    AssignLabelHead(Tuple) {
        bool is_compress = (target_type->getType() == TyType::COMPRESS);
        auto target = is_compress ? dynamic_cast<TyCompress*>(target_type.get())->content : target_type;
        if (target->getType() != TyType::TUPLE) _error(_term, target);
        auto* tt = dynamic_cast<TyTuple*>(target.get());
        if (tt->fields.size() != term->fields.size()) _error(_term, target);
        TermList fields;
        for (int i = 0; i < tt->fields.size(); ++i) {
            fields.push_back(_assignLabel(term->fields[i], ctx, tt->fields[i]));
        }
        Term res = std::make_shared<TmTuple>(fields);
        if (is_compress) res = std::make_shared<TmLabel>(res);
        return res;
    }

    AssignLabelHead(Value) {
        return _label(_term, ctx, incre::getValueType(term->data.get()), target_type);
    }

    AssignLabelHead(Func) {
        std::vector<FuncParamInfo> info_list; auto current_type(target_type);
        for (auto& [name, pre_type]: term->param_list) {
            if (current_type->getType() == TyType::ARROW) _error(_term, target_type);
            auto* at = dynamic_cast<TyArrow*>(current_type.get());
            if (!_isDifferOnLabel(at->source, pre_type, ctx)) _error(_term, target_type);
            info_list.emplace_back(name, at->source);
            current_type = at->target;
        }

        std::vector<TypeContext::BindLog> log_list;
        for (auto& [name, type]: info_list) log_list.push_back(ctx->bind(name, type));
        log_list.push_back(ctx->bind(term->func_name, target_type));

        auto content = _assignLabel(term->content, ctx, current_type);
        auto res = std::make_shared<TmFunc>(term->func_name, term->rec_res_type ? current_type : nullptr, info_list, content);

        for (int i = int(log_list.size()) - 1; i >= 0; --i) ctx->cancelBind(log_list[i]);
        return res;
    }

    Term _assignLabel(const Term& term, TypeContext* ctx, const Ty& target_type) {
        /*switch (term->getType()) {
            case TermType::VAR: AssignLabelCase(Var);
            case TermType::IF: AssignLabelCase(If);
            case TermType::TUPLE: AssignLabelCase(Tuple);
            case TermType::VALUE: AssignLabelCase(Value);
            case TermType::LABEL:
            case TermType::UNLABEL:
            case TermType::ALIGN:
            case TermType::ABS:
            case TermType::FIX:
                LOG(FATAL) << "Unexpected term " << term->toString();
            case TermType::WILDCARD: AssignLabelCase(Func);
        }*/
    }
}

IncreProgram GreedyAutoLabelSolver::assignLabel() {
    auto* ctx = new TypeContext();
    std::vector<Command> command_list;
    for (auto& command: task.program->commands) {
        switch (command->getType()) {
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDefInductive*>(command.get());
                ctx->bind(cd->type->name, cd->_type);
                for (auto& [cons_name, cons_type]: cd->type->constructors) {
                    auto inp_type = incre::subst(cons_type, cd->type->name, cd->_type);
                    auto full_type = std::make_shared<TyArrow>(inp_type, cd->_type);
                    ctx->bind(cons_name, full_type);
                }
                command_list.push_back(command);
                continue;
            }
            case CommandType::IMPORT: continue;
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                auto bind = cb->binding;
                switch (bind->getType()) {
                    case BindingType::VAR: {
                        command_list.push_back(command); continue;
                    }
                    case BindingType::TYPE: {
                        auto* bt = dynamic_cast<TypeBinding*>(bind.get());
                        ctx->bind(cb->name, bt->type);
                        command_list.push_back(command); continue;
                    }
                    case BindingType::TERM: {
                        auto* bt = dynamic_cast<TermBinding*>(bind.get());
                        auto target_type = task.getTargetType(cb->name);
                        auto term = _assignLabel(bt->term, ctx, target_type);
                        ctx->bind(cb->name, task.getTargetType(cb->name));
                        auto new_bind = std::make_shared<TermBinding>(term);
                        command_list.push_back(std::make_shared<CommandBind>(cb->name, new_bind));
                        continue;
                    }
                }
            }
        }
    }
    delete ctx;
    return std::make_shared<ProgramData>(command_list);
}