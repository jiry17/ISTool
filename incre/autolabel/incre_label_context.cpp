//
// Created by pro on 2022/12/6.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include "istool/incre/language/incre_lookup.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;

TyLabeledInductive::TyLabeledInductive(const z3::expr &_is_compress, const std::string &name, const std::vector<std::pair<std::string, Ty> > &cons_list):
    TyInductive(name, cons_list), is_compress(_is_compress) {
}
std::string TyLabeledInductive::toString() const {
    return is_compress.to_string() + "@" + TyInductive::toString();
}

TyLabeledTuple::TyLabeledTuple(const z3::expr &_is_compress, const TyList &type_list):
    TyTuple(type_list), is_compress(_is_compress) {
}
std::string TyLabeledTuple::toString() const {
    return is_compress.to_string() + "@" + TyTuple::toString();
}

void LabelContext::addConstraint(const z3::expr &cons) {
    cons_list.push_back(cons);
}
LabelContext::LabelContext(): TypeContext(), ctx(), cons_list(ctx), model(ctx) {
}
LabelContext::~LabelContext() {
}
z3::expr LabelContext::getCompressVar(TyData *type) {
    assert(type_compress_map.find(type) == type_compress_map.end());
    auto var = getTmpVar("c");
    type_compress_map.insert({type, var});
    return var;
}
z3::expr LabelContext::getFlipVar(TermData *term) {
    assert(flip_map.find(term) == flip_map.end());
    auto var = getTmpVar("f");
    flip_map.insert({term, var});
    return var;
}
z3::expr LabelContext::getPossiblePassVar(TermData *term) {
    assert(possible_pass_map.find(term) == possible_pass_map.end());
    auto var = getTmpVar("p");
    possible_pass_map.insert({term, var});
    return var;
}
void LabelContext::addUnlabelInfo(TermData *term, const z3::expr &expr) {
    assert(unlabel_info_map.find(term) == unlabel_info_map.end());
    unlabel_info_map.insert({term, expr});
}
void LabelContext::registerType(TermData *term, const Ty &type) {
    assert(term->getType() == TermType::VALUE || labeled_type_map.find(term) == labeled_type_map.end());
    labeled_type_map[term] = type;
}
z3::expr LabelContext::getTmpVar(const std::string &prefix) {
    auto name = prefix + std::to_string(++tmp_ind_map[prefix]);
    return ctx.bool_const(name.c_str());
}
void LabelContext::setModel(const z3::model &_model) {
    model = _model;
}
bool LabelContext::getLabel(const z3::expr &expr) const {
    auto value = model.eval(expr);
    if (value.bool_value() == Z3_L_TRUE) return true;
    if (value.bool_value() == Z3_L_FALSE) return false;
    return false;
}

Ty autolabel::unfoldTypeWithZ3Labels(const Ty &type, TypeContext *ctx) {
    ExternalUnfoldRule tuple_rule = {
            [](const Ty& type, TypeContext* ctx, std::vector<std::string>& tmp_names, const ExternalUnfoldMap& ext) -> Ty{
                auto* lt = dynamic_cast<TyLabeledTuple*>(type.get());
                if (!lt) LOG(FATAL) << "Type " << type->toString() << " has not been labeled";
                TyList fields;
                for (auto& field: lt->fields) {
                    fields.push_back(incre::unfoldTypeAll(field, ctx, tmp_names, ext));
                }
                return std::make_shared<TyLabeledTuple>(lt->is_compress, fields);
            }
    };
    ExternalUnfoldRule ind_rule = {
            [](const Ty& type, TypeContext* ctx, std::vector<std::string>& tmp_names, const ExternalUnfoldMap& ext) -> Ty{
                auto* lt = dynamic_cast<TyLabeledInductive*>(type.get());
                if (!lt) LOG(FATAL) << "Type " << type->toString() << " has not been labeled";
                std::vector<std::pair<std::string, Ty>> cons_list;
                tmp_names.push_back(lt->name);
                for (auto& [name, cons_ty]: lt->constructors) {
                    cons_list.emplace_back(name, incre::unfoldTypeAll(cons_ty, ctx, tmp_names, ext));
                }
                tmp_names.pop_back();
                return std::make_shared<TyLabeledInductive>(lt->is_compress, lt->name, cons_list);
            }
    };
    ExternalUnfoldMap map = {{TyType::TUPLE, tuple_rule}, {TyType::IND, ind_rule}};
    std::vector<std::string> tmp_list;
    return incre::unfoldTypeAll(type, ctx, tmp_list, map);
}

bool autolabel::isLabeledType(TyData *type) {
    return dynamic_cast<TyLabeledInductive*>(type) || dynamic_cast<TyLabeledTuple*>(type);
}
z3::expr autolabel::getLabel(TyData *type) {
    auto* ti = dynamic_cast<TyLabeledInductive*>(type);
    if (ti) return ti->is_compress;
    auto* tt = dynamic_cast<TyLabeledTuple*>(type);
    if (tt) return tt->is_compress;
    LOG(FATAL) << "Type " << type->toString() << " is not labeled";
}
Ty autolabel::changeLabel(TyData *type, const z3::expr& new_label) {
    auto* ti = dynamic_cast<TyLabeledInductive*>(type);
    if (ti) return std::make_shared<TyLabeledInductive>(new_label, ti->name, ti->constructors);
    auto* tt = dynamic_cast<TyLabeledTuple*>(type);
    if (tt) return std::make_shared<TyLabeledTuple>(new_label, tt->fields);
    LOG(FATAL) << "Type " << type->toString() << " is not labeled";
}
bool autolabel::isCompressibleType(const Ty &type, TypeContext *ctx) {
    auto full_type = incre::unfoldBasicType(type, ctx);

    match::MatchTask task;
    task.type_matcher = [](TyData* data, const match::MatchContext&) {
        return data->getType() == TyType::IND;
    };
    if (!match::match(full_type.get(), task)) return false;
    task.type_matcher = [](TyData* data, const match::MatchContext&) {
        return data->getType() == TyType::ARROW;
    };
    return !match::match(full_type.get(), task);
}