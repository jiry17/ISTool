//
// Created by pro on 2023/7/14.
//

#ifndef ISTOOL_INCRE_EXTENDED_AUTOLIFTER_H
#define ISTOOL_INCRE_EXTENDED_AUTOLIFTER_H

#include "incre_plp.h"
#include "istool/basic/specification.h"
#include "istool/incre/incre_solver.h"

namespace incre::autolifter {
    struct FullPairExample {
    public:
        int align_id, x, y;
    };

    struct InputUnfoldInfo {
        std::unordered_set<std::string> ds_input;
        DataList scalar_input;
        std::string structure_feature;
        InputUnfoldInfo() = default;
        InputUnfoldInfo(const std::unordered_set<std::string>& _ds_input, const DataList& _scalar_input, const std::string& _feature);
    };

    typedef std::unordered_map<std::string, Data> OutputUnfoldInfo;

    class NonScalarExecutionTool {
    public:
        IncreInfo* info;
        IncreExamplePool* pool;
        Env* env;
        int KUnfoldDepth;

        NonScalarExecutionTool(IncreInfo* _info, Env* _env, int _KUnfoldDepth);
        virtual InputUnfoldInfo runInp(int align_id, int example_id, const TypedProgramList& program) = 0;
        virtual bool runOup(int align_id, int example_id, const TypedProgramList& program, const InputUnfoldInfo& inp_info, OutputUnfoldInfo& result) = 0;
        bool isValid(FullPairExample& example, const TypedProgramList& program);
        virtual ~NonScalarExecutionTool() = default;
    };

    class BasicNonScalarExecutionTool: public NonScalarExecutionTool {
    public:
        std::vector<std::vector<std::string>> local_inp_names;
        BasicNonScalarExecutionTool(IncreInfo* _info,  Env* _env, int _KUnfoldDepth);
        void prepareGlobal(int align_id, int example_id);
        virtual InputUnfoldInfo runInp(int align_id, int example_id, const TypedProgramList& program);
        virtual bool runOup(int align_id, int example_id, const TypedProgramList& program, const InputUnfoldInfo& inp_info, OutputUnfoldInfo& result);

        virtual ~BasicNonScalarExecutionTool() = default;
    };

    class BasicNonScalarSolver {
    public:
        IncreInfo* info;
        NonScalarExecutionTool* runner;
        std::vector<FullPairExample> example_list;
        FullPairExample verify(const TypedProgramList & res);
        void acquireExamples(int target_num);

        int KVerifyBase, KExampleTimeOut;
        // TypeList ctype_list;
        std::vector<PSynthInfo> cinfo_list;

        TypedProgramList synthesisFromExample();
        BasicNonScalarSolver(IncreInfo* _info, NonScalarExecutionTool* _runner);
        TypedProgramList solve();
    };

    class IncreNonScalarSolver: public IncreSolver {
    public:
        PEnv env;
        int KUnfoldDepth, KExampleTimeOut;
        TypedProgramList align_list;
        NonScalarExecutionTool* runner;
        IncreNonScalarSolver(IncreInfo* _info, const PEnv& _env);
        Ty getFinalType(const Ty& type);
        virtual IncreSolution solve();
        virtual ~IncreNonScalarSolver() = default;
        Term synthesisComb(int align_id);
    };


    InputUnfoldInfo unfoldInput(const Data& data, int depth);
    void mergeInputInfo(InputUnfoldInfo& base, const InputUnfoldInfo& extra);
    bool unfoldOutput(const Data& data, const InputUnfoldInfo& info, int depth_limit, OutputUnfoldInfo& result);
    Data executeCompress(const Data& data, const TypedProgramList& program_list, Env* env);
    extern const std::string KUnfoldDepthName;

    namespace comb {
        PatternList getDSScheme(const IOExampleList& example_list, int depth_limit, Env* env);
        bool isMatchUsage(const Pattern& pattern, const Data& oup, const DataList& inp);
        Term usage2Term(const Pattern& pattern, const TermList& term, const Ty& type);
    }
}

#endif //ISTOOL_INCRE_EXTENDED_AUTOLIFTER_H
