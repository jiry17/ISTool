//
// Created by pro on 2022/9/21.
//

#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/analysis/incre_instru_type.h"
#include "glog/logging.h"

using namespace incre;

namespace {
    class CompressStructure {
        int f_id, tau_id;
        std::vector<int> f;
        std::vector<int> group;
    public:
        int getFather(int k1) {
            if (f[k1] == k1) return k1;
            return f[k1] = getFather(f[k1]);
        }
        int getFLabel() {
            f.push_back(f_id); return f_id++;
        }
        int getTauLabel() {
            f.push_back(tau_id); return tau_id++;
        }
        void merge(int x, int y) {
            f[getFather(x)] = getFather(y);
        }
        void initGroup() {
            group.resize(f.size());
            int now = 0;
            for (int i = 0; i < f.size(); ++i) {
                if (getFather(i) == i) group[i] = now++;
            }
            for (int i = 0; i < f.size(); ++i) {
                group[i] = group[getFather(i)];
            }
        }
        int getGroup(int k) {
            return group[k];
        }
    };

    void unify(const Ty& _x, const Ty& _y, TypeContext* ctx, CompressStructure* cs, std::vector<std::string>& x_tmp, std::vector<std::string>& y_tmp);

#define UnifyHead(name) void unify(Ty ## name* x, Ty ## name* y, TypeContext* ctx, CompressStructure* cs, std::vector<std::string>& x_tmp, std::vector<std::string>& y_tmp)
#define UnifyCase(name) unify(dynamic_cast<Ty ## name*>(x.get()), dynamic_cast<Ty ## name*>(y.get()), ctx, cs, x_tmp, y_tmp); break

    UnifyHead(Tuple) {
        assert(x->fields.size() == y->fields.size());
        for (int i = 0; i < x->fields.size(); ++i) {
            unify(x->fields[i], y->fields[i], ctx, cs, x_tmp, y_tmp);
        }
    }
    UnifyHead(Inductive) {
        x_tmp.push_back(x->name); y_tmp.push_back(y->name);
        std::unordered_map<std::string, Ty> cons_map;
        for (const auto& [name, ty]: y->constructors) {
            cons_map[name] = ty;
        }
        for (const auto& [name, ty]: x->constructors) {
            assert(cons_map.count(name));
            unify(ty, cons_map[name], ctx, cs, x_tmp, y_tmp);
        }
        x_tmp.pop_back(); y_tmp.pop_back();
    }
    UnifyHead(Arrow) {
        unify(x->source, y->source, ctx, cs, x_tmp, y_tmp);
        unify(x->target, y->target, ctx, cs, x_tmp, y_tmp);
    }
    UnifyHead(Compress) {
        auto* lx = dynamic_cast<TyLabeledCompress*>(x);
        auto* ly = dynamic_cast<TyLabeledCompress*>(y);
        assert(lx && ly);
        cs->merge(lx->id, ly->id);
        unify(lx->content, ly->content, ctx, cs, x_tmp, y_tmp);
    }

    void unify(const Ty& _x, const Ty& _y, TypeContext* ctx, CompressStructure* cs, std::vector<std::string>& x_tmp, std::vector<std::string>& y_tmp) {
        auto x = incre::unfoldType(_x, ctx, x_tmp);
        auto y = incre::unfoldType(_y, ctx, y_tmp);
        assert(x->getType() == y->getType());
        switch (x->getType()) {
            case TyType::VAR:
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
                break;
            case TyType::TUPLE:
                UnifyCase(Tuple);
            case TyType::IND:
                UnifyCase(Inductive);
            case TyType::ARROW:
                UnifyCase(Arrow);
            case TyType::COMPRESS:
                UnifyCase(Compress);
        }
    }

    void unify(const Ty& x, const Ty& y, TypeContext* ctx, CompressStructure* cs) {
        std::vector<std::string> x_tmp, y_tmp;
        unify(x, y, ctx, cs, x_tmp, y_tmp);
    }

#define LabelTypeHead(name) Ty _labelType(Ty ## name* type, CompressStructure* cs)
#define LabelTypeCase(name) return _labelType(dynamic_cast<Ty ## name*>(type.get()), cs)

    Ty _labelType(const Ty& type, CompressStructure* cs);

    LabelTypeHead(Compress) {
        auto res = _labelType(type->content, cs);
        return std::make_shared<TyLabeledCompress>(res, cs->getFLabel());
    }

    LabelTypeHead(Arrow) {
        auto source = _labelType(type->source, cs);
        auto target = _labelType(type->target, cs);
        return std::make_shared<TyArrow>(source, target);
    }

    LabelTypeHead(Inductive) {
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (const auto& [cname, ctype]: type->constructors) {
            cons_list.emplace_back(cname, _labelType(ctype, cs));
        }
        return std::make_shared<TyInductive>(type->name, cons_list);
    }

    LabelTypeHead(Tuple) {
        TyList fields;
        for (const auto& field: type->fields) {
            fields.push_back(_labelType(field, cs));
        }
        return std::make_shared<TyTuple>(fields);
    }

    Ty _labelType(const Ty& type, CompressStructure* cs) {
        switch (type->getType()) {
            case TyType::VAR:
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
                return type;
            case TyType::COMPRESS:
                LabelTypeCase(Compress);
            case TyType::ARROW:
                LabelTypeCase(Arrow);
            case TyType::IND:
                LabelTypeCase(Inductive);
            case TyType::TUPLE:
                LabelTypeCase(Tuple);
        }
    }

    std::pair<Term, Ty> _labelTerm(const Term& term, TypeContext* ctx, CompressStructure* cs);

#define LabelTermHead(name) std::pair<Term, Ty> _labelTerm(Tm ## name* term, const Term& _term, TypeContext* ctx, CompressStructure* cs)
#define LabelTermCase(name) return _labelTerm(dynamic_cast<Tm ## name*>(term.get()), term, ctx, cs)
#define LabelSub(name) auto [name ## _term, name ## _type] = _labelTerm(term->name, ctx, cs)
#define LabelSub2(name) auto [name ## _term, name ## _type] = _labelTerm(name, ctx, cs)

    LabelTermHead(Tuple) {
        TyList type_list; TermList term_list;
        for (const auto& field: term->fields) {
            LabelSub2(field);
            type_list.push_back(field_type);
            term_list.push_back(field_term);
        }
        return {std::make_shared<TmTuple>(term_list), std::make_shared<TyTuple>(type_list)};
    }

    LabelTermHead(Var) {
        return {_term, ctx->lookup(term->name)};
    }

    LabelTermHead(Proj) {
        LabelSub(content);
        content_type = unfoldType(content_type, ctx, {});
        auto* pt = dynamic_cast<TyTuple*>(content_type.get());
        assert(pt);
        assert(term->id > 0 && term->id <= pt->fields.size());
        return {std::make_shared<TmProj>(content_term, term->id), pt->fields[term->id - 1]};
    }

    LabelTermHead(Value) {
        if (term->data.isNull()) return {_term, std::make_shared<TyUnit>()};
        auto* v = term->data.get();
        if (dynamic_cast<VInt*>(v)) return {_term, std::make_shared<TyInt>()};
        if (dynamic_cast<VBool*>(v)) return {_term, std::make_shared<TyBool>()};
        auto* tv = dynamic_cast<VTyped*>(v);
        if (tv) return {_term, tv->type};
        LOG(FATAL) << "User cannot write " << term->data.toString() << " directly.";
    }

    LabelTermHead(Let) {
        LabelSub(def);
        auto log = ctx->bind(term->name, def_type);
        LabelSub(content);
        ctx->cancelBind(log);
        return {std::make_shared<TmLet>(term->name, def_term, content_term), content_type};
    }

    LabelTermHead(If) {
        LabelSub(c); LabelSub(t); LabelSub(f);
        unify(t_type, f_type, ctx, cs);
        return {std::make_shared<TmIf>(c_term, t_term, f_term), t_type};
    }

    LabelTermHead(App) {
        LabelSub(func); LabelSub(param);
        auto* at = dynamic_cast<TyArrow*>(func_type.get());
        assert(at);
        unify(at->source, param_type, ctx, cs);
        return {std::make_shared<TmApp>(func_term, param_term), at->target};
    }

    LabelTermHead(Abs) {
        auto type = _labelType(term->type, cs);
        auto log = ctx->bind(term->name, type);
        LabelSub(content);
        ctx->cancelBind(log);
        return {std::make_shared<TmAbs>(term->name, type, content_term), std::make_shared<TyArrow>(type, content_type)};
    }

    LabelTermHead(Create) {
        LabelSub(def);
        int id = cs->getFLabel();
        return {std::make_shared<TmLabeledCreate>(def_term, id), std::make_shared<TyLabeledCompress>(def_type, id)};
    }

    LabelTermHead(Match) {
        std::vector<std::pair<Pattern, Term>> case_list;
        TyList type_list;
        LabelSub(def);
        for (const auto& [pattern, branch]: term->cases) {
            auto logs = incre::bindPattern(pattern, def_type, ctx);
            LabelSub2(branch);
            type_list.push_back(branch_type);
            case_list.emplace_back(pattern, branch_term);
            for (int i = logs.size(); i; --i) {
                ctx->cancelBind(logs[i - 1]);
            }
        }
        for (int i = 1; i < type_list.size(); ++i) {
            unify(type_list[0], type_list[i], ctx, cs);
        }
        return {std::make_shared<TmMatch>(def_term, case_list), type_list[0]};
    }

    LabelTermHead(Pass) {
        int tau_id = cs->getTauLabel();
        TermList term_list; TyList type_list;
        std::vector<TypeContext::BindLog> bind_list;
        for (const auto& def: term->defs) {
            LabelSub2(def);
            term_list.push_back(def_term);
            type_list.push_back(def_type);
        }
        for (int i = 0; i < type_list.size(); ++i) {
            auto unfolded_type = unfoldType(type_list[i], ctx, {});
            auto* ct = dynamic_cast<TyCompress*>(unfolded_type.get());
            assert(ct);
            bind_list.push_back(ctx->bind(term->names[i], ct->content));
        }
        LabelSub(content);
        for (int i = bind_list.size(); i; --i) {
            ctx->cancelBind(bind_list[i - 1]);
        }
        return {std::make_shared<TmLabeledPass>(term->names, term_list, content_term, tau_id), content_type};
    }

    LabelTermHead(Fix) {
        LabelSub(content);
        auto full_type = unfoldType(content_type, ctx, {});
        auto* at = dynamic_cast<TyArrow*>(full_type.get());
        assert(at);
        unify(at->source, at->target, ctx, cs);
        return {std::make_shared<TmFix>(content_term), at->target};
    }

    std::pair<Term, Ty> _labelTerm(const Term& term, TypeContext* ctx, CompressStructure* cs) {
        switch (term->getType()) {
            case TermType::TUPLE: LabelTermCase(Tuple);
            case TermType::VAR: LabelTermCase(Var);
            case TermType::PROJ: LabelTermCase(Proj);
            case TermType::VALUE: LabelTermCase(Value);
            case TermType::LET: LabelTermCase(Let);
            case TermType::IF: LabelTermCase(If);
            case TermType::APP: LabelTermCase(App);
            case TermType::ABS: LabelTermCase(Abs);
            case TermType::CREATE: LabelTermCase(Create);
            case TermType::MATCH: LabelTermCase(Match);
            case TermType::PASS: LabelTermCase(Pass);
            case TermType::FIX: LabelTermCase(Fix);
        }
    }

    Command _labelCompress(CommandBind* command, TypeContext* ctx, CompressStructure* cs) {
        switch (command->binding->getType()) {
            case BindingType::TYPE: {
                auto* binding = dynamic_cast<TypeBinding*>(command->binding.get());
                auto ty = _labelType(binding->type, cs);
                ctx->bind(command->name, ty);
                return std::make_shared<CommandBind>(command->name, std::make_shared<TypeBinding>(ty));
            }
            case BindingType::TERM: {
                auto* binding = dynamic_cast<TermBinding*>(command->binding.get());
                auto [term, ty] = _labelTerm(binding->term, ctx, cs);
                ctx->bind(command->name, ty);
                return std::make_shared<CommandBind>(command->name, std::make_shared<TermBinding>(term));
            }
        }
    }

    // todo: Add checks for inductive compress, such as "Inductive List = cons {Int, Compress List} | nil {}"
    Command _labelCompress(CommandDefInductive* command, TypeContext* ctx, CompressStructure* cs) {
        auto type = _labelType(command->_type, cs);
        auto* it = dynamic_cast<TyInductive*>(type.get());
        assert(it);
        ctx->bind(it->name, type);
        for (auto [cname, cty]: it->constructors) {
            auto ity = incre::subst(cty, it->name, type);
            ctx->bind(cname, std::make_shared<TyArrow>(ity, type));
        }
        return std::make_shared<CommandDefInductive>(type);
    }

    Ty _relabelType(const Ty& type, CompressStructure* cs);

#define RelabelTypeHead(name) Ty _relabelType(Ty ## name* type, CompressStructure* cs)
#define RelabelTypeCase(name) return _relabelType(dynamic_cast<Ty ## name*>(type.get()), cs)

    RelabelTypeHead(Arrow) {
        auto source = _relabelType(type->source, cs);
        auto target = _relabelType(type->target, cs);
        return std::make_shared<TyArrow>(source, target);
    }
    RelabelTypeHead(Tuple) {
        TyList fields;
        for (const auto& field: type->fields) {
            fields.push_back(_relabelType(field, cs));
        }
        return std::make_shared<TyTuple>(fields);
    }
    RelabelTypeHead(LabeledCompress) {
        assert(type);
        auto id = cs->getGroup(type->id);
        auto content = _relabelType(type->content, cs);
        return std::make_shared<TyLabeledCompress>(content, id);
    }
    RelabelTypeHead(Inductive) {
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (auto& [cname, ctype]: type->constructors) {
            cons_list.emplace_back(cname, _relabelType(ctype,cs));
        }
        return std::make_shared<TyInductive>(type->name, cons_list);
    }

    Ty _relabelType(const Ty& type, CompressStructure* cs) {
        switch (type->getType()) {
            case TyType::VAR:
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
                return type;
            case TyType::ARROW:
                RelabelTypeCase(Arrow);
            case TyType::TUPLE:
                RelabelTypeCase(Tuple);
            case TyType::IND:
                RelabelTypeCase(Inductive);
            case TyType::COMPRESS:
                RelabelTypeCase(LabeledCompress);
        }
    }

    Term _relabelTerm(const Term& term, CompressStructure* cs);

#define RelabelTermHead(name) Term _relabelTerm(Tm ## name* term, CompressStructure* cs)
#define RelabelTermCase(name) return _relabelTerm(dynamic_cast<Tm ## name*>(term.get()), cs)

    RelabelTermHead(Tuple) {
        TermList fields;
        for (const auto& field: term->fields) {
            fields.push_back(_relabelTerm(field, cs));
        }
        return std::make_shared<TmTuple>(fields);
    }
    RelabelTermHead(Fix) {
        return std::make_shared<TmFix>(_relabelTerm(term->content, cs));
    }
    RelabelTermHead(Abs) {
        auto type = _relabelType(term->type, cs);
        auto content = _relabelTerm(term->content, cs);
        return std::make_shared<TmAbs>(term->name, type, content);
    }
    RelabelTermHead(App) {
        auto func = _relabelTerm(term->func, cs);
        auto param = _relabelTerm(term->param, cs);
        return std::make_shared<TmApp>(func, param);
    }
    RelabelTermHead(If) {
        auto c = _relabelTerm(term->c, cs);
        auto t = _relabelTerm(term->t, cs);
        auto f = _relabelTerm(term->f, cs);
        return std::make_shared<TmIf>(c, t, f);
    }
    RelabelTermHead(Match) {
        std::vector<std::pair<Pattern, Term>> cases;
        for (const auto& [pt, branch]: term->cases) {
            cases.emplace_back(pt, _relabelTerm(branch, cs));
        }
        auto def = _relabelTerm(term->def, cs);
        return std::make_shared<TmMatch>(def, cases);
    }
    RelabelTermHead(LabeledPass) {
        assert(term);
        TermList defs;
        for (const auto& def: term->defs) {
            defs.push_back(_relabelTerm(def, cs));
        }
        auto content = _relabelTerm(term->content, cs);
        return std::make_shared<TmLabeledPass>(term->names, defs, content, term->tau_id);
    }
    RelabelTermHead(LabeledCreate) {
        assert(term);
        auto id = cs->getGroup(term->id);
        auto content = _relabelTerm(term->def, cs);
        return std::make_shared<TmLabeledCreate>(content, id);
    }
    RelabelTermHead(Let) {
        auto def = _relabelTerm(term->def, cs);
        auto content = _relabelTerm(term->content, cs);
        return std::make_shared<TmLet>(term->name, def, content);
    }
    RelabelTermHead(Proj) {
        auto content = _relabelTerm(term->content, cs);
        return std::make_shared<TmProj>(content, term->id);
    }

    Term _relabelTerm(const Term& term, CompressStructure* cs) {
        switch (term->getType()) {
            case TermType::VAR:
            case TermType::VALUE:
                return term;
            case TermType::TUPLE:
                RelabelTermCase(Tuple);
            case TermType::FIX:
                RelabelTermCase(Fix);
            case TermType::MATCH:
                RelabelTermCase(Match);
            case TermType::IF:
                RelabelTermCase(If);
            case TermType::CREATE:
                RelabelTermCase(LabeledCreate);
            case TermType::PASS:
                RelabelTermCase(LabeledPass);
            case TermType::APP:
                RelabelTermCase(App);
            case TermType::ABS:
                RelabelTermCase(Abs);
            case TermType::LET:
                RelabelTermCase(Let);
            case TermType::PROJ:
                RelabelTermCase(Proj);
        }
    }

    Command _relabelCompress(CommandBind* command, CompressStructure* cs) {
        switch (command->binding->getType()) {
            case BindingType::TYPE: {
                auto* tb = dynamic_cast<TypeBinding*>(command->binding.get());
                auto type = _relabelType(tb->type, cs);
                return std::make_shared<CommandBind>(command->name, std::make_shared<TypeBinding>(type));
            }
            case BindingType::TERM: {
                auto* tb = dynamic_cast<TermBinding*>(command->binding.get());
                auto term = _relabelTerm(tb->term, cs);
                return std::make_shared<CommandBind>(command->name, std::make_shared<TermBinding>(term));
            }
        }
    }

    Command _relabelCompress(CommandDefInductive* command, CompressStructure* cs) {
        auto type = _relabelType(command->_type, cs);
        return std::make_shared<CommandDefInductive>(type);
    }
}

IncreProgram incre::labelCompress(const IncreProgram &program) {
    CommandList commands;
    auto* cs = new CompressStructure();
    auto* ctx = new TypeContext();
    for (const auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                commands.push_back(_labelCompress(cb, ctx, cs));
                break;
            }
            case CommandType::DEF_IND: {
                auto* db = dynamic_cast<CommandDefInductive*>(command.get());
                commands.push_back(_labelCompress(db, ctx, cs));
                break;
            }
            case CommandType::IMPORT: {
                commands.push_back(command);
                break;
            }
        }
    }
    cs->initGroup();
    CommandList final_commands;
    for (const auto& command: commands) {
        switch (command->getType()) {
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                final_commands.push_back(_relabelCompress(cb, cs));
                break;
            }
            case CommandType::DEF_IND: {
                auto* db = dynamic_cast<CommandDefInductive*>(command.get());
                final_commands.push_back(_relabelCompress(db, cs));
                break;
            }
            case CommandType::IMPORT: {
                final_commands.push_back(command);
                break;
            }
        }
    }
    delete ctx; delete cs;
    return std::make_shared<ProgramData>(final_commands);
}