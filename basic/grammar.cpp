//
// Created by pro on 2021/12/4.
//

#include "istool/basic/grammar.h"
#include "glog/logging.h"
#include <set>

NonTerminal::NonTerminal(const std::string &_name, const PType& _type): name(_name), type(_type) {
}

NonTerminal::~NonTerminal() {
    for (auto* r: rule_list) delete r;
}

Rule::Rule(const PSemantics &_semantics, NTList &&_param_list):
    semantics(_semantics), param_list(_param_list){
}
std::string Rule::toString() const {
    std::string res = semantics->getName();
    if (param_list.empty()) return res;
    for (int i = 0; i < param_list.size(); ++i) {
        if (i) res += ","; else res += "(";
        res += param_list[i]->name;
    }
    return res + ")";
}
PProgram Rule::buildProgram(const ProgramList &sub_list) {
    return std::make_shared<Program>(semantics, sub_list);
}

Grammar::Grammar(NonTerminal *_start, const NTList &_symbol_list): start(_start), symbol_list(_symbol_list) {
    int pos = -1;
    for (int i = 0; i < symbol_list.size(); ++i) {
        if (symbol_list[i]->name == start->name) {
            pos = i;
        }
    }
    if (pos == -1) {
        LOG(FATAL) << "Start symbol (" << start->name << ") not found";
    }
    std::swap(symbol_list[0], symbol_list[pos]);
    removeUseless();
}
Grammar::~Grammar() {
    for (auto* symbol: symbol_list) delete symbol;
}

namespace {
    void _dfs(NonTerminal* symbol, std::vector<bool>& is_visited) {
        if (is_visited[symbol->id]) return;
        is_visited[symbol->id] = true;
        for (auto* rule: symbol->rule_list) {
            for (auto* sub: rule->param_list) _dfs(sub, is_visited);
        }
    }
}

void Grammar::removeUseless() {
    indexSymbol();
    std::vector<bool> is_empty(symbol_list.size(), true);
    while (1) {
        bool is_converged = true;
        for (auto* symbol: symbol_list) {
            if (!is_empty[symbol->id]) continue;
            for (auto* r: symbol->rule_list) {
                bool flag = false;
                for (auto* sub: r->param_list) {
                    if (is_empty[sub->id]) {
                        flag = true;
                        break;
                    }
                }
                if (!flag) {
                    is_empty[symbol->id] = false;
                    is_converged = false;
                }
            }
        }
        if (is_converged) break;
    }
    if (is_empty[0]) LOG(FATAL) << "Empty grammar";
    for (auto* node: symbol_list) {
        int now = 0;
        for (auto* rule: node->rule_list) {
            bool flag = false;
            for (auto* sub: rule->param_list) {
                if (is_empty[sub->id]) {
                    flag = true; break;
                }
            }
            if (flag) delete rule;
            else node->rule_list[now++] = rule;
        }
        node->rule_list.resize(now);
    }
    std::vector<bool> is_visited(symbol_list.size(), false);
    _dfs(start, is_visited);
    int now = 0;
    for (auto* symbol: symbol_list) {
        if (is_visited[symbol->id]) symbol_list[now++] = symbol;
        else delete symbol;
    }
    symbol_list.resize(now);
}

void Grammar::indexSymbol() const {
    for (int i = 0; i < symbol_list.size(); ++i) {
        symbol_list[i]->id = i;
    }
}

void Grammar::print() const {
    std::cout << "start: " << start->name << std::endl;
    for (auto* node: symbol_list) {
        std::cout << "node: " << node->name << std::endl;
        for (auto* rule: node->rule_list) {
            std::cout << "  " << rule->toString() << std::endl;
        }
    }
}

Grammar * grammar::copyGrammar(Grammar *grammar) {
    int n = grammar->symbol_list.size();
    NTList symbols(n); grammar->indexSymbol();
    for (auto* symbol: grammar->symbol_list) {
        auto* new_symbol = new NonTerminal(symbol->name, symbol->type);
        symbols[symbol->id] = new_symbol;
    }
    for (auto* symbol: grammar->symbol_list) {
        for (auto* rule: symbol->rule_list) {
            NTList param_list;
            for (auto* sub_node: rule->param_list) {
                param_list.push_back(symbols[sub_node->id]);
            }
            symbols[symbol->id]->rule_list.push_back(new Rule(rule->semantics, std::move(param_list)));
        }
    }
    return new Grammar(symbols[grammar->start->id], symbols);
}

namespace {
    bool _isUsedName(Grammar* g, const std::string& name) {
        for (auto* symbol: g->symbol_list) {
            if (symbol->name == name) return true;
        }
        return false;
    }
}

std::string grammar::getFreeName(Grammar *grammar) {
    int id = 0;
    while (_isUsedName(grammar, "tmp@" + std::to_string(id))) ++id;
    return "tmp@" + std::to_string(id);
}

PProgram grammar::getMinimalProgram(Grammar *grammar) {
    // todo: complete this function
    grammar->print();
    for (auto* rule: grammar->start->rule_list) {
        if (rule->param_list.empty()) {
            ProgramList sub_list;
            return std::make_shared<Program>(rule->semantics, sub_list);
        }
    }
    LOG(FATAL) << "There is no program with size 1";
}