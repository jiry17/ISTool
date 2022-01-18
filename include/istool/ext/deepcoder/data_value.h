//
// Created by pro on 2022/1/15.
//

#ifndef ISTOOL_DATA_VALUE_H
#define ISTOOL_DATA_VALUE_H

#include "istool/basic/data.h"

class ProductValue: public Value {
public:
    DataList elements;
    ProductValue(const DataList& _elements);
    Data get(int id) const;
    virtual ~ProductValue() = default;
    virtual std::string toString() const;
    virtual bool equal(Value* v) const;
};

class SumValue: public Value {
public:
    int id;
    Data value;
    SumValue(int _id, const Data& _value);
    virtual ~SumValue() = default;
    virtual std::string toString() const;
    virtual bool equal(Value* v) const;
};

class ListValue: public Value {
public:
    DataList value;
    ListValue(const DataList& _value);
    virtual ~ListValue() = default;
    virtual std::string toString() const;
    virtual bool equal(Value* v) const;
};

class BTreeValue: public Value {
public:
    virtual ~BTreeValue() = default;
};
typedef std::shared_ptr<BTreeValue> BTreeNode;

class BTreeInternalValue: public BTreeValue {
public:
    BTreeNode l, r;
    Data value;
    BTreeInternalValue(const BTreeNode& _l, const BTreeNode& _r, const Data& _v);
    virtual ~BTreeInternalValue() = default;
    virtual std::string toString() const;
    virtual bool equal(Value* v) const;
};

class BTreeLeafValue: public BTreeValue {
public:
    Data value;
    BTreeLeafValue(const Data& _v);
    virtual ~BTreeLeafValue() = default;
    virtual std::string toString() const;
    virtual bool equal(Value* v) const;
};

namespace ext::ho {
    Data buildProduct(const DataList& elements);
    Data buildSum(int id, const Data& value);
    Data buildList(const DataList& content);
}

#endif //ISTOOL_DATA_VALUE_H
