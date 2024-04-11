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
void DpTypeCommandWalker::updateRes(Ty new_res) {
    if (!has_res) {
        has_res = true;
        res = new_res;
    } else {
        if (!(res->toString() == new_res->toString())) {
            LOG(FATAL) << "updateRes: type not match! res = " << res->toString() << ", new_res = " << new_res->toString();
        }
    }
}

void DpTypeCommandWalker::walkThroughTerm(Term term) {
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
            // std::cout << "zyw: DpTypeCommandWalker::walkThroughTerm, APP, term_app->func = " << term_app->func->toString() << ", term_app->param = " << term_app->param->toString() << std::endl;
            if (term_app->func->toString() == "sinsert") {
                updateRes(checker->typing(term_app->param.get(), ctx->ctx));
            }
            // if (term_app->param->toString() == "sempty") {
            //     auto sempty_type = checker->typing(term_app->param.get(), ctx->ctx);
            //     std::cout << "zyw: sempty, type = " << sempty_type->toString() << std::endl;
            // }
            
            // if (term_app->func->getType() == TermType::APP) {
            //     auto sub_term_app = std::static_pointer_cast<TmApp>(term_app->func);
            //     if (sub_term_app) {
            //         if (sub_term_app->func->toString() == "sstep1") {
            //             std::cout << "zyw: sstep1, " << std::endl;
            //             std::cout << "term_app->func = " << term_app->func->toString() << std::endl;
            //             std::cout << "term_app->param = " << term_app->param->toString() << std::endl;
            //             std::cout << "sub_term_app->func = "<< sub_term_app->func->toString() << std::endl;
            //             std::cout << "sub_term_app->param = " << sub_term_app->param->toString() << std::endl;
            //             auto tmp_type = checker->typing(term_app->param.get(), ctx->ctx);
            //             std::cout << tmp_type->toString() << std::endl;
            //         }
            //     }
            // }
            
            // if (term_app->func->toString() == "sargmax") {
            //     auto obj_func_type = checker->typing(term_app->param.get(), ctx->ctx);
            //     auto tmp = std::static_pointer_cast<TyArr>(obj_func_type);
            //     if (tmp) {
            //         std::cout << "zyw: sargmax, tmp is TyArr" << std::endl;
            //         std::cout << "solution type = " << tmp->inp->toString() << std::endl;
            //     } else {
            //         std::cout << "zyw: sargmax, not TyArr" << std::endl;
            //     }
            // } 
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

void DpTypeProgramWalker::visit(CommandDef* command) {
    return;
}

void DpTypeProgramWalker::visit(CommandBindTerm* command) {
    std::shared_ptr<CommandData> tmp = std::static_pointer_cast<CommandData>(std::make_shared<CommandBindTerm>(*command));
    cmdWalker->walkThrough(tmp);
    return;
}

void DpTypeProgramWalker::visit(CommandDeclare* command) {
    return;
}

void DpTypeProgramWalker::initialize(IncreProgramData* program) {
    return;
}

Ty incre::syntax::getSolutionType(IncreProgramData* program, IncreFullContext ctx) {
    auto checker = new incre::types::DefaultIncreTypeChecker();
    auto walker = new DpTypeProgramWalker(ctx, checker);
    walker->walkThrough(program);
    return walker->cmdWalker->res;
}

// update result, if already has result then check whether they are the same
void DpObjCommandWalker::updateRes(Term new_res) {
    if (!has_res) {
        has_res = true;
        res = new_res;
    } else {
        if (!(res->toString() == new_res->toString())) {
            LOG(FATAL) << "updateRes: has more than one object function, res = " << res->toString() << ", new_res = " << new_res->toString();
        }
    }
}

void DpObjCommandWalker::walkThroughTerm(Term term) {
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
            if (term_app->func->toString() == "sargmax") {
                updateRes(term_app->param);
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

void DpObjProgramWalker::visit(CommandDef* command) {
    return;
}

void DpObjProgramWalker::visit(CommandBindTerm* command) {
    std::shared_ptr<CommandData> tmp = std::static_pointer_cast<CommandData>(std::make_shared<CommandBindTerm>(*command));
    cmdWalker->walkThrough(tmp);
    return;
}

void DpObjProgramWalker::visit(CommandDeclare* command) {
    return;
}

void DpObjProgramWalker::initialize(IncreProgramData* program) {
    return;
}

namespace incre::syntax {
    Term getObjFunc(IncreProgramData* program, IncreFullContext& ctx) {
        auto checker = new incre::types::DefaultIncreTypeChecker();
        auto walker = new DpObjProgramWalker(ctx, checker);
        walker->walkThrough(program);
        return walker->cmdWalker->res;
    }

    Data applyObjFunc(Term& object_func, Data& sol, incre::semantics::DefaultEvaluator& default_eval, IncreFullContext& ctx) {
        incre::syntax::Term sol_term = std::make_shared<incre::syntax::TmValue>(sol);
        incre::syntax::Term new_term = std::make_shared<incre::syntax::TmApp>(object_func, sol_term);
        auto new_term_result = default_eval.evaluate(new_term.get(), ctx->ctx);
        return new_term_result;
    }

    Data calRelation(PProgram r, Data& sol_1, Data& sol_2, FunctionContext& func_ctx) {
        DataList sol_list = std::vector<Data>{sol_1, sol_2};
        ExecuteInfo* r_info = new ExecuteInfo(sol_list, func_ctx);
        return r->run(r_info);
    }
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
}

namespace grammar {
    // must use Param0
    void postProcessDpProgramList(ProgramList& program_list) {
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
    bool postProcessBoolProgram(PProgram program) {
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

    // must use Param0 and Param1, using time must be the same (recursively check)
    void postProcessBoolProgramList(ProgramList& program_list) {
        ProgramList result;
        for (auto& program: program_list) {
            if (postProcessBoolProgram(program)) {
                result.push_back(program);
            }
        }
        program_list = result;
    }

    bool postProcessDpBoolProgram(PProgram program) {
        int param0_num = _calNumOfSpecificName(program, "Param0");
        int param1_num = _calNumOfSpecificName(program, "Param1");
        if (param0_num > 0 && param1_num > 0 && param0_num == param1_num) {
            return true;
        }
        return false;
    }

    // must use Param0 and Param1, using time must be the same
    void postProcessDpBoolProgramList(ProgramList& program_list) {
        ProgramList result;
        for (auto& program: program_list) {
            if (postProcessDpBoolProgram(program)) {
                result.push_back(program);
            }
        }
        program_list = result;
    }

    ProgramList mergeProgramList(ProgramList& dp_program_list, ProgramList& bool_program_list) {
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
}

namespace {
    const int KINF = 1e9;

    // store result in partial_results
    std::vector<std::vector<PProgram>> _getNewSymbolProgram(std::vector<std::vector<PProgram>>& original_results, std::vector<int>& sub_nodes_id, int pos, std::vector<std::vector<PProgram>>& partial_results) {
        if (pos >= sub_nodes_id.size()) return partial_results;
        int id_now = sub_nodes_id[pos];
        std::vector<std::vector<PProgram>> results;
        if (partial_results.size() == 0) {
            for (auto& symbol: original_results[id_now]) {
                results.push_back(std::vector<PProgram>{symbol});
            }
        } else {
            for (auto& partial_result: partial_results) {
                for (auto& symbol: original_results[id_now]) {
                    std::vector<PProgram> new_result = partial_result;
                    new_result.push_back(symbol);
                    results.push_back(new_result);
                }
            }
        }
        return _getNewSymbolProgram(original_results, sub_nodes_id, pos+1, results);
    }
}

namespace grammar {
    std::vector<PProgram> generateHeightLimitedProgram(Grammar* grammar_original, int limit) {
        // get height limited grammar, so all the program generated from this grammar can satisfy the height limit
        Grammar* grammar = generateHeightLimitedGrammar(grammar_original, limit);
        grammar->print();
        grammar->indexSymbol();
        int n = grammar->symbol_list.size();
        // height of each symbol
        std::vector<int> d(n, KINF);
        // res[symbol->id] stores all programs for this symbol
        std::vector<std::vector<PProgram>> res(n);
        // j in Q[i] means symbol_id=j has depth i
        std::vector<std::vector<int>> Q(n);
        // id to vector of Rule
        std::vector<NonTerminal*> symbolId2Symbol(grammar->symbol_list.size());

        for (auto* symbol: grammar->symbol_list) {
            symbolId2Symbol[symbol->id] = symbol;
            // get depth of this symbol
            size_t last_at_index = symbol->name.rfind('@');
            if (last_at_index != std::string::npos) {
                std::string depth_str = symbol->name.substr(last_at_index + 1);
                int depth = std::stoi(depth_str);
                d[symbol->id] = depth;
                // Q.push({depth, symbol->id});
                Q[depth].push_back(symbol->id);
            } else {
                LOG(FATAL) << "depth not found in symbol->name";
            }
        }
        
        for (int i = 0; i < n; ++i) {
            int symbol_id_num = Q[i].size();
            for (int j = 0; j < symbol_id_num; ++j) {
                auto k = Q[i][j];
                auto depth = i;
                NonTerminal* symbol = symbolId2Symbol[k];
                for (auto& edge: symbol->rule_list) {
                    if (edge->param_list.empty()) {
                        ProgramList empty_list;
                        PProgram new_program = edge->buildProgram(empty_list);
                        std::string tmp = new_program->toString();
                        res[k].push_back(new_program);
                        continue;
                    }
                    std::vector<std::vector<PProgram>> sub_lists;
                    std::vector<int> sub_nodes_id;
                    for (auto* sub: edge->param_list) {
                        sub_nodes_id.push_back(sub->id);
                    }
                    sub_lists = _getNewSymbolProgram(res, sub_nodes_id, 0, sub_lists);
                    // std::cout << sub_lists.size() << std::endl;
                    for (auto& sub_list: sub_lists) {
                        PProgram new_program = edge->buildProgram(sub_list);
                        std::string tmp = new_program->toString();
                        if (tmp.find("inf") == std::string::npos && tmp.find("min") == std::string::npos && tmp.find("max") == std::string::npos && tmp.find("+") == std::string::npos && tmp.find("-") == std::string::npos && tmp.find("ite") == std::string::npos && tmp.find("(0,0)") == std::string::npos) {
                            res[k].push_back(new_program);
                        }
                    }
                }
            }
        }
        
        // print program for each symbol
        // for (int i = 0; i < n; ++i) {
        //     NonTerminal* symbol = symbolId2Symbol[i];
        //     std::cout << symbol->name << ", " << res[i].size() << std::endl;
        //     for (auto& program: res[i]) {
        //         std::cout << program->toString() << std::endl;
        //     }
        // }
        return res[0];
    }

    void deleteDuplicateRule(std::vector<Rule*>& rule_list) {
        std::vector<Rule*> result;
        for (auto& rule: rule_list) {
            bool has_same = false;
            for (auto& pre_rule: result) {
                if (rule->equal(*pre_rule)) {
                    has_same = true;
                    break;
                }
            }
            if (!has_same) {
                result.push_back(rule);       
            }
        }
        rule_list = result;
    }

    void deleteDuplicateRule(Grammar* grammar) {
        for (auto& symbol: grammar->symbol_list) {
            // delete duplicate rules in this symbol
            std::vector<Rule*> simplify_rule_list;
            for (auto& rule: symbol->rule_list) {
                bool has_same = false;
                for (auto& pre_rule: simplify_rule_list) {
                    if (rule->equal(*pre_rule)) {
                        has_same = true;
                        break;
                    }
                }
                if (!has_same) {
                    simplify_rule_list.push_back(rule);
                }
            }
            symbol->rule_list = simplify_rule_list;
        }
    }
} // namespace grammar

