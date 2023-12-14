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

CommandBind::CommandBind(const std::string &_name, const Term &_term, const DecorateSet &decos):
        term(_term), CommandData(CommandType::BIND, _name, decos) {
}

CommandDef::CommandDef(const std::string &_name, int _param, const std::vector<std::pair<std::string, Ty>> &_cons_list,
                       const DecorateSet &decos): param(_param), cons_list(_cons_list), CommandData(CommandType::DEF_IND, _name, decos) {
}

CommandInput::CommandInput(const std::string &_name, const syntax::Ty &_type, const DecorateSet &decos):
    CommandData(CommandType::INPUT, _name, decos), type(_type) {
}

IncreProgramData::IncreProgramData(const CommandList &_commands, const IncreConfigMap &_config_map):
        commands(_commands), config_map(_config_map) {
}

void incre::IncreProgramWalker::walkThrough(IncreProgramData *program) {
    preProcess(program);
    for (auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                visit(cb); break;
            }
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDef*>(command.get());
                visit(cd); break;
            }
            case CommandType::INPUT: {
                auto* ci = dynamic_cast<CommandInput*>(command.get());
                visit(ci); break;
            }
        }
    }
    postProcess(program);
}

namespace {
    class _DefaultContextBuilder: public IncreProgramWalker {
    public:
        IncreContext ctx;
        std::unordered_map<std::string, EnvAddress*> address_map;
    protected:
        incre::semantics::IncreEvaluator* evaluator;
        incre::types::IncreTypeChecker* checker;
        incre::syntax::IncreTypeRewriter* rewriter;

        virtual void visit(CommandBind* command) {
            auto type = checker ? checker->bindTyping(command->term.get(), ctx) : nullptr;
            auto res = evaluator ? evaluator->evaluate(command->term.get(), ctx) : Data();
            if (type && rewriter) type = rewriter->rewrite(type);
            if (type) LOG(INFO) << command->name << ": " << type->toString();
            ctx = ctx.insert(command->name, Binding(false, type, res));
        }

        virtual void visit(CommandDef* command) {
            for (auto& [cons_name, cons_type]: command->cons_list) {
                ctx = ctx.insert(cons_name, Binding(true, cons_type, {}));
            }
        }

        virtual void visit(CommandInput* command) {
            ctx = ctx.insert(command->name, command->type);
            if (address_map.find(command->name) != address_map.end()) {
                LOG(FATAL) << "Duplicated global input " << command->name;
            }
            address_map[command->name] = ctx.start.get();
        }
    public:

        _DefaultContextBuilder(incre::semantics::IncreEvaluator* _evaluator, incre::types::IncreTypeChecker* _checker,
                               incre::syntax::IncreTypeRewriter* _rewriter):
                ctx(nullptr), evaluator(_evaluator), checker(_checker), rewriter(_rewriter) {
        }

    };
}

IncreFullContext incre::buildContext(IncreProgramData *program, semantics::IncreEvaluator *evaluator,
                                types::IncreTypeChecker *checker, syntax::IncreTypeRewriter* rewriter) {
    auto* walker = new _DefaultContextBuilder(evaluator, checker, rewriter);
    walker->walkThrough(program);
    auto res = std::make_shared<IncreFullContextData>(walker->ctx, walker->address_map);
    delete walker;
    return res;
}

IncreFullContext incre::buildContext(IncreProgramData *program, const semantics::IncreEvaluatorGenerator &eval_gen,
                                     const types::IncreTypeCheckerGenerator &type_checker_gen,
                                     const syntax::IncreTypeRewriterGenerator &rewriter_gen) {
    auto *eval = eval_gen(); auto* type_checker = type_checker_gen();
    auto* rewriter = rewriter_gen();
    if (type_checker && !rewriter) LOG(FATAL) << "Missing type rewriter";
    auto ctx = buildContext(program, eval, type_checker, rewriter);
    delete eval; delete type_checker; delete rewriter;
    return ctx;
}


void IncreProgramRewriter::visit(CommandDef *command) {
    std::vector<std::pair<std::string, Ty>> cons_list;
    for (auto& [cons_name, cons_ty]: command->cons_list) {
        cons_list.emplace_back(cons_name, type_rewriter->rewrite(cons_ty));
    }
    res->commands.push_back(std::make_shared<CommandDef>(command->name, command->param, cons_list, command->decos));
}

void IncreProgramRewriter::visit(CommandBind *command) {
    auto new_term = term_rewriter->rewrite(command->term);
    res->commands.push_back(std::make_shared<CommandBind>(command->name, new_term, command->decos));
}

void IncreProgramRewriter::visit(CommandInput *command) {
    auto new_type = type_rewriter->rewrite(command->type);
    res->commands.push_back(std::make_shared<CommandInput>(command->name, new_type, command->decos));
}

void IncreProgramRewriter::preProcess(IncreProgramData *program) {
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