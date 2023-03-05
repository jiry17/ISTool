//
// Created by pro on 2022/9/23.
//

#ifndef ISTOOL_INCRE_INSTRU_RUNTIME_H
#define ISTOOL_INCRE_INSTRU_RUNTIME_H

#include "incre_instru_type.h"
#include "istool/basic/example_sampler.h"
#include <random>
#include <unordered_set>

namespace incre {
    struct IncreExampleData {
        int tau_id;
        std::unordered_map<std::string, Data> inputs;
        Data oup;
        IncreExampleData(int _tau_id, const std::unordered_map<std::string, Data>& _inputs, const Data& _oup);
        std::string toString() const;
        virtual ~IncreExampleData() = default;
    };

    typedef std::shared_ptr<IncreExampleData> IncreExample;
    typedef std::vector<IncreExample> IncreExampleList;

    class StartTermGenerator {
    public:
        virtual Term getStartTerm() = 0;
        virtual ~StartTermGenerator() = default;
    };

    class DefaultStartTermGenerator: public StartTermGenerator {
    public:
        std::string func_name;
        TyList inp_list;
        std::minstd_rand random_engine;
        DefaultStartTermGenerator(const std::string& _func_name, TyData* type);
        virtual ~DefaultStartTermGenerator() = default;
        virtual Term getStartTerm();
    };

    class IncreExamplePool {
    public:
        std::vector<IncreExampleList> example_pool;
        std::vector<std::unordered_set<std::string>> cared_vars;
        StartTermGenerator* generator;
        Context* ctx;
        void add(const IncreExample& example);
        IncreExamplePool(Context* _ctx, const std::vector<std::unordered_set<std::string>>& _cared_vars, StartTermGenerator* _generator);
        void generateExample();
        ~IncreExamplePool();
    };

    Context* buildCollectContext(const IncreProgram& program, IncreExamplePool* pool);
}

#endif //ISTOOL_INCRE_INSTRU_RUNTIME_H
