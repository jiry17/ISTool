//
// Created by pro on 2022/9/17.
//

#ifndef ISTOOL_INCRE_VALUES_H
#define ISTOOL_INCRE_VALUES_H

#include "istool/basic/value.h"
#include "incre_type.h"
#include "incre_context.h"
#include "istool/sygus/theory/basic/clia/clia_value.h"
#include "istool/ext/deepcoder/data_value.h"

namespace incre {
    typedef BoolValue VBool;
    typedef IntValue VInt;
    typedef ProductValue VTuple;

    class VUnit: public Value {
    public:
        virtual std::string toString() const;
        virtual bool equal(Value* value) const;
        virtual ~VUnit() = default;
    };

    class VInductive: public Value {
    public:
        std::string name;
        Data content;
        VInductive(const std::string& _name, const Data& _content);
        virtual bool equal(Value* value) const;
        virtual std::string toString() const;
        virtual ~VInductive() = default;
    };

    class VCompress: public Value {
    public:
        Data content;
        VCompress(const Data& _content);
        virtual bool equal(Value* value) const;
        virtual std::string toString() const;
        virtual ~VCompress() = default;
    };

    typedef std::function<Data(const Term&)> Function;

    class VFunction: public Value {
    public:
        Function func;
        VFunction(const Function& _func);
        virtual bool equal(Value* value) const;
        virtual std::string toString() const;
        virtual ~VFunction() = default;
    };

    class VNamedFunction: public VFunction {
    public:
        std::string name;
        VNamedFunction(const Function& _func, const std::string& _name);
        virtual bool equal(Value* value) const;
        virtual std::string toString() const;
        virtual ~VNamedFunction() = default;
    };

    class VTyped {
    public:
        Ty type;
        VTyped(const Ty& _type);
        virtual ~VTyped() = default;
    };

    class VBasicOperator: public VNamedFunction, public VTyped {
    public:
        std::string name;
        VBasicOperator(const Function& _func, const std::string& _name, const Ty& _type);
        virtual ~VBasicOperator() = default;
    };

    Ty getValueType(Value* value);
}

#endif //ISTOOL_INCRE_VALUES_H
