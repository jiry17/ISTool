//
// Created by pro on 2023/1/22.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include "istool/incre/autolabel/incre_func_type.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;

AutoLabelTask::AutoLabelTask(const IncreProgram& _program, const std::unordered_map<std::string, Ty>& _target_type_map):
    program(_program), target_type_map(_target_type_map) {
}

Ty AutoLabelTask::getTargetType(const std::string &name) const {
    auto it = target_type_map.find(name);
    if (it == target_type_map.end()) LOG(FATAL) << "Unlabeled symbol: " << name;
    return it->second;
}

AutoLabelSolver::AutoLabelSolver(const AutoLabelTask &_task): task(_task) {
}

AutoLabelTask autolabel::initTask(ProgramData *program) {
    auto new_program = autolabel::buildFuncTerm(program);

    std::unordered_map<std::string, Ty> label_map;
    auto* ctx = new TypeContext();
    for (auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::IMPORT: continue;
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDefInductive*>(command.get());
                auto cleared_type = incre::clearCompress(cd->_type);
                auto* type = dynamic_cast<TyInductive*>(cleared_type.get());
                ctx->bind(type->name, cleared_type);
                for (auto& [name, cons_type]: type->constructors) {
                    auto inp_type = incre::subst(cons_type, type->name, cleared_type);
                    auto full_type = std::make_shared<TyArrow>(inp_type, cleared_type);
                    ctx->bind(name, full_type);
                }
                continue;
            }
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                auto bind = cb->binding;
                switch (bind->getType()) {
                    case BindingType::VAR: {
                        auto* bv = dynamic_cast<VarTypeBinding*>(bind.get());
                        label_map[cb->name] = bv->type;
                        continue;
                    }
                    case BindingType::TERM: {
                        auto* bt = dynamic_cast<TermBinding*>(bind.get());
                        auto type = autolabel::getTypeWithFunc(bt->term, ctx);
                        ctx->bind(cb->name, type);
                        if (!label_map.count(cb->name)) label_map[cb->name] = type;
                        continue;
                    }
                    case BindingType::TYPE: {
                        auto* bt = dynamic_cast<TypeBinding*>(bind.get());
                        ctx->bind(cb->name, bt->type);
                        continue;
                    }
                }
            }
        }
    }

    return {new_program, label_map};
}

namespace {
    Term _assignTargetType(TmFunc* term, const Ty& target) {
        std::vector<FuncParamInfo> param_list;

        auto current_type = target;
        for (auto& [name, _]: term->param_list) {
            auto* ta = dynamic_cast<TyArrow*>(current_type.get());
            assert(ta);
            param_list.emplace_back(name, ta->source);
            current_type = ta->target;
        }

        return std::make_shared<TmFunc>(term->func_name, term->rec_res_type ? current_type : nullptr,
                                        param_list, term->content);
    }
}

void AutoLabelTask::updateFuncType() {
    CommandList commands;
    for (auto& command: program->commands) {
        if (command->getType() != CommandType::BIND) {
            commands.push_back(command);
            continue;
        }
        auto* cb = dynamic_cast<CommandBind*>(command.get());
        auto& bind = cb->binding;
        if (bind->getType() != BindingType::TERM) {
            if (bind->getType() != BindingType::VAR) commands.push_back(command);
            continue;
        }
        auto* tb = dynamic_cast<TermBinding*>(bind.get());
        auto& term = tb->term;
        if (term->getType() != TermType::WILDCARD) {
            commands.push_back(command);
            continue;
        }
        auto target_type = getTargetType(cb->name);
        LOG(INFO) << "assign " << cb->name << " " << target_type->toString();
        auto new_term = _assignTargetType(dynamic_cast<TmFunc*>(term.get()), target_type);
        auto new_bind = std::make_shared<TermBinding>(new_term);
        commands.push_back(std::make_shared<CommandBind>(cb->name, new_bind));
    }
    program = std::make_shared<ProgramData>(commands);
    LOG(INFO) << program->commands.size();
    program->print();
}