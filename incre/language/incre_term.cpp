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



