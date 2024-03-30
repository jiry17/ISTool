//
// Created by zyw on 2024/3/29.
//

#include "istool/incre/language/incre_dp.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::syntax;

void IncreCommandWalker::walkThrough(Command command) {
    initialize(command);
    switch (command->getType()) {
        case CommandType::BIND_TERM: {
            auto cb = std::static_pointer_cast<CommandBindTerm>(command);
            walkThroughTerm(cb->term);
            break;
        }
    }
}

// update result, if already has result then check whether they are the same
void DpCommandWalker::updateRes(Ty new_res) {
    if (!has_res) {
        has_res = true;
        res = new_res;
    } else {
        if (!(res->toString() == new_res->toString())) {
            LOG(FATAL) << "updateRes: type not match! res = " << res->toString() << ", new_res = " << new_res->toString();
        }
    }
}

void DpCommandWalker::walkThroughTerm(Term term) {
    preProcess(term);
    switch (term->getType()) {
        case TermType::VALUE: {
            break;
        }
        case TermType::IF: {
            auto tmp = std::static_pointer_cast<TmIf>(term);
            walkThroughTerm(tmp->c);
            walkThroughTerm(tmp->t);
            walkThroughTerm(tmp->f);
            break;
        }
        case TermType::VAR: {
            break;
        }
        case TermType::PRIMARY: {
            auto tmp = std::static_pointer_cast<TmPrimary>(term);
            for (auto& param : tmp->params) {
                walkThroughTerm(param);
            }
            break;
        }
        case TermType::APP: {
            auto term_app = std::static_pointer_cast<TmApp>(term);
            // std::cout << "zyw: DpCommandWalker::walkThroughTerm, APP, term_app->func = " << term_app->func->toString() << ", term_app->param = " << term_app->param->toString() << std::endl;
            if (term_app->func->toString() == "sinsert") {
                updateRes(checker->typing(term_app->param.get(), ctx->ctx));
            }
            walkThroughTerm(term_app->func);
            walkThroughTerm(term_app->param);
            break;
        }
        case TermType::FUNC: {
            auto tmp = std::static_pointer_cast<TmFunc>(term);
            walkThroughTerm(tmp->body);
            break;
        }
        case TermType::LET: {
            auto tmp = std::static_pointer_cast<TmLet>(term);
            walkThroughTerm(tmp->def);
            walkThroughTerm(tmp->body);
            break;
        }
        case TermType::TUPLE: {
            auto tmp = std::static_pointer_cast<TmTuple>(term);
            for (auto& field : tmp->fields) {
                walkThroughTerm(field);
            }
            break;
        }
        case TermType::PROJ: {
            auto tmp = std::static_pointer_cast<TmProj>(term);
            walkThroughTerm(tmp->body);
            break;
        }
        case TermType::MATCH: {
            auto tmp = std::static_pointer_cast<TmMatch>(term);
            walkThroughTerm(tmp->def);
            for (auto& match_case : tmp->cases) {
                walkThroughTerm(match_case.second);
            }
            break;
        }
        case TermType::CONS: {
            auto tmp = std::static_pointer_cast<TmCons>(term);
            walkThroughTerm(tmp->body);
            break;
        }
        case TermType::LABEL: {
            auto tmp = std::static_pointer_cast<TmLabel>(term);
            walkThroughTerm(tmp->body);
            break;
        }
        case TermType::UNLABEL: {
            auto tmp = std::static_pointer_cast<TmUnlabel>(term);
            walkThroughTerm(tmp->body);
            break;
        }
        case TermType::REWRITE: {
            auto tmp = std::static_pointer_cast<TmRewrite>(term);
            walkThroughTerm(tmp->body);
            break;
        }
    }
    postProcess(term);
}

void DpProgramWalker::visit(CommandDef* command) {
    return;
}

void DpProgramWalker::visit(CommandBindTerm* command) {
    std::shared_ptr<CommandData> tmp = std::static_pointer_cast<CommandData>(std::make_shared<CommandBindTerm>(*command));
    cmdWalker->walkThrough(tmp);
    return;
}

void DpProgramWalker::visit(CommandDeclare* command) {
    return;
}

void DpProgramWalker::initialize(IncreProgramData* program) {
    return;
}

Ty incre::syntax::getSolutionType(IncreProgramData* program, IncreFullContext ctx) {
    auto checker = new incre::types::DefaultIncreTypeChecker();
    auto walker = new DpProgramWalker(ctx, checker);
    walker->walkThrough(program);
    return walker->cmdWalker->res;
}

namespace {
    // calculate times of this program using specific name
    int _calNumOfSpecificName(PProgram program, std::string name) {
        if (program->toString() == name) return 1;
        int result = 0;
        for (auto& sub: program->sub_list) {
            result += _calNumOfSpecificName(sub, name);
        }
        return result;
    }

    // PSemantics _getSpecificSemantics(PProgram program, std::string semantics_name) {
    //     if (program->semantics->name == semantics_name) {
    //         return program->semantics;
    //     }
    //     for (auto& sub: program->sub_list) {
    //         auto tmp = _getSpecificSemantics(sub, semantics_name);
    //         if (tmp) return tmp;
    //     }
    //     return nullptr;
    // }

    // // recursively change program->semantics from old to new
    // void _changeSemantics(PProgram program, PSemantics old_semantics, PSemantics new_semantics) {
    //     if (program->semantics->name == old_semantics->name) {
    //         program->semantics = new_semantics;
    //     }
    //     for (auto& sub: program->sub_list) {
    //         _changeSemantics(sub, old_semantics, new_semantics);
    //     }
    // }
}

// must use Param0
void grammar::postProcessDpProgramList(ProgramList& program_list) {
    ProgramList result;
    for (auto& program: program_list) {
        int param0_num = _calNumOfSpecificName(program, "Param0");
        if (param0_num > 0) {
            result.push_back(program);
        }
    }
    program_list = result;
}

// check whether this program is relative symmetry
bool grammar::postProcessBoolProgram(PProgram program) {
    bool result = true;
    int param0_num = _calNumOfSpecificName(program, "Param0");
    int param1_num = _calNumOfSpecificName(program, "Param1");
    if (param0_num > 0 && param1_num > 0 && param0_num == param1_num) {
        // if sub_program used more than one param0 / param1, then their number should be the same for symmetry
        for (auto& sub: program->sub_list) {
            param0_num = _calNumOfSpecificName(sub, "Param0");
            param1_num = _calNumOfSpecificName(sub, "Param1");
            if (param0_num + param1_num >= 2) {
                result = result && postProcessBoolProgram(sub);
            }
        }
    } else {
        result = false;
    }
    return result;
}

// must use Param0 and Param1, using time must be the same
void grammar::postProcessBoolProgramList(ProgramList& program_list) {
    ProgramList result;
    for (auto& program: program_list) {
        if (postProcessBoolProgram(program)) {
            result.push_back(program);
        }
    }
    program_list = result;
}

ProgramList grammar::mergeProgramList(ProgramList& dp_program_list, ProgramList& bool_program_list) {
    if (dp_program_list.size() == 0 || bool_program_list.size() == 0) return std::vector<PProgram>();
    ProgramList result;
    PProgram param0_program = ::program::buildParam(0);
    PProgram param1_program = ::program::buildParam(1);
    for (auto& dp_program: dp_program_list) {
        PProgram dp_program_1 = ::program::rewriteParam(dp_program, std::vector<PProgram>{param0_program});
        PProgram dp_program_2 = ::program::rewriteParam(dp_program, std::vector<PProgram>{param1_program});
        // std::cout << "dp_program = " << dp_program->toString() << std::endl;
        // std::cout << "dp_program_1 = " << dp_program_1->toString() << std::endl;
        // std::cout << "dp_program_2 = " << dp_program_2->toString() << std::endl;

        for (auto& bool_program: bool_program_list) {
            PProgram tmp = ::program::rewriteParam(bool_program, ProgramList{dp_program_1, dp_program_2});
            result.push_back(tmp);
        }
    }
    return result;
}