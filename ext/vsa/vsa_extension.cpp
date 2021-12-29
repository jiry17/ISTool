//
// Created by pro on 2021/12/29.
//

#include "istool/ext/vsa/vsa_extension.h"
#include "glog/logging.h"

VSAExtension::VSAExtension() {
    ext::vsa::loadLogicWitness(this);
}

VSAExtension::~VSAExtension() {
    for (const auto& info: witness_pool) {
        delete info.second;
    }
}

void VSAExtension::registerWitnessFunction(const std::string &name, WitnessFunction *semantics) {
    witness_pool[name] = semantics;
}

WitnessList VSAExtension::getWitness(Semantics *semantics, const WitnessData &oup, const DataList &inp_list) const {
    auto* cs = dynamic_cast<ConstSemantics*>(semantics);
    if (cs) {
        if (oup->isInclude(cs->w)) return {{}};
        return {};
    }
    auto* ps = dynamic_cast<ParamSemantics*>(semantics);
    if (ps) {
        if (oup->isInclude(inp_list[ps->id])) return {{}};
        return {};
    }
    std::string name = semantics->getName();
    if (witness_pool.find(name) == witness_pool.end()) {
        LOG(FATAL) << "VSA ext: unsupported semantics " << name;
    }
    auto* wf = witness_pool.find(name)->second;
    return wf->witness(oup);
}

const std::string KVSAName = "VSA";

VSAExtension * ext::vsa::getExtension(Env *env) {
    auto* res = env->getExtension(KVSAName);
    if (res) {
        auto* tmp = dynamic_cast<VSAExtension*>(res);
        if (!tmp) {
            LOG(FATAL) << "Unmatched extension " << KVSAName;
        }
        return tmp;
    }
    auto* ext = new VSAExtension();
    env->registerExtension(KVSAName, ext);
    return ext;
}

