//
// Created by pro on 2022/9/26.
//

#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/incre/trans/incre_grammar_builder.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/ext/deepcoder/data_type.h"
#include "glog/logging.h"

namespace {
    class IncreApplySemantics: public Semantics {
    public:
        IncreApplySemantics(): Semantics("apply") {}
        virtual ~IncreApplySemantics() = default;
        virtual Data run(const ProgramList& sub_list, ExecuteInfo* info) {
            auto func = sub_list[0]->run(info);
            auto* av = dynamic_cast<incre::VFunction*>(func.get());
            if (!av) {
                LOG(FATAL) << "Expected a function, but get " << sub_list[0]->toString();
            }
            auto param = sub_list[1]->run(info);
        }
    };
}

Grammar * incre::buildFGrammar(PassTypeInfoData *pass_info, const std::vector<FComponent> &component_list, Context* ctx) {
    auto* type_ctx = new TypeContext(ctx);
    TyList type_pool;
    auto insert_type = [&](const Ty& type) {
        for (int i = 0; i < type_pool.size(); ++i) {
            if (incre::isTypeEqual(type_pool[i], type, type_ctx)) return i;
        }
        type_pool.push_back(type);
        return int(type_pool.size()) - 1;
    };

    insert_type(pass_info->oup_type);
    for (auto& [_, inp_type]: pass_info->inp_types) insert_type(inp_type);
    for (auto& component: component_list) insert_type(component.type);
    for (const auto& type: type_pool) {
        auto* at = dynamic_cast<TyArrow*>(type.get());
        if (at) {
            insert_type(at->source); insert_type(at->target);
            continue;
        }
        auto* tt = dynamic_cast<TyTuple*>(type.get());
        if (tt) {
            for (const auto& field: tt->fields) {
                insert_type(field);
            }
            continue;
        }
    }

    NTList symbols;
    for (const auto& ty: type_pool) {
        std::string name = "S[" + ty->toString() + "]";
        auto type = incre::typeFromIncre(ty);
        symbols.push_back(new NonTerminal(name, type));
    }


    for (int i = 0; i < pass_info->inp_types.size(); ++i) {
        int id = insert_type(pass_info->inp_types[i].second);
        auto* symbol = symbols[id];
        auto semantics = std::make_shared<ParamSemantics>(symbol->type, id);
        symbol->rule_list.push_back(new ConcreteRule(semantics, {}));
    }
    for (auto& component: component_list) {
        int id = insert_type(component.type);
        auto* symbol = symbols[id];
        auto semantics = std::make_shared<ConstSemantics>(component.d, component.name);
        symbol->rule_list.push_back(new ConcreteRule(semantics, {}));
    }

    for (int i = 0; i < type_pool.size(); ++i) {
        auto* symbol = symbols[i];
        auto* at = dynamic_cast<TyArrow*>(type_pool[i].get());
        if (at) {
            auto source_symbol = symbols[insert_type(at->source)];
            auto target_symbol = symbols[insert_type(at->target)];
            auto sem = std::make_shared<IncreApplySemantics>();
            target_symbol->rule_list.push_back(new ConcreteRule(sem, {symbol, source_symbol}));
            continue;
        }
        auto* tt = dynamic_cast<TyTuple*>(type_pool[i].get());
        if (tt) {
            for (int j = 0; j < tt->fields.size(); ++j) {
                auto field_symbol = symbols[insert_type(tt->fields[j])];
                auto sem = std::make_shared<AccessSemantics>(j);
                field_symbol->rule_list.push_back(new ConcreteRule(sem, {symbol}));
            }
            NTList sub_symbols;
            for (int j = 0; j < tt->fields.size(); ++j) {
                auto field_symbol = symbols[insert_type(tt->fields[j])];
                sub_symbols.push_back(field_symbol);
            }
            auto sem = std::make_shared<ProductSemantics>();
            symbol->rule_list.push_back(new ConcreteRule(sem, sub_symbols));
        }
    }

    delete ctx;
    return new Grammar(symbols[insert_type(pass_info->oup_type)], symbols);
}