//
// Created by pro on 2022/12/6.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include "istool/incre/autolabel/incre_autolabel_solver.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;

namespace {

    Ty _labelBind(BindingData* bind, LabelContext* ctx) {
        switch (bind->getType()) {
            case BindingType::TYPE: {
                auto* tb = dynamic_cast<TypeBinding*>(bind);
                return labelType(tb->type, ctx);
            }
            case BindingType::TERM: {
                auto* tb = dynamic_cast<TermBinding*>(bind);
                return labelTerm(tb->term, ctx);
            }
        }
    }

    void _labelCommand(CommandData* command, LabelContext* ctx) {
        switch (command->getType()) {
            case CommandType::DEF_IND: {
                auto* ind = dynamic_cast<CommandDefInductive*>(command);
                auto type = unfoldTypeWithZ3Labels(labelType(ind->_type, ctx), ctx);

                auto* ind_type = dynamic_cast<TyLabeledInductive*>(type.get());
                ctx->addConstraint(!ind_type->is_compress);

                ctx->bind(ind->type->name, type);
                LOG(INFO) << "IndType " << ind_type->toString();
                for (const auto& [cons_name, cons_type]: ind_type->constructors) {
                    auto source = unfoldTypeWithZ3Labels(cons_type, ctx);
                    auto ta = std::make_shared<TyArrow>(source, type);
                    ctx->bind(cons_name, ta);
                    LOG(INFO) << "  Constructor " << cons_name << " " << ta->toString();
                }
                return;
            }
            case CommandType::IMPORT: {
                LOG(FATAL) << "Unsupported CommandType IMPORT";
            }
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command);
                auto bind_type = _labelBind(cb->binding.get(), ctx);
                LOG(INFO) << "Bind " << cb->name << " " << bind_type->toString();
                ctx->bind(cb->name, bind_type);
            }
        }
    }
}

void autolabel::initProgramLabel(ProgramData *program, LabelContext* ctx) {
    for (int i = 0; i < program->commands.size(); ++i) {
        _labelCommand(program->commands[i].get(), ctx);
    }
}

namespace {

    Binding _constructBind(BindingData* bind, LabelContext* ctx) {
        switch (bind->getType()) {
            case BindingType::TYPE: {
                auto* bt = dynamic_cast<TypeBinding*>(bind);
                auto type = constructType(bt->type, ctx);
                return std::make_shared<TypeBinding>(type);
            }
            case BindingType::TERM: {
                auto* bt = dynamic_cast<TermBinding*>(bind);
                auto term = constructTerm(bt->term, ctx);
                return std::make_shared<TermBinding>(term);
            }
        }
    }

    Command _constructCommand(CommandData* command, LabelContext* ctx) {
        switch (command->getType()) {
            case CommandType::IMPORT:
                LOG(FATAL) << "Command type import is not supported";
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command);
                return std::make_shared<CommandBind>(cb->name, _constructBind(cb->binding.get(), ctx));
            }
            case CommandType::DEF_IND: {
                auto* ci = dynamic_cast<CommandDefInductive*>(command);
                auto type = constructType(ci->_type, ctx);
                return std::make_shared<CommandDefInductive>(type);
            }
        }
    }

    void _clearLabel(TyData* type, LabelContext* ctx) {
        if (isLabeledType(type)) {
            ctx->addConstraint(!getLabel(type));
        }
        switch (type->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::VAR:
            case TyType::UNIT: return;
            case TyType::ARROW: {
                auto* ta = dynamic_cast<TyArrow*>(type);
                _clearLabel(ta->source.get(), ctx);
                _clearLabel(ta->target.get(), ctx);
                return;
            }
            case TyType::IND: {
                auto* ti = dynamic_cast<TyInductive*>(type);
                for (auto& [name, sub_type]: ti->constructors) {
                    _clearLabel(sub_type.get(), ctx);
                }
                return;
            }
            case TyType::COMPRESS: {
                LOG(FATAL) << "Unexpected TyType COMPRESS";
            }
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(type);
                for (auto& field: tt->fields) _clearLabel(field.get(), ctx);
                return;
            }
        }
    }
}

IncreProgram autolabel::labelProgram(ProgramData *program) {
    auto* ctx = new LabelContext();
    initProgramLabel(program, ctx);
    auto* solver = new MinMixedIncerLbaleSolver();

    ctx->setModel(solver->solve(ctx, program));
    std::vector<Command> command_list;
    for (const auto &command: program->commands) {
        command_list.push_back(_constructCommand(command.get(), ctx));
    }
    delete ctx; delete solver;
    return std::make_shared<ProgramData>(command_list);
}