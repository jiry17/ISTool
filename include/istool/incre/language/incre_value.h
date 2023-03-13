//
// Created by pro on 2022/9/17.
//

#ifndef ISTOOL_INCRE_VALUES_H
#define ISTOOL_INCRE_VALUES_H

#include "istool/basic/value.h"
#include "incre_type.h"
#include "incre_term.h"
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

    class VFunction: public Value {
    public:
        std::string name;
        VFunction(const std::string& _name);
        virtual Data run(const Term& param, Context* ctx) = 0;
        virtual bool equal(Value* value) const;
        virtual std::string toString() const;
        virtual ~VFunction() = default;
    };

    class VAbsFunction: public VFunction {
    public:
        TmAbs* term;
        Term _term;
        VAbsFunction(const Term& __term);
        virtual Data run(const Term& param, Context* ctx);
    };

    /*class VNamedFunction: public VFunction {
    public:
        std::string name;
        VNamedFunction(const Function& _func, const std::string& _name);
        virtual bool equal(Value* value) const;
        virtual std::string toString() const;
        virtual ~VNamedFunction() = default;
    };*/

    class VOpFunction: public VFunction {
    public:
        Ty type;
        int param_num;
        std::function<Data(const DataList&)> sem;
        VOpFunction(const std::string& _op_name, int _param_num, const std::function<Data(const DataList&)>& _sem, const Ty& _type);
        virtual Data run(const Term& term, Context* ctx);
        virtual ~VOpFunction() = default;
    };

    class VPartialOpFunction: public VFunction {
    public:
        std::string op_name;
        int param_num;
        std::function<Data(const DataList&)> sem;
        DataList param_list;
        VPartialOpFunction(const std::string& _op_name, int _param_num, const std::function<Data(const DataList&)>& _sem, const DataList& _param_list);
        virtual Data run(const Term& term, Context* ctx);
        virtual ~VPartialOpFunction() = default;
    };

    Ty getValueType(Value* value);
}

#endif //ISTOOL_INCRE_VALUES_H
