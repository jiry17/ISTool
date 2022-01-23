//
// Created by pro on 2022/1/15.
//

#ifndef ISTOOL_DATA_TYPE_H
#define ISTOOL_DATA_TYPE_H

#include "istool/basic/type.h"

class TSum: public Type {
public:
    TypeList sub_types;
    TSum(const TypeList& sub_types);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~TSum() = default;
};

class TProduct: public Type {
public:
    TypeList sub_types;
    TProduct(const TypeList& sub_types);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~TProduct() = default;
};

class TArrow: public Type {
public:
    TypeList inp_types;
    PType oup_type;
    TArrow(const TypeList& _inp_types, const PType& _oup_type);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~TArrow() = default;
};

class TList: public Type {
public:
    PType content;
    TList(const PType& _content);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~TList() = default;
};

class TBTree: public Type {
public:
    PType content, leaf;
    TBTree(const PType& _content, const PType& _leaf);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~TBTree() = default;
};

namespace ext::ho {
    PType getTIntList();
    PType getTVarAList();
}

#endif //ISTOOL_DATA_TYPE_H
