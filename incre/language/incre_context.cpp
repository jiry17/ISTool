//
// Created by pro on 2023/12/5.
//
#include "istool/incre/language/incre_context.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::syntax;

EnvAddress::EnvAddress(const std::string &_name, const syntax::Binding &_bind,
                       const std::shared_ptr<EnvAddress> &_next):
                       name(_name), bind(_bind), next(_next) {
}
IncreContext::IncreContext(const std::shared_ptr<EnvAddress> &_start): start(_start) {}

syntax::Ty IncreContext::getType(const std::string &name) const {
    for (auto now = start; now; now = now->next) {
        if (now->name == name) return now->bind.getType();
    }
    LOG(FATAL) << "No type is bound to " << name;
}

Data IncreContext::getData(const std::string &name) const {
    for (auto now = start; now; now = now->next) {
        if (now->name == name) return now->bind.getData();
    }
    LOG(FATAL) << "No data is bound to " << name;
}

IncreContext IncreContext::insert(const std::string &name, const syntax::Binding &binding) const {
    return std::make_shared<EnvAddress>(name, binding, start);
}

IncreFullContextData::IncreFullContextData(const IncreContext &_ctx,
                                           const std::unordered_map<std::string, EnvAddress *> &_address_map):
                                           ctx(_ctx), address_map(_address_map) {
}

void IncreFullContextData::setGlobalInput(const std::unordered_map<std::string, Data> &global_input) {
    if (global_input.size() != address_map.size()) {
        LOG(FATAL) << "Expect " << std::to_string(address_map.size()) << " global inputs, but received " << std::to_string(global_input.size());
    }
    for (auto& [name, value]: global_input) {
        auto it = address_map.find(name);
        if (it == address_map.end()) LOG(FATAL) << "Unknown global input " << name;
        it->second->bind.data = value;
    }
}