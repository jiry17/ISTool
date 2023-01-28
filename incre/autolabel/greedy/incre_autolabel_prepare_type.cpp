//
// Created by pro on 2023/1/23.
//

#include "istool/incre/autolabel/incre_autolabel_greedy_solver.h"
#include "istool/incre/analysis/incre_instru_type.h"
#include "glog/logging.h"
#include "istool/incre/autolabel/incre_func_type.h"

using namespace incre;
using namespace incre::autolabel;

namespace {
    struct LabelContext {
    public:
        int id;
        std::vector<int> f, value;
    private:
        int getGroup(int k) {
            return f[k] == k ? k : f[k] = getGroup(f[k]);
        }
    public:
        void set(int k, int w) {
            value[k] = w;
        }
        int getValue(int k) {
            return value[getGroup(k)];
        }
        void merge(int k1, int k2) {
            k1 = getGroup(k1); k2 = getGroup(k2);
            if (value[k1] != -1 && value[k2] != -1) assert(value[k1] == value[k2]);
            value[k1] = std::max(value[k1], value[k2]); f[k2] = k1;
        }
        int getNewId() {
            f.push_back(id); value.push_back(-1);
            return id++;
        }
    };

    Ty _labelAll(const Ty& ty, LabelContext* ctx) {
        Ty base;
        switch (ty->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::VAR:
            case TyType::UNIT: {
                base = ty;
                break;
            }
            case TyType::COMPRESS:
                LOG(FATAL) << "Unexpected TyType COMPRESS";
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(ty.get());
                TyList fields;
                for (auto& field: tt->fields) {
                    fields.push_back(_labelAll(field, ctx));
                }
                base = std::make_shared<TyTuple>(fields);
                break;
            }
            case TyType::IND:
                LOG(FATAL) << "Unexpected TyType IND";
            case TyType::ARROW: {
                auto* ta = dynamic_cast<TyArrow*>(ty.get());
                auto source = _labelAll(ta->source, ctx);
                auto target = _labelAll(ta->target, ctx);
                base = std::make_shared<TyArrow>(source, target);
                break;
            }
        }
        return std::make_shared<TyLabeledCompress>(base, ctx->getNewId());
    }

    std::pair<int, Ty> _unfoldCompress(const Ty& x) {
        auto* lc = dynamic_cast<TyLabeledCompress*>(x.get());
        if (lc) return {lc->id, lc->content};
        auto* tc = dynamic_cast<TyCompress*>(x.get());
        if (!tc) LOG(FATAL) << "Expect TyCompress, but get " << x->toString();
        return {-1, tc->content};
    }

    void _align_error(const Ty& x, const Ty& y) {
        LOG(FATAL) << "Cannot align " << x->toString() << " with " << y->toString();
    }

    void _align(const Ty& x, const Ty& y, LabelContext* ctx) {
        if (x->getType() == TyType::COMPRESS && y->getType() == TyType::COMPRESS) {
            auto [x_id, x_content] = _unfoldCompress(x);
            auto [y_id, y_content] = _unfoldCompress(y);
            if (x_id != -1 && y_id != -1) ctx->merge(x_id, y_id);
            else if (x_id != -1) ctx->set(y_id, 1);
            else if (y_id != -1) ctx->set(x_id, 1);
            _align(x_content, y_content, ctx); return;
        }
        if (x->getType() == TyType::COMPRESS) {
            auto [x_id, x_content] = _unfoldCompress(x);
            if (x_id == -1) _align_error(x, y);
            ctx->set(x_id, 0);
            _align(x_content, y, ctx); return;
        }
        if (y->getType() == TyType::COMPRESS) {
            auto [y_id, y_content] = _unfoldCompress(y);
            if (y_id == -1) _align_error(x, y);
            ctx->set(y_id, 0);
            _align(x, y_content, ctx); return;
        }
        if (x->getType() != y->getType()) _align_error(x, y);
        switch (x->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
            case TyType::VAR:
                return;
            case TyType::COMPRESS:
                assert(0);
            case TyType::ARROW: {
                auto* xa = dynamic_cast<TyArrow*>(x.get());
                auto* ya = dynamic_cast<TyArrow*>(y.get());
                _align(xa->source, ya->source, ctx);
                _align(xa->target, ya->target, ctx);
                return;
            }
            case TyType::TUPLE: {
                auto* xt = dynamic_cast<TyTuple*>(x.get());
                auto* yt = dynamic_cast<TyTuple*>(y.get());
                if (xt->fields.size() != yt->fields.size()) _align_error(x, y);
                for (int i = 0; i < xt->fields.size(); ++i) {
                    _align(xt->fields[i], yt->fields[i], ctx);
                }
                return;
            }
            case TyType::IND: {
                auto* xi = dynamic_cast<TyInductive*>(x.get());
                auto* yi = dynamic_cast<TyInductive*>(y.get());
                if (xi->constructors.size() != yi->constructors.size()) _align_error(x, y);
                std::unordered_map<std::string, int> y_cons_map;
                for (int i = 0; i < yi->constructors.size(); ++i) {
                    y_cons_map[yi->constructors[i].first] = i;
                }
                for (auto& [cons_name, cons_type]: xi->constructors) {
                    auto it = y_cons_map.find(cons_name);
                    if (it == y_cons_map.end()) _align_error(x, y);
                    _align(cons_type, yi->constructors[it->second].second, ctx);
                }
                return;
            }
        }
    }

    Ty _unfoldType(const Ty& x, TypeContext* ctx) {
        ExternalUnfoldRule rule;
        rule.func = [](const Ty& type, TypeContext* ctx, std::vector<std::string>& tmps, const ExternalUnfoldMap& ext_map) -> Ty {
            auto* ct = dynamic_cast<TyCompress*>(type.get());
            auto content = incre::unfoldTypeAll(ct->content, ctx, tmps, ext_map);
            auto* lt = dynamic_cast<TyLabeledCompress*>(type.get());
            return lt ? std::make_shared<TyLabeledCompress>(content, lt->id) : std::make_shared<TyCompress>(content);
        };
        ExternalUnfoldMap ext = {{TyType::COMPRESS, rule}};
        std::vector<std::string> tmps;
        return incre::unfoldTypeAll(x, ctx, tmps, ext);
    }

    void _align(const Ty& x, const Ty& y, LabelContext* label_ctx, TypeContext* type_ctx) {
        auto full_x = _unfoldType(x, type_ctx);
        auto full_y = _unfoldType(y, type_ctx);
        return _align(full_x, full_y, label_ctx);
    }

    Ty _flip(const Ty& x, LabelContext* ctx) {
        if (x->getType() == TyType::COMPRESS) {
            auto [id, content] = _unfoldCompress(x);
            return std::make_shared<TyLabeledCompress>(content, ctx->getNewId());
        }
        return std::make_shared<TyLabeledCompress>(x, ctx->getNewId());
    }

    std::pair<Term, Ty> _prepareVarLabel(const Term& term, TypeContext* type_ctx, LabelContext* label_ctx);

#define VarLabelHead(name) std::pair<Term, Ty> _prepareVarLabel(Tm ## name* term, const Term& _term, TypeContext* type_ctx, LabelContext* label_ctx)
#define VarLabelCase(name) return _prepareVarLabel(dynamic_cast<Tm ## name*>(term.get()), term, type_ctx, label_ctx)
#define Rec(name) auto [name ## _term, name ## _ty] = _prepareVarLabel(term->name, type_ctx, label_ctx)

    VarLabelHead(Var) {
        return {_term, _flip(type_ctx->lookup(term->name), label_ctx)};
    }
    VarLabelHead(Value) {
        return {_term, _flip(incre::getValueType(term->data.get()), label_ctx)};
    }
    VarLabelHead(Proj) {
        Rec(content);
        if (content_ty->getType() == TyType::COMPRESS) {
            auto [id, content] = _unfoldCompress(content_ty);
            label_ctx->set(id, 0); content_ty = content;
        }
        auto* tt = dynamic_cast<TyTuple*>(content_ty.get());
        assert(tt && tt->fields.size() >= term->id);
        return {std::make_shared<TmProj>(content_term, term->id),
                _flip(tt->fields[term->id - 1], label_ctx)};
    }
    VarLabelHead(Func) {
        assert(term);
        std::vector<FuncParamInfo> param_info_list;
        std::vector<TypeContext::BindLog> log_list;
        for (auto& [name, param_ty]: term->param_list) {
            auto labeled_ty = _labelAll(param_ty, label_ctx);
            log_list.push_back(type_ctx->bind(name, labeled_ty));
            param_info_list.emplace_back(name, labeled_ty);
        }
        Ty labeled_rec_target = nullptr;
        if (term->rec_res_type) {
            labeled_rec_target = _labelAll(term->rec_res_type, label_ctx);
            auto full_type = labeled_rec_target;
            for (int i = int(param_info_list.size()) - 1; i >= 0; --i) {
                full_type = std::make_shared<TyArrow>(param_info_list[i].second, full_type);
            }
            log_list.push_back(type_ctx->bind(term->func_name, full_type));
        }
        Rec(content);
        for (int i = int(log_list.size()) - 1; i >= 0; --i) {
            type_ctx->cancelBind(log_list[i]);
        }
        auto res_term = std::make_shared<TmFunc>(term->func_name, labeled_rec_target, param_info_list, content_term);
        auto full_type = content_ty;
        for (int i = int(param_info_list.size()) - 1; i >= 0; --i) {
            full_type = std::make_shared<TyArrow>(param_info_list[i].second, full_type);
        }
        return {res_term, full_type};
    }
    VarLabelHead(If) {
        Rec(c); Rec(t); Rec(f);
        _align(t_ty, f_ty, label_ctx);
        _align(c_ty, std::make_shared<TyBool>(), label_ctx);
        return {std::make_shared<TmIf>(c_term, t_term, f_term), _flip(t_ty, label_ctx)};
    }
    VarLabelHead(App) {
        Rec(func); Rec(param);
        if (func_ty->getType() == TyType::COMPRESS) {
            auto [id, content] = _unfoldCompress(func_ty);
            label_ctx->set(id, 0); func_ty = content;
        }
        auto* at = dynamic_cast<TyArrow*>(func_ty.get());
        assert(at);
        return {std::make_shared<TmApp>(func_term, param_term), at->target};
    }
    VarLabelHead(Tuple) {
        TermList sub_terms; TyList sub_types;
        for (auto& field: term->fields) {
            auto [sub_term, sub_type] = _prepareVarLabel(field, type_ctx, label_ctx);
            sub_terms.push_back(sub_term);
            sub_types.push_back(sub_type);
        }
        auto res_term = std::make_shared<TmTuple>(sub_terms);
        auto res_type = _flip(std::make_shared<TyTuple>(sub_types), label_ctx);
        return {res_term, res_type};
    }
    VarLabelHead(LabeledLet) {
        auto new_label = _labelAll(term->var_type, label_ctx);
        Rec(def); _align(new_label, def_ty, label_ctx);
        auto log = type_ctx->bind(term->name, def_ty);
        Rec(content); type_ctx->cancelBind(log);
        auto res_term = std::make_shared<TmLabeledLet>(term->name, new_label, def_term, content_term);
        auto res_ty = _flip(content_ty, label_ctx);
        return {res_term, res_ty};
    }

    void _bindPattern(const Pattern& pattern, const Ty& type, TypeContext* type_ctx, LabelContext* label_ctx, std::vector<TypeContext::BindLog>& log_list) {
        switch (pattern->getType()) {
            case PatternType::UNDER_SCORE: return;
            case PatternType::VAR: {
                auto* pv = dynamic_cast<PtVar*>(pattern.get());
                log_list.push_back(type_ctx->bind(pv->name, type));
                return;
            }
            case PatternType::TUPLE: {
                auto* pt = dynamic_cast<PtTuple*>(pattern.get());
                auto* tt = dynamic_cast<TyTuple*>(type.get());
                if (type->getType() == TyType::COMPRESS) {
                    auto [id, content] = _unfoldCompress(type);
                    label_ctx->set(id, 0);
                    tt = dynamic_cast<TyTuple*>(content.get());
                }
                assert(tt && pt->pattern_list.size() == tt->fields.size());
                for (int i = 0; i < tt->fields.size(); ++i) {
                    _bindPattern(pt->pattern_list[i], tt->fields[i], type_ctx, label_ctx, log_list);
                }
                return;
            }
            case PatternType::CONSTRUCTOR: {
                auto* pc = dynamic_cast<PtConstructor*>(pattern.get());
                auto _ti = type;
                auto* ti = dynamic_cast<TyInductive*>(type.get());
                if (type->getType() == TyType::COMPRESS) {
                    auto [id, content] = _unfoldCompress(type);
                    label_ctx->set(id, 0); _ti = content;
                    ti = dynamic_cast<TyInductive*>(content.get());
                }
                assert(ti);
                for (auto& [cons_name, cons_type]: ti->constructors) {
                    if (cons_name == pc->name) {
                        auto full_type = incre::subst(cons_type, cons_name, _ti);
                        _bindPattern(pc->pattern, full_type, type_ctx, label_ctx, log_list);
                        return;
                    }
                }
                assert(0);
            }
        }
    }

    VarLabelHead(Match) {
        Rec(def); auto unfolded_ty = _unfoldType(def_ty, type_ctx);
        std::vector<std::pair<Pattern, Term>> cases;
        TyList case_types;
        for (auto& [pt, case_term]: term->cases) {
            std::vector<TypeContext::BindLog> log_list;
            _bindPattern(pt, def_ty, type_ctx, label_ctx, log_list);
            auto [sub_term, sub_ty] = _prepareVarLabel(case_term, type_ctx, label_ctx);
            cases.emplace_back(pt, sub_term); case_types.push_back(sub_ty);
            for (int i = int(log_list.size()) - 1; i >= 0; --i) {
                type_ctx->cancelBind(log_list[i]);
            }
        }
        for (int i = 1; i < case_types.size(); ++i) {
            _align(case_types[0], case_types[i], label_ctx);
        }
        auto res_term = std::make_shared<TmMatch>(def_term, cases);
        return {res_term, _flip(case_types[0], label_ctx)};
    }


    std::pair<Term, Ty> _prepareVarLabel(const Term& term, TypeContext* type_ctx, LabelContext* label_ctx) {
        switch (term->getType()) {
            case TermType::VAR: VarLabelCase(Var);
            case TermType::VALUE: VarLabelCase(Value);
            case TermType::MATCH: VarLabelCase(Match);
            case TermType::PROJ: VarLabelCase(Proj);
            case TermType::WILDCARD: VarLabelCase(Func);
            case TermType::ALIGN:
            case TermType::LABEL:
            case TermType::UNLABEL:
            case TermType::FIX:
            case TermType::ABS:
                LOG(FATAL) << "Unexpected TermType: " << term->toString();
            case TermType::APP: VarLabelCase(App);
            case TermType::TUPLE: VarLabelCase(Tuple);
            case TermType::LET: VarLabelCase(LabeledLet);
            case TermType::IF: VarLabelCase(If);
        }
    }

    Ty _rewriteType(const Ty& type, LabelContext* ctx) {
        switch (type->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
            case TyType::VAR:
                return type;
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(type.get());
                TyList fields;
                for (auto& field: tt->fields) fields.push_back(_rewriteType(field, ctx));
                return std::make_shared<TyTuple>(fields);
            }
            case TyType::COMPRESS: {
                auto [id, content] = _unfoldCompress(type);
                auto sub_res = _rewriteType(content, ctx);
                if (id == -1 || ctx->getValue(id) == 1) {
                    return std::make_shared<TyCompress>(sub_res);
                }
                return sub_res;
            }
            case TyType::ARROW: {
                auto* ta = dynamic_cast<TyArrow*>(type.get());
                auto source = _rewriteType(ta->source, ctx);
                auto target = _rewriteType(ta->target, ctx);
                return std::make_shared<TyArrow>(source, target);
            }
            case TyType::IND: {
                std::vector<std::pair<std::string, Ty>> cons_list;
                auto* ti = dynamic_cast<TyInductive*>(type.get());
                for (auto& [cons_name, cons_type]: ti->constructors) {
                    auto sub_cons = _rewriteType(cons_type, ctx);
                    cons_list.emplace_back(cons_name, sub_cons);
                }
                return std::make_shared<TyInductive>(ti->name, cons_list);
            }
        }
    }

    Term _rewriteTerm(const Term& term, LabelContext* ctx);

#define RewriteHead(name) Term _rewriteTerm(Tm ## name* term, const Term& _term, LabelContext* ctx)
#define RewriteCase(name) return _rewriteTerm(dynamic_cast<Tm ## name*>(term.get()), term, ctx)
#define RewriteRec(name) auto name = _rewriteTerm(term->name, ctx);

    RewriteHead(App) {
        RewriteRec(func); RewriteRec(param);
        return std::make_shared<TmApp>(func, param);
    }
    RewriteHead(Tuple) {
        TermList sub_terms;
        for (auto& field: term->fields) sub_terms.push_back(_rewriteTerm(field, ctx));
        return std::make_shared<TmTuple>(sub_terms);
    }
    RewriteHead(LabeledLet) {
        RewriteRec(def); RewriteRec(content);
        auto new_type = _rewriteType(term->var_type, ctx);
        return std::make_shared<TmLabeledLet>(term->name, new_type, def, content);
    }
    RewriteHead(If) {
        RewriteRec(c); RewriteRec(t); RewriteRec(f);
        return std::make_shared<TmIf>(c, t, f);
    }
    RewriteHead(Proj) {
        RewriteRec(content);
        return std::make_shared<TmProj>(content, term->id);
    }
    RewriteHead(Func) {
        std::vector<FuncParamInfo> info_list;
        for (auto& [name, param_type]: term->param_list) {
            info_list.emplace_back(name, _rewriteType(param_type, ctx));
        }
        RewriteRec(content);
        auto new_rec_res = term->rec_res_type;
        if (new_rec_res) new_rec_res = _rewriteType(new_rec_res, ctx);
        return std::make_shared<TmFunc>(term->func_name, new_rec_res, info_list, content);
    }
    RewriteHead(Match) {
        std::vector<std::pair<Pattern, Term>> cases;
        RewriteRec(def);
        for (auto& [pt, sub_term]: term->cases) {
            cases.emplace_back(pt, _rewriteTerm(sub_term, ctx));
        }
        return std::make_shared<TmMatch>(def, cases);
    }

    Term _rewriteTerm(const Term& term, LabelContext* ctx) {
        switch (term->getType()) {
            case TermType::VAR:
            case TermType::VALUE: return term;
            case TermType::ALIGN:
            case TermType::LABEL:
            case TermType::UNLABEL:
            case TermType::FIX:
            case TermType::ABS:
                LOG(FATAL) << "Unexpected TermType: " << term->toString();
            case TermType::APP: RewriteCase(App);
            case TermType::TUPLE: RewriteCase(Tuple);
            case TermType::LET: RewriteCase(LabeledLet);
            case TermType::IF: RewriteCase(If);
            case TermType::PROJ: RewriteCase(Proj);
            case TermType::WILDCARD: RewriteCase(Func);
            case TermType::MATCH: RewriteCase(Match);
        }
    }
}

IncreProgram GreedyAutoLabelSolver::prepareVarLabel(ProgramData* program) {
    auto* type_ctx = new TypeContext();
    auto* label_ctx = new LabelContext();
    CommandList tmp_commands;
    for (auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::IMPORT: break;
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDefInductive*>(command.get());
                type_ctx->bind(cd->type->name, cd->_type);
                for (auto& [c_name, c_type]: cd->type->constructors) {
                    auto inp_type = incre::subst(c_type, cd->type->name, cd->_type);
                    type_ctx->bind(cd->type->name, std::make_shared<TyArrow>(inp_type, cd->_type));
                }
                tmp_commands.push_back(command);
                break;
            }
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                auto bind = cb->binding;
                switch (bind->getType()) {
                    case BindingType::VAR: break;
                    case BindingType::TYPE: {
                        auto* tb = dynamic_cast<TypeBinding*>(bind.get());
                        type_ctx->bind(cb->name, tb->type);
                        tmp_commands.push_back(command);
                        break;
                    }
                    case BindingType::TERM: {
                        auto* tb = dynamic_cast<TermBinding*>(bind.get());
                        auto [res_term, res_type] = _prepareVarLabel(tb->term, type_ctx, label_ctx);
                        _align(res_type, task.getTargetType(cb->name), label_ctx);
                        type_ctx->bind(cb->name, res_type);
                        auto new_bind = std::make_shared<TermBinding>(res_term);
                        tmp_commands.push_back(std::make_shared<CommandBind>(cb->name, new_bind));
                        break;
                    }
                }
            }
        }
    }

    CommandList commands;
    for (auto& command: tmp_commands) {
        if (command->getType() != CommandType::BIND) {
            commands.push_back(command); continue;
        }
        auto* cb = dynamic_cast<CommandBind*>(command.get());
        if (cb->binding->getType() != BindingType::TERM) {
            commands.push_back(command); continue;
        }
        auto* tb = dynamic_cast<TermBinding*>(cb->binding.get());
        auto new_term = _rewriteTerm(tb->term, label_ctx);
        auto new_bind = std::make_shared<TermBinding>(new_term);
        commands.push_back(std::make_shared<CommandBind>(cb->name,  new_bind));
    }
    delete type_ctx; delete label_ctx;
    return std::make_shared<ProgramData>(commands);
}