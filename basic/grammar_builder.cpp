//
// Created by pro on 2021/12/4.
//

#include "istool/basic/grammar_builder.h"
#include <map>
#include <vector>
#include "glog/logging.h"

Grammar * grammar::buildGrammarFromTypes(BasicTypeSystem *system, const BasicGrammarConfig &c) {
    struct NTInfo {
        NonTerminal* nt;
        PType type;
    };
    std::map<std::string, NTInfo> nt_map;
    auto get_symbol = [&](const PType& type) {
        auto name = type->getName();
        if (nt_map.count(name) == 0) {
            auto* symbol = new NonTerminal(name, type);
            nt_map[name] = {symbol, type};
            return symbol;
        }
        auto& info = nt_map[name];
        assert(type::equal(type, info.type));
        return info.nt;
    };
    for (int i = 0; i < c.param_list.size(); ++i) {
        auto type = c.param_list[i];
        auto* symbol = get_symbol(type);
        auto semantics = semantics::buildParamSemantics(i, std::move(type));
        symbol->rule_list.push_back(new Rule(std::move(semantics), {}));
    }
    for (auto& w: c.const_list) {
        auto cp = program::buildConst(w);
        auto ct = system->getType(cp);
        if (!ct) continue;
        auto* symbol = get_symbol(ct);
        auto semantics = semantics::buildConstSemantics(w);
        symbol->rule_list.push_back(new Rule(std::move(semantics), {}));
    }
    for (auto w: c.semantics_list) {
        auto* ts = dynamic_cast<TypedSemantics*>(w.get());
        if (!ts) {
            LOG(FATAL) << "BaiscTypeSystem does not support " << w->getName() << " which is not simple typed";
        }
        NTList param_list;
        for (const auto& type: ts->inp_type_list) {
            param_list.push_back(get_symbol(type));
        }
        NonTerminal* symbol = get_symbol(ts->oup_type);
        symbol->rule_list.push_back(new Rule(std::move(w), std::move(param_list)));
    }
    auto* start = get_symbol(c.oup_type);
    NTList symbol_list;
    for (auto& info: nt_map) {
        symbol_list.push_back(info.second.nt);
    }
    auto* grammar = new Grammar(start, symbol_list);
    grammar->removeUseless();
    return grammar;
}