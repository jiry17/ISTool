//
// Created by pro on 2022/11/22.
//

#include <istool/ext/deepcoder/data_type.h>
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/trans/incre_trans.h"

using namespace incre;



SynthesisComponent::SynthesisComponent(ComponentType _type, const TypeList &_inp_types, const PType &_oup_type,
                                       const PSemantics &_semantics, const TermBuilder& _builder):
        type(_type), inp_types(_inp_types), oup_type(_oup_type), semantics(_semantics), builder(_builder) {
}
Term SynthesisComponent::buildTerm(const TermList &term_list) {
    return builder(term_list);
}
ComponentSemantics::ComponentSemantics(const Data &_data, const std::string &_name): ConstSemantics(_data) {
    name = _name;
}

namespace {
    bool _containCompress(TyData* type) {
        switch (type->getType()) {
            case TyType::COMPRESS: return true;
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(type);
                for (const auto& field: tt->fields) {
                    if (_containCompress(field.get())) return true;
                }
                return false;
            }
            case TyType::VAR:
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
                return false;
            case TyType::ARROW: {
                auto* at = dynamic_cast<TyArrow*>(type);
                return _containCompress(at->target.get()) || _containCompress(at->source.get());
            }
            case TyType::IND: {
                auto* it = dynamic_cast<TyInductive*>(type);
                for (auto& [name, sub_ty]: it->constructors) {
                    if (_containCompress(sub_ty.get())) return true;
                }
                return false;
            }
        }
    }

    SynthesisComponent* _buildSynthesisComponent(Context* ctx, const std::string& name) {
        auto bind = ctx->binding_map[name];
        if (bind->getType() != BindingType::TYPE) return nullptr;
        auto* tb = dynamic_cast<TermBinding*>(bind.get());
        if (_containCompress(tb->type.get())) return nullptr;

        auto type = incre::typeFromIncre(tb->type);
        assert(tb->term->getType() == TermType::VALUE);
        auto* tv = dynamic_cast<TmValue*>(tb->term.get());
        auto sem = std::make_shared<ComponentSemantics>(tv->data, name);
        return new SynthesisComponent(ComponentType::BOTH, {}, type, sem, [=](const TermList& term_list) -> Term{
            assert(term_list.empty());
            return std::make_shared<TmVar>(name);
        });
    }

    class TmAppSemantics: public FullExecutedSemantics {
    public:
        TmAppSemantics(): FullExecutedSemantics("app") {}
        virtual Data run(DataList&& inp_list, ExecuteInfo* info) {
            assert(inp_list.size() == 2);
            auto* fv = dynamic_cast<VFunction*>(inp_list[0].get());
            return fv->func(std::make_shared<TmValue>(inp_list[1]));
        }
        virtual ~TmAppSemantics() = default;
    };

    std::vector<SynthesisComponent*> _getBasicSynthesisComponent(Env* env) {
        auto TINT = theory::clia::getTInt(), TBOOL = type::getTBool();
        auto ty_int = std::make_shared<TyInt>();
        auto* ite = new SynthesisComponent(ComponentType::COMB, {TBOOL, TINT, TINT}, TBOOL, env->getSemantics("ite"), [=](const TermList& term_list) -> Term {
            assert(term_list.size() == 3);
            return std::make_shared<TmIf>(term_list[0], term_list[1], term_list[2]);
        });
        std::vector<SynthesisComponent*> comp_list; comp_list.push_back(ite);

        for (auto op_name: {"+", "-", "=", "<"}) {
            auto sem = env->getSemantics(op_name);
            auto* ts = dynamic_cast<TypedSemantics*>(sem.get());
            assert(ts);
            auto* comp = new SynthesisComponent(ComponentType::BOTH, {TINT, TINT}, ts->oup_type, sem, [=](const TermList& term_list) -> Term {
                auto op = incre::getOperator(op_name); assert(term_list.size() <= 2);
                if (term_list.empty()) {
                    auto x = std::make_shared<TmVar>("x"), y = std::make_shared<TmVar>("y");
                    auto base = std::make_shared<TmApp>(std::make_shared<TmApp>(op, x), y);
                    auto lam_y = std::make_shared<TmAbs>("y", ty_int, base);
                    return std::make_shared<TmAbs>("x", ty_int, base);
                } else if (term_list.size() == 1) {
                    auto x = std::make_shared<TmVar>("x");
                    auto base = std::make_shared<TmApp>(std::make_shared<TmApp>(op, term_list[0]), x);
                    return std::make_shared<TmAbs>("x", ty_int, base);
                } else return std::make_shared<TmApp>(std::make_shared<TmApp>(op, term_list[0]), term_list[1]);
            });
            comp_list.push_back(comp);
        }

        auto var_a = std::make_shared<TVar>("a"), var_b = std::make_shared<TVar>("b");
        auto var_arrow = std::make_shared<TArrow>((TypeList){var_a}, var_b);
        auto* app = new SynthesisComponent(ComponentType::BOTH, {var_arrow, var_a}, var_b, std::make_shared<TmAppSemantics>(), [=](const TermList& term_list) -> Term {
            assert(term_list.size() == 2);
            return std::make_shared<TmApp>(term_list[0], term_list[1]);
        });
        comp_list.push_back(app);
        return comp_list;
    }
}

// TODO: add lambda expressions

std::vector<SynthesisComponent *> incre::collectComponentList(Context *ctx, Env *env) {
    auto component_list = _getBasicSynthesisComponent(env);
    for (const auto& [name, _]: ctx->binding_map) {
        auto* comp = _buildSynthesisComponent(ctx, name);
        if (comp) component_list.push_back(comp);
    }
    return component_list;
}

