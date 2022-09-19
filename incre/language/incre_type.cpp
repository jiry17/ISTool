//
// Created by pro on 2022/9/15.
//

#include "istool/incre/language/incre_type.h"

using namespace incre;

std::string TyInt::toString() const {return "Int";}

std::string TyBool::toString() const {return "Bool";}

std::string TyUnit::toString() const {return "Unit";}

TyTuple::TyTuple(const TyList &_fields): fields(_fields) {}
std::string TyTuple::toString() const {
    std::string res = "{";
    for (int i = 0; i < fields.size(); ++i) {
        if (i) res += ","; res += fields[i]->toString();
    }
    return res + "}";
}

TyInductive::TyInductive(const std::string &_name, const std::vector<std::pair<std::string, Ty> > &_constructors):
    name(_name), constructors(_constructors) {
}
std::string TyInductive::toString() const {
    std::string res = name + ". <";
    for (int i = 0; i < constructors.size(); ++i) {
        auto [name, ty] = constructors[i];
        if (i) res += " | ";
        res += name + " " + ty->toString();
    }
    return res + ">";
}

TyVar::TyVar(const std::string &_name): name(_name) {}
std::string TyVar::toString() const {return name;}

TyCompress::TyCompress(const Ty &_content): content(_content) {}
std::string TyCompress::toString() const {
    return "compress " + content->toString();
}

TyArrow::TyArrow(const Ty &_source, const Ty &_target): source(_source), target(_target) {}
std::string TyArrow::toString() const {
    return "(" + source->toString() + ") -> (" + target->toString() + ")";
}
