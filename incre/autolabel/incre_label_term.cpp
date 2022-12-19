//
// Created by pro on 2022/12/6.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include "istool/incre/language/incre_lookup.h"
#include "glog/logging.h"
#include <unordered_set>

using namespace incre;
using namespace incre::autolabel;

namespace {
    bool _isValidPassResult(TyData* type) {
        match::MatchTask task;
        task.type_matcher = [](TyData* type, const match::MatchContext&) {
            return type->getType() == TyType::ARROW;
        };
        return !match::match(type, task);
    }

    std::pair<bool, z3::expr> __collectPassResultCons(TyData* type, z3::context& ctx) {
        switch (type->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
                return {false, ctx.bool_val(true)};
            case TyType::TUPLE: {
                z3::expr_vector cons_list(ctx);
                auto* tt = dynamic_cast<TyLabeledTuple*>(type); assert(tt);
                bool is_ind_related = false;
                for (auto& field: tt->fields) {
                    auto [res_tag, res_cons] = __collectPassResultCons(field.get(), ctx);
                    is_ind_related |= res_tag;
                    cons_list.push_back(res_cons);
                }
                if (!is_ind_related) return {false, ctx.bool_val(true)};
                return {true, z3::mk_and(cons_list) || tt->is_compress};
            }
            case TyType::IND: {
                auto* ti = dynamic_cast<TyLabeledInductive*>(type); assert(ti);
                return {true, ti->is_compress};
            }
            case TyType::ARROW:
                LOG(FATAL) << "Unexpected type " << type->toString();
        }
    }

    z3::expr _collectPassResultCons(TyData* type, z3::context& ctx) {
        auto res = __collectPassResultCons(type, ctx);
        LOG(INFO) << "Pass constraint " << type->toString();
        std::cout << "  " << res.second.to_string() << std::endl;
        return res.second;
    }

    void __matchType(TyData* x, TyData* y, LabelContext* ctx) {
        if (x->getType() != y->getType()) {
            LOG(FATAL) << "Type mismatch: " << x->toString() << " & " << y->toString();
        }
        switch (x->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
            case TyType::VAR: return; // Assume the base type is matched
            case TyType::TUPLE: {
                auto* xt = dynamic_cast<TyLabeledTuple*>(x); assert(xt);
                auto* yt = dynamic_cast<TyLabeledTuple*>(y); assert(yt);
                for (int i = 0; i < xt->fields.size(); ++i) {
                    __matchType(xt->fields[i].get(), yt->fields[i].get(), ctx);
                }
                ctx->addConstraint(xt->is_compress == yt->is_compress);
                return;
            }
            case TyType::IND: {
                auto* xi = dynamic_cast<TyLabeledInductive*>(x); assert(xi);
                auto* yi = dynamic_cast<TyLabeledInductive*>(y); assert(yi);
                for (int i = 0; i < xi->constructors.size(); ++i) {
                    __matchType(xi->constructors[i].second.get(), yi->constructors[i].second.get(), ctx);
                }
                ctx->addConstraint(xi->is_compress == yi->is_compress);
                return;
            }
            case TyType::ARROW: {
                auto* xa = dynamic_cast<TyArrow*>(x); auto* ya = dynamic_cast<TyArrow*>(y);
                __matchType(xa->source.get(), ya->source.get(), ctx);
                __matchType(xa->target.get(), ya->target.get(), ctx);
                return;
            }
            case TyType::COMPRESS:
                LOG(FATAL) << "Unexpected TyType COMPRESS " << x->toString() << " " << y->toString();
        }
    }

    void _matchType(TyData* x, TyData* y, LabelContext* ctx) {
        LOG(INFO) << "Match type";
        std::cout << "  " << x->toString() << std::endl;
        std::cout << "  " << y->toString() << std::endl;
        __matchType(x, y, ctx);
    }

    void _bindLabeledPattern(PatternData* pt, const Ty& type, LabelContext* ctx, std::vector<std::pair<std::string, Ty>>& matches) {
        switch (pt->getType()) {
            case PatternType::CONSTRUCTOR: {
                auto* pc = dynamic_cast<PtConstructor*>(pt);
                auto* ti = dynamic_cast<TyLabeledInductive*>(type.get());
                ctx->addConstraint(!ti->is_compress);

                auto cleared_type = changeLabel(ti, ctx->ctx.bool_val(false));
                auto bind_log = ctx->bind(ti->name, cleared_type);
                int pos = -1;
                for (int i = 0; i < ti->constructors.size(); ++i) {
                    if (ti->constructors[i].first == pc->name) {
                        pos = i; break;
                    }
                }
                assert(pos != -1);
                _bindLabeledPattern(pc->pattern.get(), unfoldTypeWithZ3Labels(ti->constructors[pos].second, ctx), ctx, matches);
                ctx->cancelBind(bind_log);
                return;
            }
            case PatternType::TUPLE: {
                auto* pp = dynamic_cast<PtTuple*>(pt);
                auto* tt = dynamic_cast<TyLabeledTuple*>(type.get());
                ctx->addConstraint(!tt->is_compress);
                assert(pp->pattern_list.size() == tt->fields.size());
                for (int i = 0; i < pp->pattern_list.size(); ++i) {
                    _bindLabeledPattern(pp->pattern_list[i].get(), tt->fields[i], ctx, matches);
                }
                return;
            }
            case PatternType::VAR: {
                auto* pv = dynamic_cast<PtVar*>(pt);
                matches.emplace_back(pv->name, type);
                return;
            }
            case PatternType::UNDER_SCORE: return;
        }
    }

    std::vector<TypeContext::BindLog> _bindLabeledPattern(PatternData* pt, const Ty& type, LabelContext* ctx) {
        std::vector<std::pair<std::string, Ty>> matches;
        _bindLabeledPattern(pt, type, ctx, matches);
        std::vector<TypeContext::BindLog> log_list;
        for (const auto& [name, type]: matches) log_list.push_back(ctx->bind(name, type));
        return log_list;
    }

    Ty _labelTerm(const Term &term, LabelContext *ctx);
#define LabelTermHead(name) Ty _labelTerm(Tm ## name* term, LabelContext* ctx)
#define LabelTermCase(name) {raw_type = _labelTerm(dynamic_cast<Tm ## name*>(term.get()), ctx); \
    LOG(INFO) << "term label " << term->toString() << std::endl; std::cout << "  " << raw_type->toString() << std::endl; \
    break;}

    LabelTermHead(Value) {
        return getValueType(term->data.get());
    }
    LabelTermHead(Var) {
        LOG(INFO) << "var " << term->name << " " << ctx->lookup(term->name)->toString();
        return ctx->lookup(term->name);
    }
    LabelTermHead(Tuple) {
        TyList fields;
        for (const auto& field: term->fields) fields.push_back(_labelTerm(field, ctx));
        return std::make_shared<TyLabeledTuple>(ctx->ctx.bool_val(false), fields);
    }
    LabelTermHead(Fix) {
        auto type = _labelTerm(term->content, ctx);
        auto* at = dynamic_cast<TyArrow*>(type.get());
        _matchType(at->source.get(), at->target.get(), ctx);
        return at->source;
    }
    LabelTermHead(Abs) {
        auto source = labelType(term->type, ctx);
        auto log = ctx->bind(term->name, source);
        auto target = _labelTerm(term->content, ctx);
        auto res = std::make_shared<TyArrow>(source, target);
        ctx->cancelBind(log);
        return res;
    }
    LabelTermHead(Proj) {
        auto content_type = _labelTerm(term->content, ctx);
        auto* tt = dynamic_cast<TyTuple*>(content_type.get());
        assert(tt);
        return tt->fields[term->id - 1];
    }
    LabelTermHead(Match) {
        auto def_type = _labelTerm(term->def, ctx);
        TyList case_type_list;
        for (auto& [pt, sub_term]: term->cases) {
            auto log_list = _bindLabeledPattern(pt.get(), def_type, ctx);
            case_type_list.push_back(_labelTerm(sub_term, ctx));
            for (int i = int(log_list.size()) - 1; i >= 0; --i) ctx->cancelBind(log_list[i]);
        }
        for (int i = 1; i < case_type_list.size(); ++i) {
            _matchType(case_type_list[0].get(), case_type_list[i].get(), ctx);
        }
        return case_type_list[0];
    }
    LabelTermHead(Let) {
        auto def_type = _labelTerm(term->def, ctx);
        auto log = ctx->bind(term->name, def_type);
        auto content_type = _labelTerm(term->content, ctx);
        ctx->cancelBind(log);
        return content_type;
    }
    LabelTermHead(If) {
        auto cond_type = _labelTerm(term->c, ctx);
        auto t_type = _labelTerm(term->t, ctx), f_type = _labelTerm(term->f, ctx);
        _matchType(t_type.get(), f_type.get(), ctx);
        return t_type;
    }
    LabelTermHead(App) {
        auto func_type = _labelTerm(term->func, ctx);
        auto param_type = _labelTerm(term->param, ctx);
        auto* at = dynamic_cast<TyArrow*>(func_type.get());
        assert(at);
        _matchType(at->source.get(), param_type.get(), ctx);
        return at->target;
    }

    Ty _labelTerm(const Term &term, LabelContext *ctx) {
        Ty raw_type;
        switch (term->getType()) {
            case TermType::VALUE: LabelTermCase(Value)
            case TermType::VAR: LabelTermCase(Var)
            case TermType::TUPLE: LabelTermCase(Tuple)
            case TermType::CREATE:
            case TermType::PASS:
                LOG(FATAL) << "Unexpected term type CREATE and PASS";
            case TermType::FIX: LabelTermCase(Fix)
            case TermType::PROJ: LabelTermCase(Proj)
            case TermType::MATCH: LabelTermCase(Match)
            case TermType::ABS: LabelTermCase(Abs)
            case TermType::LET: LabelTermCase(Let)
            case TermType::IF: LabelTermCase(If)
            case TermType::APP: LabelTermCase(App)
        }

        // is flip
        auto final_type = raw_type;
        if (isCompressibleType(raw_type, ctx)) {
            auto flip_label = ctx->getFlipVar(term.get());
            auto pre_label = getLabel(raw_type.get());
            ctx->addUnlabelInfo(term.get(), pre_label && flip_label);
            final_type = changeLabel(raw_type.get(), flip_label ^ pre_label);
        }

        // is pass
        if (_isValidPassResult(raw_type.get())) {
            LOG(INFO) << "Possible pass " << term->toString();
            auto pass_label = ctx->getPossiblePassVar(term.get());
            ctx->addConstraint(z3::implies(pass_label, _collectPassResultCons(final_type.get(), ctx->ctx)));
        }
        ctx->registerType(term.get(), final_type);
        return final_type;
    }

    struct PassInfo {
        bool is_cons;
        z3::expr cons;
        std::string name;
        PassInfo(const z3::expr& _cons): cons(_cons), is_cons(true) {}
        PassInfo(const std::string& _name, z3::context& ctx): is_cons(false), name(_name), cons(ctx.bool_val(false)) {
        }
    };

    void _getValidPassCons(const Term& term, LabelContext* ctx, std::vector<PassInfo>& info_list) {
        bool is_possible_pass = false;
        {
            auto it = ctx->possible_pass_map.find(term.get());
            if (it != ctx->possible_pass_map.end()) {
                info_list.emplace_back(it->second);
                is_possible_pass = true;
            }
        }
        {
            auto it = ctx->unlabel_info_map.find(term.get());
            std::unordered_set<std::string> unbounded_var_map;
            for (auto& unbounded_var: getUnboundedVars(term.get())) {
                unbounded_var_map.insert(unbounded_var);
            }

            if (it != ctx->unlabel_info_map.end()) {
                z3::expr_vector pass_labels(ctx->ctx);
                for (auto i = int(info_list.size()) - 1; i >= 0; --i) {
                    auto& info = info_list[i];
                    if (info.is_cons) {
                        pass_labels.push_back(info.cons);
                    } else if (unbounded_var_map.find(info.name) != unbounded_var_map.end()) {
                        break;
                    }
                }
                ctx->addConstraint(z3::implies(it->second, z3::mk_or(pass_labels)));
            }
        }

        switch (term->getType()) {
            case TermType::ABS: {
                auto* at = dynamic_cast<TmAbs*>(term.get());
                info_list.emplace_back(at->name, ctx->ctx);
                _getValidPassCons(at->content, ctx, info_list);
                info_list.pop_back();
                break;
            }
            case TermType::LET: {
                auto* lt = dynamic_cast<TmLet*>(term.get());
                _getValidPassCons(lt->def, ctx, info_list);
                info_list.emplace_back(lt->name, ctx->ctx);
                _getValidPassCons(lt->content, ctx, info_list);
                info_list.pop_back();
                break;
            }
            case TermType::MATCH: {
                auto* mt = dynamic_cast<TmMatch*>(term.get());
                _getValidPassCons(mt->def, ctx, info_list);
                for (auto& [pt, sub_term]: mt->cases) {
                    auto name_list = collectNames(pt.get());
                    for (auto& name: name_list) info_list.emplace_back(name, ctx->ctx);
                    _getValidPassCons(sub_term, ctx, info_list);
                    for (auto& _: name_list) info_list.pop_back();
                }
                break;
            }
            default: {
                auto sub_list = incre::getSubTerms(term.get());
                for (const auto& sub: sub_list) {
                    _getValidPassCons(sub, ctx, info_list);
                }
                break;
            }
        }

        if (is_possible_pass) info_list.pop_back();
    }
}

Ty autolabel::labelTerm(const Term &term, LabelContext *ctx) {
    auto res_type = _labelTerm(term, ctx);
    std::vector<PassInfo> info_list;
    _getValidPassCons(term, ctx, info_list);
    return res_type;
}