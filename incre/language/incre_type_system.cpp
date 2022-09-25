//
// Created by pro on 2022/9/20.
//

#include "istool/incre/language/incre.h"
#include "glog/logging.h"

using namespace incre;

namespace {
    int _lookup(const std::vector<std::string>& name_list, const std::string& name) {
        for (int i = name_list.size(); i; --i) {
            if (name_list[i - 1] == name) return i;
        }
        return 0;
    }
}

Ty incre::unfoldType(const Ty &x, TypeContext *ctx, const std::vector<std::string> &tmp_names) {
    auto* tv = dynamic_cast<TyVar*>(x.get());
    if (tv && !_lookup(tmp_names, tv->name)) return unfoldType(ctx->lookup(tv->name), ctx, tmp_names);
    return x;
}

namespace {

#define TypeEqualCase(name) return _isTypeEqual(dynamic_cast<Ty ## name*>(x.get()), dynamic_cast<Ty ## name*>(y.get()), ctx, x_tmp, y_tmp)
#define TypeEqualHead(name) bool _isTypeEqual(Ty ## name * x, Ty ## name* y, TypeContext* ctx, std::vector<std::string>& x_tmp, std::vector<std::string>& y_tmp)

    // TODO: May be error when type binding is reloaded
    bool _isTypeEqual(const Ty &_x, const Ty &_y, TypeContext *ctx, std::vector<std::string> &x_tmp,
                      std::vector<std::string> &y_tmp);

    TypeEqualHead(Int) { return x && y; }

    TypeEqualHead(Bool) { return x && y; }

    TypeEqualHead(Unit) { return x && y; }

    TypeEqualHead(Tuple) {
        if (!x || !y || x->fields.size() != y->fields.size()) return false;
        for (int i = 0; i < x->fields.size(); ++i) {
            if (!_isTypeEqual(x->fields[i], y->fields[i], ctx, x_tmp, y_tmp)) return false;
        }
        return true;
    }

    TypeEqualHead(Arrow) {
        if (!x || !y) return false;
        return _isTypeEqual(x->source, y->source, ctx, x_tmp, y_tmp) &&
               _isTypeEqual(x->target, y->target, ctx, x_tmp, y_tmp);
    }

    TypeEqualHead(Var) {
        if (!x || !y) return false;
        return _lookup(x_tmp, x->name) == _lookup(y_tmp, y->name);
    }

    TypeEqualHead(Compress) {
        if (!x || !y) return false;
        return _isTypeEqual(x->content, y->content, ctx, x_tmp, y_tmp);
    }

    TypeEqualHead(Inductive) {
        if (!x || !y || x->constructors.size() != y->constructors.size()) return false;
        std::unordered_map<std::string, Ty> cons_map;
        for (const auto&[cons_name, cons_type]: y->constructors) {
            cons_map[cons_name] = cons_type;
        }
        x_tmp.push_back(x->name);
        y_tmp.push_back(y->name);
        bool res = true;
        for (const auto&[cons_name, cons_type]: x->constructors) {
            if (cons_map.find(cons_name) == cons_map.end()) continue;
            if (!_isTypeEqual(cons_type, cons_map[cons_name], ctx, x_tmp, y_tmp)) {
                res = false;
                break;
            }
        }
        x_tmp.pop_back();
        y_tmp.pop_back();
        return res;
    }

    bool _isTypeEqual(const Ty &_x, const Ty &_y, TypeContext *ctx, std::vector<std::string> &x_tmp,
                      std::vector<std::string> &y_tmp) {
        auto x = unfoldType(_x, ctx, x_tmp);
        auto y = unfoldType(_y, ctx, y_tmp);
        switch (x->getType()) {
            case TyType::INT:
                TypeEqualCase(Int);
            case TyType::BOOL:
                TypeEqualCase(Bool);
            case TyType::UNIT:
                TypeEqualCase(Unit);
            case TyType::TUPLE:
                TypeEqualCase(Tuple);
            case TyType::ARROW:
                TypeEqualCase(Arrow);
            case TyType::VAR:
                TypeEqualCase(Var);
            case TyType::COMPRESS:
                TypeEqualCase(Compress);
            case TyType::IND:
                TypeEqualCase(Inductive);
        }
        LOG(FATAL) << "Unknown type " << x->toString();
    }
}

bool incre::isTypeEqual(const Ty &x, const Ty &y, TypeContext *ctx) {
    std::vector<std::string> x_tmp, y_tmp;
    return _isTypeEqual(x, y, ctx, x_tmp, y_tmp);
}

namespace {
#define GetTypeCase(name) return _getType(dynamic_cast<Tm ## name*>(term.get()), ctx, ext)
#define GetTypeHead(name) Ty _getType(Tm ## name* term, TypeContext* ctx, const ExternalTypeMap& ext)

    GetTypeHead(Tuple) {
        TyList fields;
        for (const auto& field: term->fields) {
            fields.push_back(getType(field, ctx, ext));
        }
        return std::make_shared<TyTuple>(fields);
    }
    GetTypeHead(Value) {
        if (term->data.isNull()) return std::make_shared<TyUnit>();
        auto* v = term->data.get();
        if (dynamic_cast<VInt*>(v)) return std::make_shared<TyInt>();
        if (dynamic_cast<VBool*>(v)) return std::make_shared<TyBool>();
        auto* tv = dynamic_cast<VTyped*>(v);
        if (tv) return tv->type;
        LOG(FATAL) << "User cannot write " << term->data.toString() << " directly.";
    }
    GetTypeHead(Var) {
        return ctx->lookup(term->name);
    }
    GetTypeHead(Proj) {
        auto full_type = unfoldType(getType(term->content, ctx, ext), ctx, {});
        auto* tt = dynamic_cast<TyTuple*>(full_type.get());
        if (!tt) {
            LOG(FATAL) << "Cannot get projection for non-tuple term " << term->content->toString();
        }
        if (term->id <= 0 || term->id > tt->fields.size()) {
            LOG(FATAL) << "Invalid index " << term->id << " for tuple " << term->content->toString();
        }
        return tt->fields[term->id - 1];
    }
    GetTypeHead(Let) {
        auto def = getType(term->def, ctx, ext);
        auto log = ctx->bind(term->name, def);
        auto res = getType(term->content, ctx, ext);
        ctx->cancelBind(log);
        return res;
    }
    GetTypeHead(If) {
        auto c = unfoldType(getType(term->c, ctx, ext), ctx, {});
        if (!dynamic_cast<TyBool*>(c.get())) {
            LOG(FATAL) << "The condition of an if-expression should be boolean, but get " << term->c->toString();
        }
        auto t = getType(term->t, ctx, ext);
        auto f = getType(term->f, ctx, ext);
        if (!incre::isTypeEqual(t, f, ctx)) {
            LOG(FATAL) << "The two branches of an if-expression should have the same type, but get " << term->t->toString() << " and " << term->f->toString();
        }
        return t;
    }
    GetTypeHead(App) {
        auto func = unfoldType(getType(term->func, ctx, ext), ctx, {});
        auto param = getType(term->param, ctx, ext);
        auto* ta = dynamic_cast<TyArrow*>(func.get());
        if (!ta) {
            LOG(FATAL) << "The function used in an app-expression should have type TyArrow, but get " << term->func->toString();
        }
        if (!incre::isTypeEqual(ta->source, param, ctx)) {
            LOG(FATAL) << term->func->toString() << " expects an input of type " << ta->source->toString()
                << ", but get " << term->param->toString() << " of type " << param->toString();
        }
        return ta->target;
    }
    GetTypeHead(Create) {
        auto content = unfoldType(getType(term->def, ctx, ext), ctx, {});
        if (dynamic_cast<TyArrow*>(content.get())) {
            LOG(FATAL) << "Cannot create a compress value for a function, but get " << term->def->toString();
        }
        return std::make_shared<TyCompress>(content);
    }
    GetTypeHead(Abs) {
        auto log = ctx->bind(term->name, term->type);
        auto res = getType(term->content, ctx, ext);
        ctx->cancelBind(log);
        return std::make_shared<TyArrow>(term->type, res);
    }
    GetTypeHead(Fix) {
        auto res = unfoldType(getType(term->content, ctx, ext), ctx, {});
        auto* ta = dynamic_cast<TyArrow*>(res.get());
        if (!ta) {
            LOG(FATAL) << "A fix-expression requires a function, but get " << term->content->toString();
        }
        if (!incre::isTypeEqual(ta->source, ta->target, ctx)) {
            LOG(FATAL) << "The input type and the output type of the function in a fix-expression should be the same, but get " << term->content->toString();
        }
        return ta->source;
    }

    GetTypeHead(Match) {
        auto res = getType(term->def, ctx, ext);
        TyList branch_types;
        for (const auto& [pt, branch]: term->cases) {
            auto logs = bindPattern(pt, res, ctx);
            branch_types.push_back(getType(branch, ctx, ext));
            for (int i = logs.size(); i; --i) ctx->cancelBind(logs[i - 1]);
        }
        for (int i = 1; i < branch_types.size(); ++i) {
            if (!incre::isTypeEqual(branch_types[0], branch_types[i], ctx)) {
                LOG(FATAL) << "The branches of a match-expression should have the same type, but get " << branch_types[0]->toString() << " and " << branch_types[i]->toString();
            }
        }
        return branch_types[0];
    }

    GetTypeHead(Pass) {
        std::vector<TypeContext::BindLog> logs;
        for (int i = 0; i < term->defs.size(); ++i) {
            auto def_type = unfoldType(getType(term->defs[i], ctx, ext), ctx, {});
            auto* ct = dynamic_cast<TyCompress*>(def_type.get());
            if (!ct) LOG(FATAL) << "The definition of a pass-expression should be compressed, but get " << def_type->toString();
            logs.push_back(ctx->bind(term->names[i], ct->content));
        }
        auto res = getType(term->content, ctx, ext);
        for (int i = logs.size(); i; --i) {
            ctx->cancelBind(logs[i - 1]);
        }
        return res;
    }
}

Ty incre::getType(const Term& term, TypeContext* ctx, const ExternalTypeMap& ext) {
    auto term_type = term->getType();
    auto it = ext.find(term_type);
    if (it != ext.end()) {
        return it->second.func(term, ctx, ext);
    }
    switch (term->getType()) {
        case TermType::VALUE: GetTypeCase(Value);
        case TermType::TUPLE: GetTypeCase(Tuple);
        case TermType::VAR: GetTypeCase(Var);
        case TermType::PROJ: GetTypeCase(Proj);
        case TermType::LET: GetTypeCase(Let);
        case TermType::IF: GetTypeCase(If);
        case TermType::APP: GetTypeCase(App);
        case TermType::CREATE: GetTypeCase(Create);
        case TermType::ABS: GetTypeCase(Abs);
        case TermType::FIX: GetTypeCase(Fix);
        case TermType::MATCH: GetTypeCase(Match);
        case TermType::PASS: GetTypeCase(Pass);
    }
    LOG(FATAL) << "Unknown term " << term->toString();
}

Ty incre::getType(const Term &x, Context *ctx, const ExternalTypeMap& ext) {
    auto* tmp_ctx = new TypeContext(ctx);
    auto res = getType(x, tmp_ctx, ext);
    delete tmp_ctx;
    return res;
}

namespace {


    void _addPtBinding(const Pattern& pt, const Ty& type, TypeContext* ctx, std::vector<TypeContext::BindLog>& res);

#define PtBindingHead(name) void _addPtBinding(Pt ## name* pt, const Ty& type, TypeContext* ctx, std::vector<TypeContext::BindLog>& res)
#define PtBindingCase(name) _addPtBinding(dynamic_cast<Pt ## name*>(pt.get()), type, ctx, res)

    PtBindingHead(Var) {
        res.push_back(ctx->bind(pt->name, type));
    }
    PtBindingHead(Tuple) {
        auto* tt = dynamic_cast<TyTuple*>(unfoldType(type, ctx, {}).get());
        if (!tt || pt->pattern_list.size() != tt->fields.size()) {
            LOG(FATAL) << "Pattern " << pt->toString() << " cannot match a value of type " << type->toString();
        }
        for (int i = 0; i < pt->pattern_list.size(); ++i) {
            _addPtBinding(pt->pattern_list[i], tt->fields[i], ctx, res);
        }
    }
    PtBindingHead(Constructor) {
        auto it = incre::getConstructor(unfoldType(type, ctx, {}), pt->name);
        _addPtBinding(pt->pattern, it, ctx, res);
    }

    void _addPtBinding(const Pattern& pt, const Ty& type, TypeContext* ctx, std::vector<TypeContext::BindLog>& res) {
        switch (pt->getType()) {
            case PatternType::VAR: {
                PtBindingCase(Var); return;
            }
            case PatternType::TUPLE: {
                PtBindingCase(Tuple); return;
            }
            case PatternType::UNDER_SCORE: return;
            case PatternType::CONSTRUCTOR: {
                PtBindingCase(Constructor); return;
            }
        }
    }

}

std::vector<TypeContext::BindLog> incre::bindPattern(const Pattern &pt, const Ty &type, TypeContext *ctx) {
    std::vector<TypeContext::BindLog> res;
    _addPtBinding(pt, type, ctx, res);
    return res;
}