//
// Created by pro on 2022/9/16.
//

#ifndef ISTOOL_INCRE_CONTEXT_H
#define ISTOOL_INCRE_CONTEXT_H

#include "incre_term.h"
#include "incre_type.h"
#include <unordered_map>

namespace incre {
    enum class BindingType {
        TYPE, TERM
    };

    class BindingData {
    public:
        BindingType type;
        BindingData(const BindingType& _type);
        virtual std::string toString() const = 0;
        BindingType getType() const;
        virtual ~BindingData() = default;
    };

    typedef std::shared_ptr<BindingData> Binding;

    class TypeBinding: public BindingData {
    public:
        Ty type;
        TypeBinding(const Ty& _type);
        virtual std::string toString() const;
        virtual ~TypeBinding() = default;
    };

    class TermBinding: public BindingData {
    public:
        Term term;
        TermBinding(const Term& _term);
        virtual std::string toString() const;
        virtual ~TermBinding() = default;
    };

    class Context {
        std::unordered_map<std::string, Binding> binding_map;
    public:
        void addBinding(const std::string& name, const Ty& type);
        void addBinding(const std::string& name, const Term& term);
        Term getTerm(const std::string& name);
        Ty getType(const std::string& name);
    };
}

#endif //ISTOOL_INCRE_CONTEXT_H
