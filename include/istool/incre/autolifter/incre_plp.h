//
// Created by pro on 2022/9/27.
//

#ifndef ISTOOL_INCRE_PLP_H
#define ISTOOL_INCRE_PLP_H

#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/analysis/incre_instru_info.h"

namespace incre::autolifter {
    // TODO: Current implmenetation supports only tuple of scalar values
    class PLPInputGenerator {
        DataList getPartial(int id) const;
        void extendPartial(int id);
    public:
        int compress_id;
        IncreExampleList* examples;
        DataStorage partial_examples;
        std::vector<std::string> names;
        std::unordered_map<std::string, int> program_map;
        std::vector<DataList*> cache_list;
        Env* env;

        PLPInputGenerator(Env* _env, IncreExampleList* _examples, const std::vector<std::string>& _names, int _compress_id);
        ~PLPInputGenerator();
        void registerProgram(Program* program, DataList* cache);
        Data getInput(Program* program, int id);
        DataList* getCache(Program* program, int len);
    };
    typedef std::shared_ptr<PLPInputGenerator> InputGenerator;

    class PLPConstGenerator {
    public:
        IncreExampleList* examples;
        DataList cache;
        std::vector<std::string> const_names;
        DataList* getCache(int len);
        PLPConstGenerator(IncreExampleList* _examples, const std::vector<std::string>& _const_names);
    };
    typedef std::shared_ptr<PLPConstGenerator> ConstGenerator;

    class PLPOutputGenerator {
        std::vector<int> trace;
        Data getPartial(int id) const;
        void extendPartial(int id);
    public:
        IncreExampleList* examples;
        DataList partial_outputs;
        std::unordered_map<std::string, int> program_map;
        std::vector<DataList*> cache_list;
        Env* env;
        int compress_id;

        PLPOutputGenerator(Env* _env, IncreExampleList* _examples, const std::vector<int>& _trace, int _compress_id);
        ~PLPOutputGenerator();
        DataList* getCache(Program*, int len);
    };
    typedef std::shared_ptr<PLPOutputGenerator> OutputGenerator;

    class PLPInfo {
    public:
        std::vector<InputGenerator> inps;
        std::vector<Grammar*> f_grammars;
        OutputGenerator o;
        ConstGenerator c;
        IncreExamplePool* pool;
        PLPInfo(const std::vector<InputGenerator>& _inps, const std::vector<Grammar*>& _grammars, const OutputGenerator& _o,
                const ConstGenerator& _c, IncreExamplePool* _pool);
        virtual ~PLPInfo() = default;
    };
}

#endif //ISTOOL_INCRE_PLP_H
