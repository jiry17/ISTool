//
// Created by pro on 2022/9/16.
//

#include "istool/incre/language/incre_context.h"
#include "glog/logging.h"

using namespace incre;

BindingType BindingData::getType() const {return type;}
BindingData::BindingData(const BindingType &_type): type(_type) {}

TypeBinding::TypeBinding(const Ty &_type): BindingData(BindingType::TYPE), type(_type) {}
std::string TypeBinding::toString() const {
    return type->toString();
}
TermBinding::TermBinding(const Term &_term): BindingData(BindingType::TERM), term(_term) {}
std::string TermBinding::toString() const {
    return term->toString();
}

void Context::addBinding(const std::string &name, const Ty &type) {
    binding_map[name] = std::make_shared<TypeBinding>(type);
}

void Context::addBinding(const std::string &name, const Term &term) {
    binding_map[name] = std::make_shared<TermBinding>(term);
}

Ty Context::getType(const std::string &name) {
    if (binding_map.find(name) == binding_map.end()) {
        LOG(FATAL) << "Context error: name " << name << " not found.";
    }
    auto binding = dynamic_cast<TypeBinding*>(binding_map[name].get());
    if (!binding) {
        LOG(FATAL) << "Context error: expected a type bound to " << name << ", but found a term.";
    }
    return binding->type;
}

Term Context::getTerm(const std::string& name) {
    if (binding_map.find(name) == binding_map.end()) {
        LOG(FATAL) << "Context error: name " << name << " not found.";
    }
    auto binding = dynamic_cast<TermBinding*>(binding_map[name].get());
    if (!binding) {
        LOG(FATAL) << "Context error: expected a term bound to " << name << ", but found a type.";
    }
    return binding->term;
}