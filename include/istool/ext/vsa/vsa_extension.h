//
// Created by pro on 2021/12/29.
//

#ifndef ISTOOL_VSA_EXTENSION_H
#define ISTOOL_VSA_EXTENSION_H

#include "istool/basic/env.h"
#include "witness_manager.h"
#include <list>

class VSAExtension: public Extension {
public:
    std::list<WitnessManager*> manager_list;
    VSAExtension();
    void registerWitnessManager(WitnessManager* manager);
    void registerWitnessFunction(const std::string& name, WitnessFunction* semantics);
    WitnessList getWitness(Semantics* semantics, const WitnessData& oup, const DataList& inp_list) const;
    virtual ~VSAExtension();
};

namespace ext {
    namespace vsa {
        VSAExtension* getExtension(Env* env);
    }
}

#endif //ISTOOL_VSA_EXTENSION_H
