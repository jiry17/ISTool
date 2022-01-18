//
// Created by pro on 2021/12/29.
//

#include "istool/ext/vsa/vsa_extension.h"
#include "glog/logging.h"

VSAExtension::VSAExtension() {
    ext::vsa::loadLogicWitness(this);
    manager_list.push_back(new BasicWitnessManager());
}

VSAExtension::~VSAExtension() {
    for (auto* m: manager_list) delete m;
}

void VSAExtension::registerWitnessManager(WitnessManager *manager) {
    manager_list.push_front(manager);
}
void VSAExtension::registerWitnessFunction(const std::string &name, WitnessFunction *semantics) {
    OperatorWitnessManager* om = nullptr;
    for (auto* m: manager_list) {
        om = dynamic_cast<OperatorWitnessManager*>(m);
        if (om) break;
    }
    if (!om) {
        om = new OperatorWitnessManager();
        registerWitnessManager(om);
    }
    om->registerWitness(name, semantics);
}

WitnessList VSAExtension::getWitness(Semantics *semantics, const WitnessData &oup, const DataList &inp_list) const {
    for (auto* m: manager_list) {
        if (m->isMatch(semantics)) {
            return m->getWitness(semantics, oup, inp_list);
        }
    }
    LOG(FATAL) << "Witness: unsupported semantics " << semantics->name;
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

