//
// Created by pro on 2023/4/5.
//

#include "istool/incre/grammar/incre_component_collector.h"

using namespace incre;
using namespace incre::grammar;

SynthesisComponent::SynthesisComponent(int _command_id, int _priority):
    command_id(_command_id), priority(_priority) {
}

UserProvidedComponent::UserProvidedComponent(const TypeList &_inp_types, const PType &_oup_type, const PSemantics &_semantics, int command_id):
    SynthesisComponent(command_id, 0), semantics(_semantics), inp_types(_inp_types), oup_type(_oup_type) {
}

ComponentSemantics::ComponentSemantics(const Data &_data, const std::string &_name): ConstSemantics(_data) {
    name = _name;
}

#include "istool/sygus/theory/basic/clia/clia_semantics.h"
#include "istool/ext/deepcoder/data_type.h"

namespace {
    NonTerminal* _getSymbol(std::unordered_map<std::string, NonTerminal *> &symbol_map, const PType& type) {
        auto feature = type->getName();
        if (symbol_map.find(feature) != symbol_map.end()) {
            return symbol_map[feature];
        }
        auto symbol = new NonTerminal(feature, type);
        return symbol_map[feature] = symbol;
    }

    NTList _getAllSymbols(const std::unordered_map<std::string, NonTerminal*>& symbol_map) {
        NTList res;
        for (auto& [_, symbol]: symbol_map) res.push_back(symbol);
        return res;
    }

    void _expandToFixedPoint(std::unordered_map<std::string, NonTerminal*>& symbol_map, const std::function<TypeList(const PType&)>& expand) {
        NTList symbol_list = _getAllSymbols(symbol_map);
        while (1) {
            for (auto* symbol: symbol_list) {
                for (auto& expanded_type: expand(symbol->type)) {
                    _getSymbol(symbol_map, expanded_type);
                }
            }
            if (symbol_list.size() == symbol_map.size()) break;
            symbol_list = _getAllSymbols(symbol_map);
        }
    }
}

void UserProvidedComponent::insertComponent(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto* source = _getSymbol(symbol_map, oup_type);
    NTList param_list;
    for (const auto& inp_type: inp_types) {
        param_list.push_back(_getSymbol(symbol_map, inp_type));
    }
    source->rule_list.push_back(new ConcreteRule(semantics, param_list));
}

Term UserProvidedComponent::buildTerm(const PSemantics& sem, const TermList &term_list) {
    assert(term_list.empty());
    auto* cs = dynamic_cast<ComponentSemantics*>(semantics.get());
    assert(cs);
    return std::make_shared<TmVar>(cs->name);
}

IteComponent::IteComponent(int _priority): SynthesisComponent(-1, _priority) {
}
void IteComponent::insertComponent(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto* term = _getSymbol(symbol_map, theory::clia::getTInt());
    auto* cond = _getSymbol(symbol_map, type::getTBool());
    term->rule_list.push_back(new ConcreteRule(std::make_shared<IteSemantics>(), {cond, term, term}));
}
Term IteComponent::buildTerm(const PSemantics& sem, const TermList &term_list) {
    assert(term_list.size() == 3);
    return std::make_shared<TmIf>(term_list[0], term_list[1], term_list[2]);
}

class TmAppSemantics: public FullExecutedSemantics {
public:
    Context* ctx;
    TmAppSemantics(Context* _ctx): FullExecutedSemantics("app"), ctx(_ctx) {}
    virtual Data run(DataList&& inp_list, ExecuteInfo* info) {
        Data current = inp_list[0];
        for (int i = 1; i < inp_list.size(); ++i) {
            auto* fv = dynamic_cast<VFunction*>(current.get());
            current = fv->run(std::make_shared<TmValue>(inp_list[i]), ctx);
        }
        return current;
    }
    virtual ~TmAppSemantics() = default;
};

ApplyComponent::ApplyComponent(int _priority, Context* _ctx, bool _is_only_full): is_only_full(_is_only_full),
    SynthesisComponent(-1, _priority), ctx(_ctx) {
}
Term ApplyComponent::buildTerm(const PSemantics& sem, const TermList &term_list) {
    auto current = term_list[0];
    for (int i = 1; i < term_list.size(); ++i) {
        current = std::make_shared<TmApp>(current, term_list[i]);
    }
    return current;
}
void ApplyComponent::insertComponent(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto semantics = std::make_shared<TmAppSemantics>(ctx);
    if (is_only_full) {
        auto symbol_list = _getAllSymbols(symbol_map);
        for (auto* func: symbol_list) {
            auto current = func->type;
            NTList param_list = {func};
            while (1) {
                auto* ta = dynamic_cast<TArrow*>(current.get());
                if (!ta) break; assert(ta->inp_types.size() == 1);
                param_list.push_back(_getSymbol(symbol_map, ta->inp_types[0]));
                current = ta->oup_type;
            }
            if (param_list.size() == 1) continue;
            auto* res = _getSymbol(symbol_map, current);
            res->rule_list.push_back(new ConcreteRule(semantics, param_list));
        }
    } else {
        _expandToFixedPoint(symbol_map, [](const PType& type) -> TypeList {
            auto* ta = dynamic_cast<TArrow*>(type.get());
            if (!ta) return {}; return {ta->inp_types[0], ta->oup_type};
        });
        for (auto* func: _getAllSymbols(symbol_map)) {
            auto current = func->type;
            auto* ta = dynamic_cast<TArrow*>(current.get());
            if (!ta) continue; assert(ta->inp_types.size() == 1);
            auto param = _getSymbol(symbol_map, ta->inp_types[0]);
            auto res = _getSymbol(symbol_map, ta->oup_type);
            res->rule_list.push_back(new ConcreteRule(semantics, {func, param}));
        }
    }
}

#include "istool/ext/deepcoder/deepcoder_semantics.h"

TupleComponent::TupleComponent(int _priority): SynthesisComponent( -1, _priority) {
}
void TupleComponent::insertComponent(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto sem = std::make_shared<ProductSemantics>();
    _expandToFixedPoint(symbol_map, [](const PType& type) -> TypeList {
        auto* tp = dynamic_cast<TProduct*>(type.get());
        if (!tp) return {}; return tp->sub_types;
    });
    for (auto* symbol: _getAllSymbols(symbol_map)) {
        auto* tp = dynamic_cast<TProduct*>(symbol->type.get());
        if (!tp) continue;
        NTList param_list;
        for (auto& sub_type: tp->sub_types) {
            param_list.push_back(_getSymbol(symbol_map, sub_type));
        }
        symbol->rule_list.push_back(new ConcreteRule(sem, param_list));
    }
}
Term TupleComponent::buildTerm(const PSemantics& sem, const TermList &term_list) {
    return std::make_shared<TmTuple>(term_list);
}

ProjComponent::ProjComponent(int _priority): SynthesisComponent(-1, _priority) {
}
void ProjComponent::insertComponent(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    _expandToFixedPoint(symbol_map, [](const PType& type) -> TypeList {
        auto* tp = dynamic_cast<TProduct*>(type.get());
        if (!tp) return {}; return tp->sub_types;
    });
    for (auto* symbol: _getAllSymbols(symbol_map)) {
        auto* tp = dynamic_cast<TProduct*>(symbol->type.get());
        if (!tp) continue;
        for (int i = 0; i < tp->sub_types.size(); ++i) {
            auto sem = std::make_shared<AccessSemantics>(i);
            auto* target = _getSymbol(symbol_map, tp->sub_types[i]);
            target->rule_list.push_back(new ConcreteRule(sem, {symbol}));
        }
    }
}
Term ProjComponent::buildTerm(const PSemantics &sem, const TermList &term_list) {
    auto* as = dynamic_cast<AccessSemantics*>(sem.get());
    assert(as && term_list.size() == 1);
    return std::make_shared<TmProj>(term_list[0], as->id + 1);
}


namespace {
    bool _cmp_priority(const PSynthesisComponent& x, const PSynthesisComponent& y) {
        return x->priority < y->priority;
    }
}

ComponentPool::ComponentPool(const SynthesisComponentList &_align_list, const SynthesisComponentList &_compress_list,
                             const SynthesisComponentList &_comb_list):
                             align_list(_align_list), compress_list(_compress_list), comb_list(_comb_list) {
    std::sort(align_list.begin(), align_list.end(), _cmp_priority);
    std::sort(compress_list.begin(), compress_list.end(), _cmp_priority);
    std::sort(comb_list.begin(), comb_list.end(), _cmp_priority);
}

ComponentCollector::ComponentCollector(ComponentCollectorType _type): type(_type) {
}