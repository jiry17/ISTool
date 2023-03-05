//
// Created by pro on 2022/11/16.
//

#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolifter;

namespace {
    NonTerminal* _getSymbol(const PType& type, std::vector<NonTerminal*>& nt_list) {
        for (auto* symbol: nt_list) {
            if (type::equal(type, symbol->type)) {
                return symbol;
            }
        }
        auto name = type->getName();
        auto* symbol = new NonTerminal(name, type);
        nt_list.push_back(symbol);
        return symbol;
    }

    PType _getCompressType(IncreInfo* info, int compress_id) {
        for (const auto& align_info: info->align_infos) {
            for (auto&[name, ty]: align_info->inp_types) {
                if (ty->getType() == TyType::COMPRESS) {
                    auto *cty = dynamic_cast<TyLabeledCompress *>(ty.get());
                    if (cty && cty->id == compress_id) return incre::typeFromIncre(cty->content);
                }
            }
        }
        LOG(FATAL) << "Compress #" << compress_id << " not found";
    }
}

namespace {
    Grammar* _buildGrammar(const std::vector<SynthesisComponent*>& component_list, const TypeList& inp_types,
            const std::function<bool(Type*)>& is_oup_type, bool is_multi_output, const std::function<bool(SynthesisComponent*)>& filter) {
        std::vector<NonTerminal*> nt_list;

        // TODO: use a more general treatment
        SynthesisComponent* app_component = nullptr;

        for (int i = 0; i < inp_types.size(); ++i) {
            auto* symbol = _getSymbol(inp_types[i], nt_list);
            symbol->rule_list.push_back(new ConcreteRule(semantics::buildParamSemantics(i, inp_types[i]), {}));
        }

        for (auto* component: component_list) {
            if (!filter(component)) continue;
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
            for (int i = 0; i < nt_list.size(); ++i) {
                auto* symbol = nt_list[i];
                auto* at = dynamic_cast<TArrow*>(symbol->type.get());
                if (!at) continue;
                assert(at->inp_types.size() == 1);
                auto* source = _getSymbol(at->inp_types[0], nt_list);
                auto* target = _getSymbol(at->oup_type, nt_list);
                target->rule_list.push_back(new ConcreteRule(app_component->semantics, {symbol, source}));
            }
        }

        for (int i = 0; i < nt_list.size(); ++i) {
            auto* symbol = nt_list[i];
            auto* tt = dynamic_cast<TProduct*>(symbol->type.get());
            if (!tt) continue;
            for (int j = 0; j < tt->sub_types.size(); ++j) {
                auto* target = _getSymbol(tt->sub_types[j], nt_list);
                auto sem = std::make_shared<AccessSemantics>(j);
                target->rule_list.push_back(new ConcreteRule(sem, {symbol}));
            }
        }

        NTList possible_start_list;
        for (auto* s: nt_list) {
            if (is_oup_type(s->type.get())) possible_start_list.push_back(s);
        }

        if (!is_multi_output) {
            if (possible_start_list.empty()) {
                auto* dummy_symbol = new NonTerminal("start", type::getTBool());
                return new Grammar(dummy_symbol, {dummy_symbol});
            }
            assert(possible_start_list.size() == 1);
            return new Grammar(possible_start_list[0], nt_list);
        } else {
            auto* start_symbol = new NonTerminal("start", type::getTVarA());
            nt_list.push_back(start_symbol);
            for (auto* possible: possible_start_list) {
                auto sem = std::make_shared<TypeLabeledDirectSemantics>(possible->type);
                start_symbol->rule_list.push_back(new ConcreteRule(sem, {possible}));
            }
            return new Grammar(start_symbol, nt_list);
        }
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
Grammar * IncreAutoLifterSolver::buildAuxGrammar(int compress_id) {
    auto inp_type = _getCompressType(info, compress_id);
    return _buildGrammar(info->component_list, {inp_type}, _isPrimaryType, true, [](SynthesisComponent* component) -> bool {
       return component->type != ComponentType::COMB;
    });
}


Grammar * IncreAutoLifterSolver::buildCompressGrammar(const TypeList &type_list, int align_id) {
    int pos = info->align_infos[align_id]->command_id;
    auto* grammar = _buildGrammar(info->component_list, type_list, _isCompressOrPrimaryType, true, [=](SynthesisComponent* component) -> bool{
        if (component->type == ComponentType::COMB) return false;
        if (pos <= component->info.command_id) return false;
        return !component->info.is_recursive;
    });
    return grammar;
}
Grammar * IncreAutoLifterSolver::buildCombinatorGrammar(const TypeList &type_list, const PType& oup_type, int align_id) {
    auto feature = std::to_string(align_id) + "@" + type::typeList2String(type_list) + "@" + oup_type->getName();
    if (combine_grammar_map.count(feature)) return combine_grammar_map[feature];
    int pos = info->align_infos[align_id]->command_id;
    auto is_oup = [=](Type* type) {
        return type::equal(type, oup_type.get());
    };
    auto* grammar = _buildGrammar(info->component_list, type_list, is_oup, false, [=](SynthesisComponent* component) -> bool{
        if (component->type == ComponentType::AUX) return false;
        if (pos <= component->info.command_id) return false;
        return !component->info.is_recursive && !component->info.is_compress_related;
    });
    return combine_grammar_map[feature] = grammar;
}
