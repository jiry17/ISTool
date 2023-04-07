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
        int priority;
        SynthesisComponent(int command_id, int priority);
        virtual void insertComponent(std::unordered_map<std::string, NonTerminal*>& symbol_map) = 0;
        virtual Term buildTerm(const PSemantics& sem, const TermList& term_list) = 0;
        virtual ~SynthesisComponent() = default;
    };
    typedef std::shared_ptr<SynthesisComponent> PSynthesisComponent;
    typedef std::vector<PSynthesisComponent> SynthesisComponentList;

    class ComponentSemantics: public ConstSemantics {
    public:
        ComponentSemantics(const Data& _data, const std::string& _name);
        virtual ~ComponentSemantics() = default;
    };

    class UserProvidedComponent: public SynthesisComponent {
    public:
        TypeList inp_types;
        PType oup_type;
        PSemantics semantics;
        UserProvidedComponent(const TypeList& _inp_types, const PType& _oup_type, const PSemantics& _semantics, int command_id);
        virtual Term buildTerm(const PSemantics& sem, const TermList& term_list);
        virtual void insertComponent(std::unordered_map<std::string, NonTerminal*>& symbol_map);
        ~UserProvidedComponent() = default;
    };

#define LanguageComponent(name) \
    class name ## Component: public SynthesisComponent { \
    public: \
        name ## Component(int _priority); \
        virtual Term buildTerm(const PSemantics& sem, const TermList& term_list); \
        virtual void insertComponent(std::unordered_map<std::string, NonTerminal*>& symbol_map); \
        ~name ## Component() = default;\
    }

    LanguageComponent(Ite);
    LanguageComponent(Tuple);
    LanguageComponent(Proj);

    class ApplyComponent: public SynthesisComponent {
    public:
        bool is_only_full;
        Context* ctx;
        ApplyComponent(int _priority, Context* ctx, bool _is_only_full);
        virtual Term buildTerm(const PSemantics& sem, const TermList& term_list);
        virtual void insertComponent(std::unordered_map<std::string, NonTerminal*>& symbol_map);
        ~ApplyComponent() = default;
    };

    enum class GrammarType {
        ALIGN, COMPRESS, COMB
    };

    class ComponentPool {
    public:
        SynthesisComponentList align_list, compress_list, comb_list;
        ComponentPool(const SynthesisComponentList& _align_list, const SynthesisComponentList& _compress_list, const SynthesisComponentList& _comb_list);
        // TODO: add a grammar builder function
        ~ComponentPool() = default;
    };

    enum class ComponentCollectorType {
        DEFAULT, LABEL
    };

    class ComponentCollector {
    public:
        ComponentCollectorType type;
        ComponentCollector(ComponentCollectorType _type);
        virtual ComponentPool collect(ProgramData* program) = 0;
        virtual ~ComponentCollector() = default;
    };
}

#endif //ISTOOL_INCRE_COMPONENT_COLLECTOR_H
