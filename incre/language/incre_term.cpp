//
// Created by pro on 2022/9/15.
//

#include "istool/incre/language/incre_term.h"
#include "glog/logging.h"

using namespace incre;

TermData::TermData(TermType _term_type): term_type(_term_type) {}
TermType TermData::getType() const {return term_type;}

TmValue::TmValue(const Data &_data): TermData(TermType::VALUE), data(_data) {}
std::string TmValue::toString() const {
    return data.toString();
}

TmIf::TmIf(const Term &_c, const Term &_t, const Term &_f): TermData(TermType::IF), c(_c), t(_t), f(_f) {}
std::string TmIf::toString() const {
    return "if " + c->toString() + " then " + t->toString() + " else " + f->toString();
}

TmVar::TmVar(const std::string &_name): TermData(TermType::VAR), name(_name) {}
std::string TmVar::toString() const {
    return name;
}

TmLet::TmLet(const std::string &_name, const Term &_def, const Term &_content): TermData(TermType::LET), name(_name), def(_def), content(_content) {
}
std::string TmLet::toString() const {
    return "let " + name + " = " + def->toString() + " in " + content->toString();
}

TmTuple::TmTuple(const TermList &_fields): TermData(TermType::TUPLE), fields(_fields) {}
std::string TmTuple::toString() const {
    std::string res = "{";
    for (int i = 0; i < fields.size(); ++i) {
        if (i) res += ",";
        res += fields[i]->toString();
    }
    return res + "}";
}

TmProj::TmProj(const Term &_content, int _id): TermData(TermType::PROJ), content(_content), id(_id) {}
std::string TmProj::toString() const {
    return content->toString() + "." + std::to_string(id);
}

TmAbs::TmAbs(const std::string &_name, const Ty &_type, const Term &_content): TermData(TermType::ABS),
    name(_name), type(_type), content(_content) {
}
std::string TmAbs::toString() const {
    return "lambda " + name + ": " + type->toString() + "." + content->toString();
}

TmApp::TmApp(const Term &_func, const Term &_param): TermData(TermType::APP), func(_func), param(_param) {
}
std::string TmApp::toString() const {
    return func->toString() + " (" +  param->toString() + ")";
}

TmFix::TmFix(const Term &_content): TermData(TermType::FIX), content(_content) {}
std::string TmFix::toString() const {
    return "fix " + content->toString();
}

TmMatch::TmMatch(const Term &_def, const std::vector<std::pair<Pattern, Term> > &_cases):
    TermData(TermType::MATCH), def(_def), cases(_cases) {
}
std::string TmMatch::toString() const {
    std::string res = "match " + def->toString() + " with ";
    for (int i = 0; i < cases.size(); ++i) {
        if (i) res += " | ";
        auto [pt, term] = cases[i];
        res += pt->toString() + " -> " + term->toString();
    }
    return res;
}

TmCreate::TmCreate(const Term &_def): TermData(TermType::CREATE), def(_def) {}
std::string TmCreate::toString() const {
    return "create " + def->toString();
}

TmPass::TmPass(const std::vector<std::string> &_names, const TermList &_defs, const Term &_content):
    TermData(TermType::PASS), names(_names), defs(_defs), content(_content) {
    if (names.size() != defs.size()) {
        LOG(FATAL) << "In a TmPass, the numbers of names and definitions must be the same";
    }
}
std::string TmPass::toString() const {
    std::string res = "pass (";
    for (int i = 0; i < names.size(); ++i) {
        if (i) res += ", "; res += names[i];
    }
    res += ") from (";
    for (int i = 0; i < defs.size(); ++i) {
        if (i) res += ","; res += defs[i]->toString();
    }
    res += ") to " + content->toString();
    return res;
}

TermList incre::getSubTerms(TermData *term) {
    switch (term->getType()) {
        case TermType::VALUE:
        case TermType::VAR: return {};
        case TermType::CREATE: {
            auto* tc = dynamic_cast<TmCreate*>(term);
            return {tc->def};
        }
        case TermType::APP: {
            auto* ta = dynamic_cast<TmApp*>(term);
            return {ta->func, ta->param};
        }
        case TermType::ABS: {
            auto* ta = dynamic_cast<TmAbs*>(term);
            return {ta->content};
        }
        case TermType::LET: {
            auto* tl = dynamic_cast<TmLet*>(term);
            return {tl->def, tl->content};
        }
        case TermType::MATCH: {
            auto* tm = dynamic_cast<TmMatch*>(term);
            TermList res = {tm->def};
            for (auto& [_, sub]: tm->cases) res.push_back(sub);
            return res;
        }
        case TermType::TUPLE: {
            auto* tt = dynamic_cast<TmTuple*>(term);
            return tt->fields;
        }
        case TermType::PASS: {
            auto* tp = dynamic_cast<TmPass*>(term);
            auto res = tp->defs; res.push_back(tp->content);
            return res;
        }
        case TermType::PROJ: {
            auto* tp = dynamic_cast<TmProj*>(term);
            return {tp->content};
        }
        case TermType::IF: {
            auto* ti = dynamic_cast<TmIf*>(term);
            return {ti->c, ti->t, ti->f};
        }
        case TermType::FIX: {
            auto* tf = dynamic_cast<TmFix*>(term);
            return {tf->content};
        }
    }
}

namespace {
#define ReplaceHead(name) Term _replaceTerm(Tm ## name* term, const Term& _term, const std::function<Term(const Term&)>& replace_func)
#define ReplaceCase(name) return _replaceTerm(dynamic_cast<Tm ## name*>(term.get()), term, replace_func)
#define ReplaceSub(name) auto name = replaceTerm(term->name, replace_func)
    ReplaceHead(Abs) {
        ReplaceSub(content);
        return std::make_shared<TmAbs>(term->name, term->type, content);
    }
    ReplaceHead(App) {
        ReplaceSub(func); ReplaceSub(param);
        return std::make_shared<TmApp>(func, param);
    }
    ReplaceHead(Match) {
        ReplaceSub(def); std::vector<std::pair<Pattern, Term>> cases;
        for (auto& [pt, sub_term]: term->cases) {
            cases.emplace_back(pt, replaceTerm(sub_term, replace_func));
        }
        return std::make_shared<TmMatch>(def, cases);
    }
    ReplaceHead(Create) {
        ReplaceSub(def);
        return std::make_shared<TmCreate>(def);
    }
    ReplaceHead(If) {
        ReplaceSub(c); ReplaceSub(t); ReplaceSub(f);
        return std::make_shared<TmIf>(c, t, f);
    }
    ReplaceHead(Fix) {
        ReplaceSub(content);
        return std::make_shared<TmFix>(content);
    }
    ReplaceHead(Pass) {
        TermList defs;
        for (auto& def: term->defs) defs.push_back(replaceTerm(def, replace_func));
        ReplaceSub(content);
        return std::make_shared<TmPass>(term->names, defs, content);
    }
    ReplaceHead(Let) {
        ReplaceSub(def); ReplaceSub(content);
        return std::make_shared<TmLet>(term->name, def, content);
    }
    ReplaceHead(Tuple) {
        TermList fields;
        for (auto& field: term->fields) fields.push_back(replaceTerm(field, replace_func));
        return std::make_shared<TmTuple>(fields);
    }
    ReplaceHead(Proj) {
        ReplaceSub(content);
        return std::make_shared<TmProj>(content, term->id);
    }
}

Term incre::replaceTerm(const Term& term, const std::function<Term(const Term&)>& replace_func) {
    auto res = replace_func(term);
    if (res) return res;
    switch (term->getType()) {
        case TermType::VALUE:
        case TermType::VAR: return term;
        case TermType::ABS: ReplaceCase(Abs);
        case TermType::APP: ReplaceCase(App);
        case TermType::MATCH: ReplaceCase(Match);
        case TermType::CREATE: ReplaceCase(Create);
        case TermType::PASS: ReplaceCase(Pass);
        case TermType::IF: ReplaceCase(If);
        case TermType::FIX: ReplaceCase(Fix);
        case TermType::LET: ReplaceCase(Let);
        case TermType::TUPLE: ReplaceCase(Tuple);
        case TermType::PROJ: ReplaceCase(Proj);
    }
}

std::string incre::termType2String(TermType type) {
    switch (type) {
        case TermType::VALUE: return "Value";
        case TermType::VAR: return "Var";
        case TermType::ABS: return "Abs";
        case TermType::APP: return "App";
        case TermType::MATCH: return "Match";
        case TermType::CREATE: return "Create";
        case TermType::PASS: return "Pass";
        case TermType::IF: return "If";
        case TermType::FIX: return "Fix";
        case TermType::LET: return "Let";
        case TermType::TUPLE: return "Tuple";
        case TermType::PROJ: return "Proj";
    }
}