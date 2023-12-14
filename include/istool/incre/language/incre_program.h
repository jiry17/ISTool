//
// Created by pro on 2023/12/8.
//

#ifndef ISTOOL_INCRE_H
#define ISTOOL_INCRE_H

#include "incre_semantics.h"
#include "incre_rewriter.h"
#include "incre_types.h"

namespace incre {
    enum class CommandType {
        BIND, DEF_IND, INPUT
    };

    enum class CommandDecorate {
        INPUT, START, SYN_EXTRACT, SYN_COMBINE, SYN_COMPRESS, SYN_NO_PARTIAL, TERM_NUM
    };

    typedef std::unordered_set<CommandDecorate> DecorateSet;

    class CommandData {
    public:
        std::string name;
        CommandType type;
        DecorateSet decos;
        CommandData(const CommandType& _type, const std::string& _name, const DecorateSet& decos);
        bool isDecrorateWith(CommandDecorate deco) const;
        CommandType getType() const;
        virtual ~CommandData() = default;
    };
    typedef std::shared_ptr<CommandData> Command;
    typedef std::vector<Command> CommandList;

    class CommandBind: public CommandData {
    public:
        syntax::Term term;
        CommandBind(const std::string& _name, const syntax::Term& _term, const DecorateSet& decos);
    };

    typedef std::vector<std::pair<std::string, syntax::Ty>> IndConstructorInfo;
    class CommandDef: public CommandData {
    public:
        int param;
        IndConstructorInfo cons_list;
        CommandDef(const std::string& _name, int _param, const IndConstructorInfo& _cons_list, const DecorateSet& decos);
    };

    class CommandInput: public CommandData {
    public:
        syntax::Ty type;
        CommandInput(const std::string& _name, const syntax::Ty& _type, const DecorateSet& decos);
    };

    enum class IncreConfig {
        COMPOSE_NUM, /*Max components in align, default 3*/
        VERIFY_BASE, /*Base number of examples in verification, default 1000*/
        SAMPLE_SIZE, /*Max size of random data structures, default 10*/
        SAMPLE_INT_MAX, /*Int Max of Sample, Default 5*/
        SAMPLE_INT_MIN, /*Int Min of Sample, Default -5*/
        NON_LINEAR, /*Whether consider * in synthesis, default false*/
        ENABLE_FOLD, /*Whether consider `fold` operator on data structures in synthesis, default false*/
        TERM_NUM, /* Number of terms considered by PolyGen*/
        CLAUSE_NUM, /* Number of terms considered by PolyGen*/
        PRINT_ALIGN, /*Whether print align results to the result*/
        THREAD_NUM /*The number of threads available for collecting examples*/
    };

    typedef std::unordered_map<IncreConfig, Data> IncreConfigMap;

    class IncreProgramData {
    public:
        IncreConfigMap config_map;
        CommandList commands;
        IncreProgramData(const CommandList& _commands, const IncreConfigMap& _config_map);
    };
    typedef std::shared_ptr<IncreProgramData> IncreProgram;

    namespace config {
        extern const std::string KComposeNumName;
        extern const std::string KMaxTermNumName;
        extern const std::string KMaxClauseNumName;
        extern const std::string KIsNonLinearName;
        extern const std::string KVerifyBaseName;
        extern const std::string KDataSizeLimitName;
        extern const std::string KIsEnableFoldName;
        extern const std::string KSampleIntMinName;
        extern const std::string KSampleIntMaxName;
        extern const std::string KPrintAlignName;
        extern const std::string KThreadNumName;

        IncreConfigMap buildDefaultConfigMap();
        void applyConfig(IncreProgramData* program, Env* env);
    }

    class IncreProgramWalker {
    protected:
        virtual void visit(CommandDef* command) = 0;
        virtual void visit(CommandBind* command) = 0;
        virtual void visit(CommandInput* command) = 0;
        virtual void preProcess(IncreProgramData* program) {}
        virtual void postProcess(IncreProgramData* program) {}
    public:
        void walkThrough(IncreProgramData* program);
        virtual ~IncreProgramWalker() = default;
    };

    IncreFullContext buildContext(IncreProgramData* program, semantics::IncreEvaluator* evaluator, types::IncreTypeChecker* checker, syntax::IncreTypeRewriter* rewriter);

#define BuildGen(name) []{return new name();}
    IncreFullContext buildContext(IncreProgramData* program,
                              const semantics::IncreEvaluatorGenerator& = [](){return nullptr;},
                              const types::IncreTypeCheckerGenerator& = [](){return nullptr;},
                              const syntax::IncreTypeRewriterGenerator& = [](){return nullptr;});
}

namespace incre::syntax {
    class IncreProgramRewriter: public IncreProgramWalker {
    protected:
        IncreTypeRewriter* type_rewriter;
        IncreTermRewriter* term_rewriter;
        virtual void visit(CommandDef* command);
        virtual void visit(CommandBind* command);
        virtual void visit(CommandInput* command);
        virtual void preProcess(IncreProgramData* program);
    public:
        IncreProgram res;
        IncreProgramRewriter(IncreTypeRewriter* _type_rewriter, IncreTermRewriter* _term_rewriter);
    };

    IncreProgram rewriteProgram(IncreProgramData* program, const IncreTypeRewriterGenerator& type_gen, const IncreTermRewriterGenerator& term_gen);
}

#endif //ISTOOL_INCRE_H
