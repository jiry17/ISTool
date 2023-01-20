//
// Created by pro on 2022/11/16.
//

#include <istool/ext/deepcoder/deepcoder_semantics.h>
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
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
        LOG(FATAL) << "Compres #" << compress_id << " not found";
    }
}

namespace {
    Grammar* _buildGrammar(const std::vector<SynthesisComponent*>& component_list, const TypeList& inp_types,
            const TypeList& oup_type_list, bool is_multi_output, const std::function<bool(SynthesisComponent*)>& filter) {
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

        NonTerminal* start_symbol; assert(!oup_type_list.empty());
        if (!is_multi_output) {
            assert(oup_type_list.size() == 1);
            start_symbol = _getSymbol(oup_type_list[0], nt_list);
        } else {
            start_symbol = new NonTerminal("start", std::make_shared<TVar>("res"));
            nt_list.push_back(start_symbol);
            for (auto& oup_type: oup_type_list) {
                auto* symbol = _getSymbol(oup_type, nt_list);
                auto sem = std::make_shared<TypeLabeledDirectSemantics>(oup_type);
                start_symbol->rule_list.push_back(new ConcreteRule(sem, {symbol}));
            }
        }

        auto* g = new Grammar(start_symbol, nt_list);
        return g;
    }

    TypeList _getPrimaryTypes() {
        static TypeList res;
        if (res.empty()) {
            res.push_back(type::getTBool());
            res.push_back(theory::clia::getTInt());
        }
        return res;
    }
}


Grammar * IncreAutoLifterSolver::buildAuxGrammar(int compress_id) {
    auto inp_type = _getCompressType(info, compress_id);
    return _buildGrammar(info->component_list, {inp_type}, _getPrimaryTypes(), true, [](SynthesisComponent* component) -> bool{
        return component->type != ComponentType::COMB;
    });
}
Grammar * IncreAutoLifterSolver::buildConstGrammar(const TypeList &type_list, int align_id) {
    int pos = info->align_infos[align_id]->command_id;
    auto* grammar = _buildGrammar(info->component_list, type_list, _getPrimaryTypes(), true, [=](SynthesisComponent* component) -> bool{
        if (component->type == ComponentType::COMB) return false;
        if (pos <= component->info.command_id) return false;
        return !component->info.is_recursive && !component->info.is_compress_related;
    });
    return grammar;
}
Grammar * IncreAutoLifterSolver::buildCombinatorGrammar(const TypeList &type_list, const PType& oup_type, int align_id) {
    auto feature = std::to_string(align_id) + "@" + type::typeList2String(type_list) + "@" + oup_type->getName();
    if (grammar_map.count(feature)) return grammar_map[feature];
    int pos = info->align_infos[align_id]->command_id;
    auto* grammar = _buildGrammar(info->component_list, type_list, {oup_type}, false, [=](SynthesisComponent* component) -> bool{
        if (component->type == ComponentType::AUX) return false;
        if (pos <= component->info.command_id) return false;
        return !component->info.is_recursive && !component->info.is_compress_related;
    });
    LOG(INFO) << "Grammar";
    grammar->print();
    return grammar_map[feature] = grammar;
}
