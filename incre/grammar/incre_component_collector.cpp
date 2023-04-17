//
// Created by pro on 2023/4/5.
//

#include "istool/incre/grammar/incre_component_collector.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;
using namespace incre::grammar;

SynthesisComponent::SynthesisComponent(int _command_id): command_id(_command_id){
}

#include "istool/sygus/theory/basic/clia/clia_semantics.h"
#include "istool/ext/deepcoder/data_type.h"

namespace {
    NonTerminal* _getSymbol(const std::unordered_map<std::string, NonTerminal *> &symbol_map, Type* type) {
        auto feature = type->getName();
        auto it = symbol_map.find(feature);
        if (it != symbol_map.end()) return it->second;
        return nullptr;
    }

    void _insertSymbol(std::unordered_map<std::string, NonTerminal*>& symbol_map, const PType& type) {
        auto feature = type->getName();
        if (symbol_map.find(feature) != symbol_map.end()) return;
        symbol_map[feature] = new NonTerminal(feature, type);
    }

    NTList _getAllSymbols(const std::unordered_map<std::string, NonTerminal*>& symbol_map) {
        NTList res;
        for (auto& [_, symbol]: symbol_map) res.push_back(symbol);
        return res;
    }
}

IncreComponent::IncreComponent(const std::string &_name, const PType &_type, const Data &_data, const Term& _term, int _command_id):
        SynthesisComponent(_command_id), name(_name), type(_type), data(_data), term(_term) {
}
void IncreComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    _insertSymbol(symbol_map, type);
}
void IncreComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto* symbol = _getSymbol(symbol_map, type.get());
    if (symbol) {
        auto sem = semantics::buildConstSemantics(data); sem->name = name;
        symbol->rule_list.push_back(new ConcreteRule(sem, {}));
    }
}
Term IncreComponent::tryBuildTerm(const PSemantics &sem, const TermList &term_list) {
    auto* cs = dynamic_cast<ConstSemantics*>(sem.get());
    if (!cs || sem->getName() != name) return nullptr;
    return term;
}

ConstComponent::ConstComponent(const PType &_type, const DataList &_const_list,
                               const std::function<bool(Value *)> &_is_inside):
                               type(_type), const_list(_const_list), is_inside(_is_inside), SynthesisComponent(-1) {
}

void ConstComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    _insertSymbol(symbol_map, type);
}

void ConstComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    for (auto& data: const_list) {
        auto sem = semantics::buildConstSemantics(data);
        auto* symbol = _getSymbol(symbol_map, type.get()); assert(symbol);
        symbol->rule_list.push_back(new ConcreteRule(sem, {}));
    }
}
Term ConstComponent::tryBuildTerm(const PSemantics &sem, const TermList &term_list) {
    auto* cs = dynamic_cast<ConstSemantics*>(sem.get());
    if (!cs || !is_inside(cs->w.get())) return nullptr;
    return std::make_shared<TmValue>(cs->w);
}

BasicOperatorComponent::BasicOperatorComponent(const std::string &_name, const PSemantics &__sem):
    SynthesisComponent(-1), name(_name), _sem(__sem){
    sem = dynamic_cast<TypedSemantics*>(_sem.get()); assert(sem);
}

namespace {
    PType _replace(const PType& type) {
        if (dynamic_cast<TVar*>(type.get())) return theory::clia::getTInt();
        return type;
    }
}

void BasicOperatorComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    _insertSymbol(symbol_map, _replace(sem->oup_type));
}

void BasicOperatorComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto target = _getSymbol(symbol_map, _replace(sem->oup_type).get());
    if (!target) return;
    NTList param_list;
    for (auto& inp_type: sem->inp_type_list) {
        auto param_symbol = _getSymbol(symbol_map, _replace(inp_type).get());
        if (!param_symbol) return;
        param_list.push_back(param_symbol);
    }
    target->rule_list.push_back(new ConcreteRule(_sem, param_list));
}

Term BasicOperatorComponent::tryBuildTerm(const PSemantics &current_sem, const TermList &term_list) {
    if (current_sem != _sem) return nullptr;
    auto res = incre::getOperator(name);
    for (int i = 0; i < term_list.size(); ++i) {
        res = std::make_shared<TmApp>(res, term_list[i]);
    }
    return res;
}

IteComponent::IteComponent(): SynthesisComponent(-1) {
}

void IteComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
}
void IteComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto ti = theory::clia::getTInt(), tb = type::getTBool();
    auto* term = _getSymbol(symbol_map, ti.get());
    auto* cond = _getSymbol(symbol_map, tb.get());
    if (term && cond) {
        term->rule_list.push_back(new ConcreteRule(std::make_shared<IteSemantics>(), {cond, term, term}));
    }
}
Term IteComponent::tryBuildTerm(const PSemantics& sem, const TermList &term_list) {
    if (!dynamic_cast<IteSemantics*>(sem.get())) return nullptr;
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

ApplyComponent::ApplyComponent(Context* _ctx, bool _is_only_full): is_only_full(_is_only_full),
    SynthesisComponent(-1), ctx(_ctx) {
}
Term ApplyComponent::tryBuildTerm(const PSemantics& sem, const TermList &term_list) {
    if (!dynamic_cast<TmAppSemantics*>(sem.get())) return nullptr;
    auto current = term_list[0];
    for (int i = 1; i < term_list.size(); ++i) {
        current = std::make_shared<TmApp>(current, term_list[i]);
    }
    return current;
}

void ApplyComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    if (is_only_full) {
        auto symbol_list = _getAllSymbols(symbol_map);
        for (auto* func: symbol_list) {
            auto current = func->type;
            while (1) {
                auto* ta = dynamic_cast<TArrow*>(current.get());
                if (!ta) {
                    _insertSymbol(symbol_map, current);
                    break;
                }
                assert(ta->inp_types.size() == 1);
                _insertSymbol(symbol_map, ta->inp_types[0]);
                current = ta->oup_type;
            }
        }
    } else {
        auto symbol_list = _getAllSymbols(symbol_map);
        for (auto* func: symbol_list) {
            auto current = func->type;
            while (1) {
                _insertSymbol(symbol_map, current);
                auto* ta = dynamic_cast<TArrow*>(current.get());
                if (!ta) break;
                _insertSymbol(symbol_map, ta->inp_types[0]);
                assert(ta->inp_types.size() == 1);
                current = ta->oup_type;
            }
        }
    }
}
void ApplyComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto semantics = std::make_shared<TmAppSemantics>(ctx);
    if (is_only_full) {
        auto symbol_list = _getAllSymbols(symbol_map);
        for (auto* func: symbol_list) {
            auto current = func->type;
            NTList param_list = {func};
            while (1) {
                auto* ta = dynamic_cast<TArrow*>(current.get());
                if (!ta) break; assert(ta->inp_types.size() == 1);
                param_list.push_back(_getSymbol(symbol_map, ta->inp_types[0].get()));
                current = ta->oup_type;
            }
            if (param_list.size() == 1) continue;
            auto* res = _getSymbol(symbol_map, current.get());

            bool is_valid = res;
            for (auto* param: param_list) {
                if (!param) is_valid = false;
            }
            if (is_valid) res->rule_list.push_back(new ConcreteRule(semantics, param_list));
        }
    } else {
        for (auto* func: _getAllSymbols(symbol_map)) {
            auto current = func->type;
            auto* ta = dynamic_cast<TArrow*>(current.get());
            if (!ta) continue; assert(ta->inp_types.size() == 1);
            auto param = _getSymbol(symbol_map, ta->inp_types[0].get());
            auto res = _getSymbol(symbol_map, ta->oup_type.get());
            if (param && res) res->rule_list.push_back(new ConcreteRule(semantics, {func, param}));
        }
    }
}

#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/incre/trans/incre_trans.h"

TupleComponent::TupleComponent(): SynthesisComponent( -1) {
}

void TupleComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto symbol_list = _getAllSymbols(symbol_map);
    for (auto* symbol: symbol_list) {
        auto* tp = dynamic_cast<TProduct*>(symbol->type.get());
        if (tp) {
            for (auto& sub_type: tp->sub_types) {
                _insertSymbol(symbol_map, sub_type);
            }
        }
    }
}
void TupleComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto sem = std::make_shared<ProductSemantics>();
    for (auto* symbol: _getAllSymbols(symbol_map)) {
        auto* tp = dynamic_cast<TProduct*>(symbol->type.get());
        if (!tp) continue;
        NTList param_list;
        for (auto& sub_type: tp->sub_types) {
            param_list.push_back(_getSymbol(symbol_map, sub_type.get()));
        }

        bool is_valid = true;
        for (auto* param: param_list) {
            if (!param) is_valid = false;
        }
        if (is_valid) symbol->rule_list.push_back(new ConcreteRule(sem, param_list));
    }
}
Term TupleComponent::tryBuildTerm(const PSemantics& sem, const TermList &term_list) {
    if (!dynamic_cast<ProductSemantics*>(sem.get())) return nullptr;
    return std::make_shared<TmTuple>(term_list);
}

ProjComponent::ProjComponent(): SynthesisComponent(-1) {
}

void ProjComponent::extendNTMap(std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    auto symbol_list = _getAllSymbols(symbol_map);
    for (auto* symbol: symbol_list) {
        auto* tp = dynamic_cast<TProduct*>(symbol->type.get());
        if (tp) {
            for (auto& sub_type: tp->sub_types) {
                _insertSymbol(symbol_map, sub_type);
            }
        }
    }
}
void ProjComponent::insertComponent(const std::unordered_map<std::string, NonTerminal *> &symbol_map) {
    for (auto* symbol: _getAllSymbols(symbol_map)) {
        auto* tp = dynamic_cast<TProduct*>(symbol->type.get());
        if (!tp) continue;
        for (int i = 0; i < tp->sub_types.size(); ++i) {
            auto sem = std::make_shared<AccessSemantics>(i);
            auto* target = _getSymbol(symbol_map, tp->sub_types[i].get());
            if (target) target->rule_list.push_back(new ConcreteRule(sem, {symbol}));
        }
    }
}
Term ProjComponent::tryBuildTerm(const PSemantics &sem, const TermList &term_list) {
    auto* as = dynamic_cast<AccessSemantics*>(sem.get());
    if (!as) return nullptr;
    assert(term_list.size() == 1);
    return std::make_shared<TmProj>(term_list[0], as->id + 1);
}

ComponentPool::ComponentPool(const SynthesisComponentList &_align_list, const SynthesisComponentList &_compress_list,
                             const SynthesisComponentList &_comb_list):
                             align_list(_align_list), compress_list(_compress_list), comb_list(_comb_list) {
}
ComponentPool::ComponentPool() {
}

void ComponentPool::print() const {
    std::vector<std::pair<std::string, SynthesisComponentList>> all_components = {
            {"compress", compress_list}, {"align", align_list}, {"comb", comb_list}
    };
    for (auto& [name, comp_list]: all_components) {
        std::cout << "Components for " << name << ":" << std::endl;
        for (auto& comp: comp_list) {
            auto* uc = dynamic_cast<IncreComponent*>(comp.get());
            if (uc) std::cout << "  " << uc->term->toString() << " " << uc->type->getName() << " " << uc->command_id << std::endl;
            auto* bc = dynamic_cast<BasicOperatorComponent*>(comp.get());
            if (bc) std::cout << "  " << bc->_sem->getName() << " " << type::typeList2String(bc->sem->inp_type_list) << " " << bc->sem->oup_type->getName() << std::endl;
        }
        std::cout << std::endl;
    }
}

TypeLabeledDirectSemantics::TypeLabeledDirectSemantics(const PType &_type): NormalSemantics(_type->getName(), _type, {_type}), type(_type) {
}
Data TypeLabeledDirectSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    return inp_list[0];
}

namespace {
    Grammar* _buildGrammar(const TypeList& inp_list, const SynthesisComponentList& component_list, const std::function<bool(Type*)>& is_oup, bool is_single) {
        std::unordered_map<std::string, NonTerminal*> symbol_map;
        for (int i = 0; i < inp_list.size(); ++i) {
            _insertSymbol(symbol_map, inp_list[i]);
            auto* symbol = _getSymbol(symbol_map, inp_list[i].get());
            symbol->rule_list.push_back(new ConcreteRule(semantics::buildParamSemantics(i, inp_list[i]), {}));
        }
        int pre_size;
        do {
            pre_size = symbol_map.size();
            for (auto& component: component_list) {
                component->extendNTMap(symbol_map);
            }
        } while (pre_size < symbol_map.size());


        for (auto& component: component_list) {
            component->insertComponent(symbol_map);
        }
        NTList start_list, symbol_list;
        for (auto& [_, symbol]: symbol_map) {
            symbol_list.push_back(symbol);
            if (is_oup(symbol->type.get())) start_list.push_back(symbol);
        }
        if (start_list.empty()) {
            auto* dummy_symbol = new NonTerminal("start", type::getTBool());
            return new Grammar(dummy_symbol, {dummy_symbol});
        }
        if (is_single) {
            assert(start_list.size() == 1);
            auto* grammar = new Grammar(start_list[0], symbol_list, true);
            return grammar;
        }
        auto* start_symbol = new NonTerminal("start", type::getTVarA());
        symbol_list.push_back(start_symbol);
        for (auto* possible: start_list) {
            auto sem = std::make_shared<TypeLabeledDirectSemantics>(possible->type);
            start_symbol->rule_list.push_back(new ConcreteRule(sem, {possible}));
        }
        auto* grammar = new Grammar(start_symbol, symbol_list, true);
        return grammar;
    }

    bool _isPrimaryType(Type* type) {
        return dynamic_cast<TBool*>(type) || dynamic_cast<TInt*>(type);
    }
    bool _isCompressType(Type* type) {
        return dynamic_cast<TCompress*>(type);
    }
    bool _isCompressOrPrimaryType(Type* type) {
        return _isPrimaryType(type) || _isCompressType(type);
    }
}

Grammar *ComponentPool::buildAlignGrammar(const TypeList &inp_list) {
    return _buildGrammar(inp_list, align_list, _isPrimaryType, false);
}

Grammar *ComponentPool::buildCompressGrammar(const TypeList &inp_list, int command_id) {
    SynthesisComponentList component_list;
    for (auto& component: compress_list) {
        if (component->command_id < command_id) {
            component_list.push_back(component);
        }
    }
    return _buildGrammar(inp_list, component_list, _isCompressOrPrimaryType, false);
}

Grammar *ComponentPool::buildCombinatorGrammar(const TypeList &inp_list, const PType &oup_type, int command_id) {
    LOG(INFO) << "comb grammar";
    SynthesisComponentList component_list;
    for (auto& component: comb_list) {
        if (component->command_id < command_id) {
            component_list.push_back(component);
        }
    }
    return _buildGrammar(inp_list, component_list, [&](Type* type){return type::equal(type, oup_type.get());}, true);
}

const std::string incre::grammar::collector::KCollectMethodName = "Collector@CollectMethodName";

namespace {
    const ComponentCollectorType default_type = ComponentCollectorType::LABEL;
}

ComponentPool incre::grammar::collectComponent(Context* ctx, Env* env, ProgramData* program) {
    auto* ref = env->getConstRef(collector::KCollectMethodName, BuildData(Int, default_type));
    auto collector_type = static_cast<ComponentCollectorType>(theory::clia::getIntValue(*ref));
    switch (collector_type) {
        case ComponentCollectorType::LABEL: return collector::collectComponentFromLabel(ctx, env, program);
        case ComponentCollectorType::SOURCE: return collector::collectComponentFromSource(ctx, env, program);
    }
}

#define RegisterComponent(type, comp) basic.type ## _list.push_back(comp)
#define RegisterAll(comp) RegisterComponent(comb, comp), RegisterComponent(align, comp), RegisterComponent(compress, comp)

ComponentPool incre::grammar::collector::getBasicComponentPool(Context* ctx, Env* env, bool is_full_apply) {
    ComponentPool basic;
    // insert basic operator
    const std::vector<std::string> op_list = {"+", "-", "*", "=", "<", "and", "or", "not"};
    const std::unordered_set<std::string> all_used_op = {"+", "-"};

    for (auto op_name: op_list) {
        auto sem = env->getSemantics(op_name);
        auto comp = std::make_shared<BasicOperatorComponent>(op_name, sem);
        if (all_used_op.find(op_name) != all_used_op.end()) RegisterAll(comp); else RegisterComponent(comb, comp);
    }

    // insert const operator
    auto ic = std::make_shared<ConstComponent>(theory::clia::getTInt(), (DataList){BuildData(Int, 0)},
                                               [](Value* value)->bool {return dynamic_cast<IntValue*>(value);});
    RegisterAll(ic);
    auto ib = std::make_shared<ConstComponent>(type::getTBool(), (DataList){},
                                               [](Value* value) -> bool {return dynamic_cast<BoolValue*>(value);});
    RegisterAll(ib);

    // insert language constructs
    RegisterAll(std::make_shared<IteComponent>());
    RegisterAll(std::make_shared<ProjComponent>());
    RegisterAll(std::make_shared<TupleComponent>());
    RegisterAll(std::make_shared<ApplyComponent>(ctx, is_full_apply));
    return basic;
}