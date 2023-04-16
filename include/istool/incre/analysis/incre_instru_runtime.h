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
        std::unordered_map<std::string, Data> local_inputs, global_inputs;
        Data oup;
        IncreExampleData(int _tau_id, const std::unordered_map<std::string, Data>& _local, const std::unordered_map<std::string, Data>& _global, const Data& _oup);
        std::string toString() const;
        virtual ~IncreExampleData() = default;
    };

    typedef std::shared_ptr<IncreExampleData> IncreExample;
    typedef std::vector<IncreExample> IncreExampleList;

    class IncreDataGenerator {
    public:
        Env* env;
        IncreDataGenerator(Env* _env);
        virtual Data getRandomData(const Ty& type) = 0;
        virtual ~IncreDataGenerator() = default;
    };

    class SizeSafeValueGenerator: public IncreDataGenerator {
    public:

        struct SplitScheme {
            std::string cons_name;
            Ty cons_type;
            std::vector<int> size_list;
            SplitScheme(const std::string& _cons_name, const Ty& _cons_type, const std::vector<int>& _size_list);
            std::string toString() const;
        };
        typedef std::vector<SplitScheme> SplitList;

        std::unordered_map<std::string, SplitList*> split_map;
        std::unordered_map<std::string, Ty> ind_map;
        SplitList* getPossibleSplit(const Ty& type, int size);
        SizeSafeValueGenerator(Env* _env);
        virtual Data getRandomData(const Ty& type);
        virtual ~SizeSafeValueGenerator();
    };

    class FirstOrderFunctionGenerator: public SizeSafeValueGenerator {
    public:
        virtual Data getRandomData(const Ty& type);
        FirstOrderFunctionGenerator(Env* _env);
        virtual ~FirstOrderFunctionGenerator() = default;
    };

    class FixedPoolFunctionGenerator: public SizeSafeValueGenerator {
    public:
        virtual Data getRandomData(const Ty& type);
        FixedPoolFunctionGenerator(Env* _env);
        virtual ~FixedPoolFunctionGenerator() = default;
    };

    class IncreExampleCollector {
    public:
        std::vector<IncreExampleList> example_pool;
        std::vector<std::unordered_set<std::string>> cared_vars;
        Context* ctx;
        std::unordered_map<std::string, Data> current_global;
        IncreExampleCollector(const std::vector<std::unordered_set<std::string>>& _cared_vars, ProgramData* _program);
        void add(int tau_id, const std::unordered_map<std::string, Data>& local, const Data& oup);
        void collect(const Term& start, const std::unordered_map<std::string, Data>& _global);
        ~IncreExampleCollector();
    };

    class IncreExamplePool {
    public:
        std::vector<IncreExampleList> example_pool;
        std::vector<std::unordered_set<std::string>> cared_vars;
        IncreDataGenerator* generator;
        int thread_num;
        Context* ctx;
        IncreProgram program;

        std::vector<std::pair<std::string, Ty>> input_list;
        std::vector<std::pair<std::string, TyList>> start_list;
        std::uniform_int_distribution<int> start_dist;

        void merge(IncreExampleCollector* collector);
        std::pair<Term, std::unordered_map<std::string, Data>> generateStart();
        IncreExamplePool(const IncreProgram& _program, Env* env, const std::vector<std::unordered_set<std::string>>& _cared_vars);
        void generateSingleExample();
        void generateBatchedExample(int tau_id, int target_num, TimeGuard* guard);
        ~IncreExamplePool();
    };

    extern const std::string KExampleThreadName;
}

#endif //ISTOOL_INCRE_INSTRU_RUNTIME_H
