//
// Created by pro on 2022/9/23.
//

#ifndef ISTOOL_INCRE_INSTRU_RUNTIME_H
#define ISTOOL_INCRE_INSTRU_RUNTIME_H

#include "incre_instru_type.h"

namespace incre {
    struct IncreExampleData {
        int tau_id;
        std::unordered_map<std::string, Term> inputs;
        Data oup;
        IncreExampleData(int _tau_id, const std::unordered_map<std::string, Term>& _inputs, const Data& _oup);
        virtual ~IncreExampleData() = default;
    };

    typedef std::shared_ptr<IncreExampleData> IncreExample;
    typedef std::vector<IncreExample> IncreExampleList;

    struct IncreExamplePool {
        std::vector<IncreExampleList> example_pool;
        void add(const IncreExample& example);
    };

    Data collectExample(const Term& term, Context* context, IncreExamplePool* pool);
}

#endif //ISTOOL_INCRE_INSTRU_RUNTIME_H
