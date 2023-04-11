//
// Created by pro on 2023/4/5.
//

#ifndef ISTOOL_INCRE_COMPONENT_COLLECTOR_H
#define ISTOOL_INCRE_COMPONENT_COLLECTOR_H

#include "istool/basic/grammar.h"
#include "istool/incre/language/incre.h"

namespace incre::grammar {
    class SynthesisComponent {
    public:
        int command_id;
        SynthesisComponent(int command_id);
        virtual void insertComponent(const std::unordered_map<std::string, NonTerminal*>& symbol_map) = 0;
        virtual void extendNTMap(std::unordered_map<std::string, NonTerminal*>& symbol_map) = 0;
        virtual Term tryBuildTerm(const PSemantics& sem, const TermList& term_list) = 0;
        virtual ~SynthesisComponent() = default;
    };
    typedef std::shared_ptr<SynthesisComponent> PSynthesisComponent;
    typedef std::vector<PSynthesisComponent> SynthesisComponentList;

    class IncreComponent: public SynthesisComponent {
    public:
        PType type;
        Data data;
        Term term;
        std::string name;
        IncreComponent(const std::string& _name, const PType& _type, const Data& _data, const Term& _term, int command_id);
        virtual void insertComponent(const std::unordered_map<std::string, NonTerminal*>& symbol_map);
        virtual void extendNTMap(std::unordered_map<std::string, NonTerminal*>& symbol_map);
        virtual Term tryBuildTerm(const PSemantics& sem, const TermList& term_list);
        ~IncreComponent() = default;
    };

    class ConstComponent: public SynthesisComponent {
    public:
        PType type;
        DataList const_list;
        std::function<bool(Value*)> is_inside;
        ConstComponent(const PType& _type, const DataList& _const_list, const std::function<bool(Value*)>& _is_inside);
        virtual void insertComponent(const std::unordered_map<std::string, NonTerminal*>& symbol_map);
        virtual void extendNTMap(std::unordered_map<std::string, NonTerminal*>& symbol_map);
        virtual Term tryBuildTerm(const PSemantics& sem, const TermList& term_list);
        ~ConstComponent() = default;
    };

    class BasicOperatorComponent: public SynthesisComponent {
    public:
        std::string name;
        TypedSemantics* sem;
        PSemantics _sem;
        BasicOperatorComponent(const std::string& _name, const PSemantics& __semantics);
        virtual Term tryBuildTerm(const PSemantics& sem, const TermList& term_list);
        virtual void insertComponent(const std::unordered_map<std::string, NonTerminal*>& symbol_map);
        virtual void extendNTMap(std::unordered_map<std::string, NonTerminal*>& symbol_map);
        ~BasicOperatorComponent() = default;
    };

#define LanguageComponent(name) \
    class name ## Component: public SynthesisComponent { \
    public: \
        name ## Component(); \
        virtual void insertComponent(const std::unordered_map<std::string, NonTerminal*>& symbol_map); \
        virtual void extendNTMap(std::unordered_map<std::string, NonTerminal*>& symbol_map); \
        virtual Term tryBuildTerm(const PSemantics& sem, const TermList& term_list); \
        ~name ## Component() = default;\
    }

    LanguageComponent(Ite);
    LanguageComponent(Tuple);
    LanguageComponent(Proj);

    class ApplyComponent: public SynthesisComponent {
    public:
        bool is_only_full;
        Context* ctx;
        ApplyComponent(Context* ctx, bool _is_only_full);
        virtual Term tryBuildTerm(const PSemantics& sem, const TermList& term_list);
        virtual void extendNTMap(std::unordered_map<std::string, NonTerminal*>& symbol_map);
        virtual void insertComponent(const std::unordered_map<std::string, NonTerminal*>& symbol_map);
        ~ApplyComponent() = default;
    };

    enum class GrammarType {
        ALIGN, COMPRESS, COMB
    };

    struct TypeLabeledDirectSemantics: public NormalSemantics {
    public:
        PType type;
        TypeLabeledDirectSemantics(const PType& _type);
        virtual Data run(DataList&& inp_list, ExecuteInfo* info);
        virtual ~TypeLabeledDirectSemantics() = default;
    };

    class ComponentPool {
    public:
        SynthesisComponentList align_list, compress_list, comb_list;
        ComponentPool(const SynthesisComponentList& _align_list, const SynthesisComponentList& _compress_list, const SynthesisComponentList& _comb_list);
        ComponentPool();
        void print() const;

        Grammar* buildAlignGrammar(const TypeList& inp_list);
        Grammar* buildCompressGrammar(const TypeList& inp_list, int command_id);
        Grammar* buildCombinatorGrammar(const TypeList& inp_list, const PType& oup_type, int command_id);
        ~ComponentPool() = default;
    };

    enum ComponentCollectorType {
        SOURCE = 0, LABEL = 1
    };

    namespace collector {
        ComponentPool collectComponentFromSource(Context* ctx, Env* env, ProgramData* program);
        ComponentPool collectComponentFromLabel(Context* ctx, Env* env, ProgramData* program);
        ComponentPool getBasicComponentPool(Context* ctx, Env* env, bool is_full_apply);
        extern const std::string KCollectMethodName;
    }
    ComponentPool collectComponent(Context* ctx, Env* env, ProgramData* program);
}

#endif //ISTOOL_INCRE_COMPONENT_COLLECTOR_H
