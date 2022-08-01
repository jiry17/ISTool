//
// Created by pro on 2022/8/1.
//

#include <istool/ext/composed_semantics/composed_semantics.h>
#include "istool/ext/composed_semantics/composed_rule.h"

ComposedRule::ComposedRule(const PProgram &sketch, const NTList &param_list):
    Rule(param_list), composed_sketch(sketch) {
}

namespace {
    std::string _buildSketch(Program* sketch, const NTList& param_list) {
        auto* ps = dynamic_cast<ParamSemantics*>(sketch->semantics.get());
        if (ps) return param_list[ps->id]->name;
        std::vector<std::string> sub_list(sketch->sub_list.size());
        for (int i = 0; i < sketch->sub_list.size(); ++i) {
            sub_list[i] = _buildSketch(sketch->sub_list[i].get(), param_list);
        }
        return sketch->semantics->buildProgramString(sub_list);
    }
}

std::string ComposedRule::toString() const {
    return _buildSketch(composed_sketch.get(), param_list);
}

PProgram ComposedRule::buildProgram(const ProgramList &sub_list) {
    return program::rewriteParam(composed_sketch, sub_list);
}

Rule * ComposedRule::clone(const NTList &new_param_list) {
    return new ComposedRule(composed_sketch, new_param_list);
}

namespace {
    ComposedSemantics* _getComposedSemantics(Rule* rule) {
        auto* cr = dynamic_cast<ConcreteRule*>(rule);
        if (cr) return dynamic_cast<ComposedSemantics*>(cr->semantics.get());
        return nullptr;
    }
}

Grammar * ext::grammar::rewriteComposedRule(Grammar *g) {
    g->indexSymbol(); int n = g->symbol_list.size();
    NTList symbol_list(n);
    for (int i = 0; i < n; ++i) {
        symbol_list[i] = new NonTerminal(g->symbol_list[i]->name, g->symbol_list[i]->type);
    }
    for (int symbol_id = 0; symbol_id < n; ++symbol_id) {
        auto* symbol = symbol_list[symbol_id];
        for (auto* rule: g->symbol_list[symbol_id]->rule_list) {
            ComposedSemantics* cs = _getComposedSemantics(rule);
            NTList param_list(rule->param_list.size());
            for (int i = 0; i < param_list.size(); ++i) {
                param_list[i] = symbol_list[rule->param_list[i]->id];
            }
            if (!cs) {
                symbol->rule_list.push_back(rule->clone(param_list));
                continue;
            }
            assert(cs->param_num == rule->param_list.size());
            symbol->rule_list.push_back(new ComposedRule(cs->body, param_list));
        }
    }
    return new Grammar(symbol_list[g->start->id], symbol_list);
}