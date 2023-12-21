//
// Created by pro on 2023/12/8.
//

#include "istool/incre/language/incre_program.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::syntax;


CommandData::CommandData(const CommandType &_type, const std::string& _name, const DecorateSet &_decos):
        type(_type), decos(_decos), name(_name) {
}

bool CommandData::isDecrorateWith(CommandDecorate deco) const {
    return decos.find(deco) != decos.end();
}

CommandType CommandData::getType() const {return type;}

CommandBindTerm::CommandBindTerm(const std::string &_name, bool _is_func, const syntax::Term &_term,
                                 const DecorateSet &decos): CommandData(CommandType::BIND_TERM, _name, decos),
                                 is_func(_is_func), term(_term) {
}

CommandDef::CommandDef(const std::string &_name, int _param, const std::vector<std::pair<std::string, Ty>> &_cons_list,
                       const DecorateSet &decos): param(_param), cons_list(_cons_list), CommandData(CommandType::DEF_IND, _name, decos) {
}

CommandDeclare::CommandDeclare(const std::string &_name, const syntax::Ty &_type, const DecorateSet &decos):
                               CommandData(CommandType::DECLARE, _name, decos), type(_type) {
}

IncreProgramData::IncreProgramData(const CommandList &_commands, const IncreConfigMap &_config_map):
        commands(_commands), config_map(_config_map) {
}

void incre::IncreProgramWalker::walkThrough(IncreProgramData *program) {
    initialize(program);
    for (auto& command: program->commands) {
        preProcess(command.get());
        switch (command->getType()) {
            case CommandType::BIND_TERM: {
                auto* cb = dynamic_cast<CommandBindTerm*>(command.get());
                visit(cb); break;
            }
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDef*>(command.get());
                visit(cd); break;
            }
            case CommandType::DECLARE: {
                auto* ci = dynamic_cast<CommandDeclare*>(command.get());
                visit(ci); break;
            }
        }
        postProcess(command.get());
    }
}

incre::DefaultContextBuilder::DefaultContextBuilder(incre::semantics::IncreEvaluator *_evaluator,
                                                    incre::types::IncreTypeChecker *_checker):
                                                    evaluator(_evaluator), checker(_checker), ctx(nullptr) {
}

void incre::DefaultContextBuilder::visit(CommandBindTerm *command) {
    if (ctx.isContain(command->name)) {
        auto* address = ctx.getAddress(command->name);
        if (!address->bind.data.isNull()) LOG(FATAL) << "Duplicated declaration on name " << command->name;
        if (checker) {
            assert(address->bind.type);
            checker->unify(address->bind.type, checker->typing(command->term.get(), ctx));
        }
        if (evaluator) {
            address->bind.data = evaluator->evaluate(command->term.get(), ctx);
        }
    } else if (command->is_func) {
        ctx = ctx.insert(command->name, Binding(false, {}, {}));
        auto* address = ctx.getAddress(command->name);
        if (checker) {
            checker->pushLevel(); auto current_type = checker->getTmpVar();
            address->bind.type = current_type;
            checker->unify(current_type, checker->typing(command->term.get(), ctx));
            checker->popLevel();
            auto final_type = checker->generalize(current_type, command->term.get());
            address->bind.type = final_type;
        }
        if (evaluator) {
            auto v = evaluator->evaluate(command->term.get(), ctx);
            address->bind.data = v;
        }
    } else {
        Ty type = nullptr;
        if (checker) {
            checker->pushLevel(); type = checker->typing(command->term.get(), ctx);
            checker->popLevel(); type = checker->generalize(type, command->term.get());
        }
        auto res = evaluator ? evaluator->evaluate(command->term.get(), ctx): Data();
        ctx = ctx.insert(command->name, Binding(false, type, res));
    }
}

void incre::DefaultContextBuilder::visit(CommandDef *command) {
    for (auto& [cons_name, cons_type]: command->cons_list) {
        ctx = ctx.insert(cons_name, Binding(true, cons_type, {}));
    }
}

void incre::DefaultContextBuilder::visit(CommandDeclare *command) {
    if (ctx.isContain(command->name)) LOG(FATAL) << "Duplicated name " << command->name;
    ctx = ctx.insert(command->name, command->type);
}

IncreFullContext incre::buildContext(IncreProgramData *program, semantics::IncreEvaluator *evaluator, types::IncreTypeChecker *checker) {
    auto* walker = new DefaultContextBuilder(evaluator, checker);
    walker->walkThrough(program);
    auto res = std::make_shared<IncreFullContextData>(walker->ctx, walker->address_map);
    delete walker;
    return res;
}

IncreFullContext incre::buildContext(IncreProgramData *program, const semantics::IncreEvaluatorGenerator &eval_gen,
                                     const types::IncreTypeCheckerGenerator &type_checker_gen) {
    auto *eval = eval_gen(); auto* type_checker = type_checker_gen();
    auto ctx = buildContext(program, eval, type_checker);
    delete eval; delete type_checker;
    return ctx;
}


void IncreProgramRewriter::visit(CommandDef *command) {
    std::vector<std::pair<std::string, Ty>> cons_list;
    for (auto& [cons_name, cons_ty]: command->cons_list) {
        cons_list.emplace_back(cons_name, type_rewriter->rewrite(cons_ty));
    }
    res->commands.push_back(std::make_shared<CommandDef>(command->name, command->param, cons_list, command->decos));
}

void IncreProgramRewriter::visit(CommandBindTerm *command) {
    auto new_term = term_rewriter->rewrite(command->term);
    res->commands.push_back(std::make_shared<CommandBindTerm>(command->name, command->is_func, new_term, command->decos));
}

void IncreProgramRewriter::visit(CommandDeclare *command) {
    auto new_type = type_rewriter->rewrite(command->type);
    res->commands.push_back(std::make_shared<CommandDeclare>(command->name, new_type, command->decos));
}

void IncreProgramRewriter::initialize(IncreProgramData *program) {
    res = std::make_shared<IncreProgramData>(CommandList(), program->config_map);
}

IncreProgramRewriter::IncreProgramRewriter(IncreTypeRewriter *_type_rewriter, IncreTermRewriter *_term_rewriter):
        type_rewriter(_type_rewriter), term_rewriter(_term_rewriter) {
}

IncreProgram syntax::rewriteProgram(IncreProgramData *program, const IncreTypeRewriterGenerator &type_gen,
                                    const IncreTermRewriterGenerator &term_gen) {
    auto* type_rewriter = type_gen(); auto* term_rewriter = term_gen();
    auto* rewriter = new IncreProgramRewriter(type_rewriter, term_rewriter);
    rewriter->walkThrough(program); auto res = rewriter->res;
    delete type_rewriter; delete term_rewriter; delete rewriter;
    return res;
}