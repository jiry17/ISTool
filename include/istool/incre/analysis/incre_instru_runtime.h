//
// Created by pro on 2022/9/23.
//

#ifndef ISTOOL_INCRE_INSTRU_RUNTIME_H
#define ISTOOL_INCRE_INSTRU_RUNTIME_H

#include "istool/incre/language/incre_program.h"
#include "istool/basic/example_sampler.h"
#include "incre_instru_types.h"
#include <random>
#include <unordered_set>

namespace incre::example {
    struct IncreExampleData {
        int rewrite_id;
        DataList local_inputs, global_inputs;
        Data oup;
        IncreExampleData(int _rewrite_id, const DataList& _local, const DataList& _global, const Data& _oup);
        std::string toString() const;
        virtual ~IncreExampleData() = default;
    };

    syntax::Ty getContentTypeForGen(syntax::TypeData* res, syntax::TypeData* cons_ty);

    typedef std::shared_ptr<IncreExampleData> IncreExample;
    typedef std::vector<IncreExample> IncreExampleList;

    // store IO example of each hole for DP synthesis
    class DpExampleData {
    public:
        Data inp;
        Data oup;
        DpExampleData(const Data& _inp, const Data& _oup) : inp(_inp), oup(_oup) {}
        std::string toString() const;
        ~DpExampleData() = default;
    };

    typedef std::shared_ptr<DpExampleData> DpExample;
    typedef std::vector<DpExample> DpExampleList;

    class IncreDataGenerator {
    public:
        Env* env;
        int KSizeLimit, KIntMin, KIntMax;
        std::unordered_map<std::string, CommandDef*> cons_map;
        Data getRandomInt();
        Data getRandomBool();
        IncreDataGenerator(Env* _env, const std::unordered_map<std::string, CommandDef*>& _cons_map);
        virtual Data getRandomData(const syntax::Ty& type) = 0;
        virtual ~IncreDataGenerator() = default;
    };

    std::unordered_map<std::string, CommandDef*> extractConsMap(IncreProgramData* program);

    typedef std::variant<std::pair<std::string, syntax::Ty>, std::vector<int>> SizeSplitScheme;
    typedef std::vector<SizeSplitScheme> SizeSplitList;

    class SizeSafeValueGenerator: public IncreDataGenerator {
    public:
        std::unordered_map<std::string, SizeSplitList*> split_map;
        SizeSplitList* getPossibleSplit(syntax::TypeData* type, int size);
        SizeSafeValueGenerator(Env* _env, const std::unordered_map<std::string, CommandDef*>& _ind_cons_map);
        virtual Data getRandomData(const syntax::Ty& type);
        virtual ~SizeSafeValueGenerator();
    };

    class IncreExampleCollector;

    class IncreExampleCollectionEvaluator: public incre::semantics::IncreLabeledEvaluator {
    protected:
        RegisterEvaluateCase(Rewrite);
    public:
        IncreExampleCollector* collector;
        IncreExampleCollectionEvaluator(IncreExampleCollector* _collector);
    };

    class DpExampleCollectionEvaluator: public incre::semantics::DefaultEvaluator {
    protected:
        // RegisterEvaluateCase(Value);
        // RegisterEvaluateCase(App);
        TERM_CASE_ANALYSIS(RegisterEvaluateCase);
    public:
        IncreExampleCollector* collector;
        DpExampleCollectionEvaluator(IncreExampleCollector* _collector);
    };

    class IncreExampleCollector {
    public:
        std::vector<IncreExampleList> example_pool;
        DpExampleList dp_example_pool;
        std::vector<std::vector<std::string>> cared_vars;
        std::vector<std::string> global_name;
        DataList current_global;
        IncreFullContext ctx;
        IncreExampleCollectionEvaluator* eval;
        DpExampleCollectionEvaluator* dp_eval;
        std::unordered_map<std::string, EnvAddress*> global_address_map;

        IncreExampleCollector(IncreProgramData* program, const std::vector<std::vector<std::string>>& cared_vars,
                              const std::vector<std::string>& _global_name);
        void add(int rewrite_id, const DataList& local_inp, const Data& oup);
        void add_dp_example(const Data& inp, const Data& oup);
        virtual void collect(const syntax::Term& start, const DataList& global);
        virtual void collectDp(const syntax::Term& start, const DataList& global);
        void clear();
        virtual ~IncreExampleCollector();
    };

    class IncreExamplePool {
    private:
        // program processed in constructor of IncreInfo
        IncreProgram program;
        // vars in env for each hole
        std::vector<std::vector<std::string>> cared_vars;
        IncreDataGenerator* generator;
        std::vector<bool> is_finished;
        int thread_num;

        // first: name of the start symbol (e.g. main in 01knapsack)
        // second: type of inputs for the start symbol
        std::vector<std::pair<std::string, syntax::TyList>> start_list;
        // existing examples for each hole, each example stored as string which looks like the result of IncreExampleData.toString()
        std::vector<std::unordered_set<std::string>> existing_example_set;
        std::unordered_set<std::string> existing_dp_example_set;
    public:
        // name of global_input?
        std::vector<std::string> global_name_list;
        // type of global_input?
        syntax::TyList global_type_list;
        // example for each hole where IncreExampleList contains a lot of examples for each hole
        std::vector<IncreExampleList> example_pool;
        // example pool for DP synthesis
        DpExampleList dp_example_pool;

        IncreExamplePool(const IncreProgram& _program, const std::vector<std::vector<std::string>>& _cared_vars, IncreDataGenerator* _g);
        ~IncreExamplePool();

        // merge example in collector into example_pool
        void merge(int rewrite_id, IncreExampleCollector* collector, TimeGuard* guard);
        // merge example for DP synthesis
        void mergeDp(int rewrite_id, IncreExampleCollector* collector, TimeGuard* guard);
        // generate example for the whole program, Term = call for the "start" command(function) , DataList = global_input
        std::pair<syntax::Term, DataList> generateStart();
        // generate example for the program and get corresponding I/O examples for each hole
        void generateSingleExample();
        // generate example for the program and get corresponding I/O examples for each hole, used in DP synthesis
        void generateDpSingleExample();
        void generateBatchedExample(int rewrite_id, int target_num, TimeGuard* guard);
        // print cared_vars
        void printCaredVars();
        // print start_list
        void printStartList();
        // print existing_example_set
        void printExistingExampleSet();
        // print global_name_list
        void printGlobalNameList();
        // print global_type_list
        void printGlobalTypeList();
    };
}

#endif //ISTOOL_INCRE_INSTRU_RUNTIME_H
