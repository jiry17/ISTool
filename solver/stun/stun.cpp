//
// Created by pro on 2022/1/3.
//

#include "istool/solver/stun/stun.h"
#include "istool/sygus/theory/theory.h"
#include "glog/logging.h"
#include <queue>
#include <unordered_set>

STUNSolver::STUNSolver(Specification* spec, TermSolver *_term_solver, Unifier *_unifier):
    term_solver(_term_solver), unifier(_unifier), PBESolver(spec) {
    if (spec->info_list.size() != 1) {
        LOG(FATAL) << "STUNSolver can only synthesize a single program";
    }
    func_name = spec->info_list[0]->name;
}
STUNSolver::~STUNSolver() {
    delete term_solver; delete unifier;
}

FunctionContext STUNSolver::synthesis(const std::vector<Example> &example_list, TimeGuard *guard) {
    auto term_list = term_solver->synthesisTerms(example_list, guard);
    FunctionContext res;
    res[func_name] = unifier->unify(term_list, example_list, guard);
    return res;
}

namespace {
    Grammar* _buildSTUNGrammar(Grammar* g, NonTerminal* start, const std::unordered_set<std::string>& banned) {
        NTList symbol_list;
        std::unordered_map<std::string, NonTerminal*> symbol_map;
        auto copy = [&](NonTerminal* symbol) {
            auto* new_symbol = new NonTerminal(symbol->name, symbol->type);
            symbol_list.push_back(new_symbol);
            symbol_map[symbol->name] = new_symbol;
        };
        for (auto* symbol: g->symbol_list) copy(symbol);
        for (auto* symbol: g->symbol_list) {
            auto* new_symbol = symbol_map[symbol->name];
            for (auto* rule: symbol->rule_list) {
                if (banned.find(rule->semantics->name) != banned.end()) continue;
                NTList new_sub_list;
                for (auto* sub: rule->param_list) new_sub_list.push_back(symbol_map[sub->name]);
                new_symbol->rule_list.push_back(new Rule(rule->semantics, std::move(new_sub_list)));
            }
        }
        auto* grammar = new Grammar(symbol_map[start->name], symbol_list);
        grammar->removeUseless();
        return grammar;
    }
}

std::pair<PSynthInfo, PSynthInfo> solver::divideSpecForSTUN(const PSynthInfo &info) {
    auto* g = info->grammar; auto* start = g->start;
    bool is_used_ite = false;
    NonTerminal* b = nullptr;
    for (auto* rule: start->rule_list) {
        if (rule->semantics->getName() == "ite") {
            if (is_used_ite || rule->param_list[1]->name != start->name || rule->param_list[2]->name != start->name) {
                return {nullptr, nullptr};
            }
            is_used_ite = true; b = rule->param_list[0];
        }
    }
    std::unordered_set<std::string> banned = {"||", "&&", "!", "ite"};
    auto* tg = _buildSTUNGrammar(g, start, banned);
    auto t_info = std::make_shared<SynthInfo>(info->name, info->inp_type_list, info->oup_type, tg);
    auto* cg = _buildSTUNGrammar(g, b, banned);
    auto c_info = std::make_shared<SynthInfo>(info->name, info->inp_type_list, type::getTBool(), cg);
    return {t_info, c_info};
}

std::pair<PSynthInfo, PSynthInfo> solver::divideSyGuSSpecForSTUN(const PSynthInfo &info, Env *env) {
    auto theory = sygus::getSyGuSTheory(env);
    if (theory == TheoryToken::BV) {
        LOG(FATAL) << "The special treatment for BV has not been finished";
    }
    return divideSpecForSTUN(info);
}