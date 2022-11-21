//
// Created by pro on 2022/11/16.
//

#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolifter;

namespace {
    NonTerminal* _getSymbol(const PType& type, std::vector<NonTerminal*>& nt_list) {
        // LOG(INFO) << "insert " << type->getName();
        for (auto* symbol: nt_list) {
            if (type::equal(type, symbol->type)) return symbol;
        }
        auto name = type->getName();
        auto* symbol = new NonTerminal(name, type);
        nt_list.push_back(symbol);
        return symbol;
    }

    PType _getCompressType(IncreInfo* info, int compress_id) {
        for (const auto& pass_info: info->pass_infos) {
            for (auto&[name, ty]: pass_info->inp_types) {
                if (ty->getType() == TyType::COMPRESS) {
                    auto *cty = dynamic_cast<TyLabeledCompress *>(ty.get());
                    if (cty && cty->id == compress_id) return incre::typeFromIncre(cty->content);
                }
            }
        }
        LOG(FATAL) << "Compres #" << compress_id << " not found";
    }
}
/*Grammar * IncreAutoLifterSolver::buildConstGrammar(const TypeList &type_list) {
    std::vector<NonTerminal*> nt_list;
    for (int i = 0; i < type_list.size(); ++i) {
        auto* symbol = _getSymbol(type_list[i], nt_list);
        auto ps = semantics::buildParamSemantics(i, type_list[i]);
        symbol->rule_list.push_back(new ConcreteRule(ps, {}));
    }

    for (auto* symbol: nt_list) {
        auto* at = dynamic_cast<TArrow*>(symbol->type.get());
        if (at) {
            NTList param_list = {symbol};
            for (auto& inp_type: at->inp_types) {
                param_list.push_back(_getSymbol(inp_type, nt_list));
            }
            auto* oup_symbol = _getSymbol(at->oup_type, nt_list);
            auto* rule = new ConcreteRule(env->getSemantics("apply"), param_list);
            oup_symbol->rule_list.push_back(rule);
        }
    }

    // TODO: support more data types
    auto oup_type = theory::clia::getTInt();
    auto* start_symbol = _getSymbol(oup_type, nt_list);
    return new Grammar(start_symbol, nt_list);
}

namespace {
}

Grammar * IncreAutoLifterSolver::buildAuxGrammar(int compress_id) {
    auto inp_type = _getCompressType(info, compress_id);
    if (!list::isList(inp_type.get())) LOG(FATAL) << "Unsupported type " << inp_type->getName();

    auto const_list = list::getConstList(env.get());
    auto semantics_list = list::getSemanticsList(inp_type);

    std::vector<NonTerminal*> nt_list;
    for (auto& [type, value]: const_list) {
        // LOG(INFO) << "Const " << value.toString() << "@" << type->getName();
        auto* symbol = _getSymbol(type, nt_list);
        symbol->rule_list.push_back(new ConcreteRule(semantics::buildConstSemantics(value), {}));
    }
    auto* param_symbol = _getSymbol(inp_type, nt_list);
    param_symbol->rule_list.push_back(new ConcreteRule(semantics::buildParamSemantics(0, inp_type), {}));

    for (auto& semantics: semantics_list) {
        NTList param_list;
        auto* ts = dynamic_cast<TypedSemantics*>(semantics.get());
        assert(ts);
        // LOG(INFO) << "Semantics " << semantics->getName() << " " << type::typeList2String(ts->inp_type_list) << " " << ts->oup_type->getName();
        for (const auto& param_type: ts->inp_type_list) param_list.push_back(_getSymbol(param_type, nt_list));
        NonTerminal* source_symbol = _getSymbol(ts->oup_type, nt_list);
        source_symbol->rule_list.push_back(new ConcreteRule(semantics, param_list));
    }

    auto* start_symbol = _getSymbol(theory::clia::getTInt(), nt_list);
    return new Grammar(start_symbol, nt_list);
}

namespace {

    PType _replaceVarA(const PType& type) {
        if (dynamic_cast<TVar*>(type.get())) return theory::clia::getTInt();
        return type;
    }

    Grammar* _buildBasicGrammar(const TypeList& type_list, const std::vector<PSemantics>& semantics_list,
            const std::vector<std::pair<PType, Data>>& const_list, const PType& oup_type) {
        std::vector<NonTerminal*> nt_list;
        for (const auto& sem: semantics_list) {
            auto* ts = dynamic_cast<TypedSemantics*>(sem.get());
            assert(ts);
            auto* symbol = _getSymbol(_replaceVarA(ts->oup_type), nt_list);
            NTList param_list;
            for (const auto& inp_type: ts->inp_type_list) {
                param_list.push_back(_getSymbol(_replaceVarA(inp_type), nt_list));
            }
            symbol->rule_list.push_back(new ConcreteRule(sem, param_list));
        }
        for (const auto& [type, d]: const_list) {
            auto* symbol = _getSymbol(type, nt_list);
            symbol->rule_list.push_back(new ConcreteRule(semantics::buildConstSemantics(d), {}));
        }

        for (int i = 0; i < type_list.size(); ++i) {
            auto* symbol = _getSymbol(type_list[i], nt_list);
            symbol->rule_list.push_back(new ConcreteRule(semantics::buildParamSemantics(i, type_list[i]), {}));
        }

        auto* start_symbol = _getSymbol(oup_type, nt_list);
        auto* g = new Grammar(start_symbol, nt_list);
        return g;
    }
}

Grammar * IncreAutoLifterSolver::buildCombinatorGrammar(const TypeList &type_list) {
    std::vector<std::string> name_list = {"+", "-", "and", "or", "not", "ite", "<", "<=", "="};
    std::vector<PSemantics> semantics_list;
    for (const auto& name: name_list) semantics_list.push_back(env->getSemantics(name));
    std::vector<std::pair<PType, Data>> const_list = {{theory::clia::getTInt(), BuildData(Int, 0)}, {theory::clia::getTInt(), BuildData(Int, 1)}};
    return _buildBasicGrammar(type_list, semantics_list, const_list, theory::clia::getTInt());
}

Grammar * IncreAutoLifterSolver::buildCombinatorCondGrammar(const TypeList &type_list) {
    std::vector<std::string> name_list = {"+", "-", "and", "or", "not", "ite", "<", "<=", "="};
    std::vector<PSemantics> semantics_list;
    for (const auto& name: name_list) semantics_list.push_back(env->getSemantics(name));
    std::vector<std::pair<PType, Data>> const_list = {{theory::clia::getTInt(), BuildData(Int, 0)}, {theory::clia::getTInt(), BuildData(Int, 1)}};
    return _buildBasicGrammar(type_list, semantics_list, const_list, type::getTBool());
}*/

namespace {
    Grammar* _buildGrammar(const std::vector<SynthesisComponent*>& component_list, ComponentType type, const TypeList& inp_types,
            const PType& oup_type) {
        std::vector<NonTerminal*> nt_list;

        // TODO: use a more general treatment
        SynthesisComponent* app_component = nullptr;

        for (auto* component: component_list) {
            if (component->type != ComponentType::BOTH && component->type != type) continue;
            if (component->semantics->getName() == "app") {
                app_component = component; continue;
            }
            auto* symbol = _getSymbol(component->oup_type, nt_list);
            NTList param_list;
            for (const auto& inp_type: component->inp_types) {
                param_list.push_back(_getSymbol(inp_type, nt_list));
            }
            symbol->rule_list.push_back(new ConcreteRule(component->semantics, param_list));
        }

        if (app_component) {
            for (auto* symbol: nt_list) {
                auto* at = dynamic_cast<TArrow*>(symbol->type.get());
                assert(at->inp_types.size() == 1);
                auto* source = _getSymbol(at->inp_types[0], nt_list);
                auto* target = _getSymbol(at->oup_type, nt_list);
                target->rule_list.push_back(new ConcreteRule(app_component->semantics, {symbol, source}));
            }
        }

        auto* start_symbol = _getSymbol(oup_type, nt_list);
        auto* g = new Grammar(start_symbol, nt_list);
        LOG(INFO) << "Build grammar ";
        g->print();
        return g;
    }
}


Grammar * IncreAutoLifterSolver::buildAuxGrammar(int compress_id) {
    auto inp_type = _getCompressType(info, compress_id);
    return _buildGrammar(info->component_list, ComponentType::AUX, {inp_type}, theory::clia::getTInt());
}
Grammar * IncreAutoLifterSolver::buildConstGrammar(const TypeList &type_list) {
    return _buildGrammar(info->component_list, ComponentType::AUX, type_list, theory::clia::getTInt());
}
Grammar * IncreAutoLifterSolver::buildCombinatorGrammar(const TypeList &type_list) {
    return _buildGrammar(info->component_list, ComponentType::COMB, type_list, theory::clia::getTInt());
}
Grammar * IncreAutoLifterSolver::buildCombinatorCondGrammar(const TypeList &type_list) {
    return _buildGrammar(info->component_list, ComponentType::COMB, type_list, type::getTBool());
}
