//
// Created by pro on 2022/11/22.
//

#include <istool/ext/deepcoder/data_type.h>
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/trans/incre_trans.h"
#include "glog/logging.h"

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
    SynthesisComponent* _buildSynthesisComponent(Context* ctx, TypeContext* type_ctx, const std::string& name,
            const std::unordered_map<std::string, bool>& compress_map) {
        auto bind = ctx->binding_map[name];
        if (bind->getType() != BindingType::TERM) return nullptr;
        auto* tb = dynamic_cast<TermBinding*>(bind.get());
        assert(compress_map.find(name) != compress_map.end());
        if (compress_map.find(name)->second) return nullptr;

        auto type = incre::typeFromIncre(incre::unfoldAll(tb->type, type_ctx));
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
        auto* ite = new SynthesisComponent(ComponentType::COMB, {TBOOL, TINT, TINT}, TINT, env->getSemantics("ite"), [=](const TermList& term_list) -> Term {
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

        std::vector<std::pair<PType, Data>> const_list = {{TINT, BuildData(Int, 0)}, {TINT, BuildData(Int, 1)}};

        for (auto [type, value]: const_list) {
            auto term = std::make_shared<TmValue>(value);
            auto* comp = new SynthesisComponent(ComponentType::BOTH, {}, type, semantics::buildConstSemantics(value), [=](const TermList& term_list) -> Term {
               assert(term_list.empty());
               return term;
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
std::vector<SynthesisComponent *> incre::collectComponentList(Context *ctx, Env *env, const std::unordered_map<std::string, bool>& compress_map) {
    auto component_list = _getBasicSynthesisComponent(env);
    auto* type_ctx = new TypeContext(ctx);
    for (const auto& [name, _]: ctx->binding_map) {
        auto* comp = _buildSynthesisComponent(ctx, type_ctx, name, compress_map);
        if (comp) component_list.push_back(comp);
    }
    delete type_ctx;
    return component_list;
}

