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

Rule::Rule(PSemantics &&_semantics, NTList &&_param_list):
    semantics(_semantics), param_list(_param_list){
}
std::string Rule::toString() const {
    std::string res = semantics->getName();
    if (param_list.empty()) return res;
    for (int i = 0; i < param_list.size(); ++i) {
        if (i) res += ",";
        res += param_list[i]->name;
    }
    return res + ")";
}
PProgram Rule::buildProgram(ProgramList &&sub_list) {
    auto sem = semantics;
    return std::make_shared<Program>(std::move(sem), sub_list);
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
}
Grammar::~Grammar() {
    for (auto* symbol: symbol_list) delete symbol;
}

void Grammar::removeUseless() {
    std::set<std::string> nonempty_set;
    while (1) {
        bool is_converged = true;
        for (auto* symbol: symbol_list) {
            if (nonempty_set.find(symbol->name) != nonempty_set.end()) continue;
            for (auto* r: symbol->rule_list) {
                bool is_empty = false;
                for (auto* sub: r->param_list) {
                    if (nonempty_set.find(sub->name) == nonempty_set.end()) {
                        is_empty = true;
                        break;
                    }
                }
                if (!is_empty) {
                    nonempty_set.insert(symbol->name);
                    is_converged = false;
                }
            }
        }
        if (is_converged) break;
    }
    nonempty_set.insert(start->name);
    int now = 0;
    for (int i = 0; i < symbol_list.size(); ++i) {
        if (nonempty_set.find(symbol_list[i]->name) == nonempty_set.end()) {
            delete symbol_list[i];
        } else {
            symbol_list[now++] = symbol_list[i];
        }
    }
    symbol_list.resize(now);
}

void Grammar::indexSymbol() {
    for (int i = 0; i < symbol_list.size(); ++i) {
        symbol_list[i]->id = i;
    }
}