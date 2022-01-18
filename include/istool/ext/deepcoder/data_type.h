//
// Created by pro on 2022/1/15.
//

#ifndef ISTOOL_DATA_TYPE_H
#define ISTOOL_DATA_TYPE_H

#include "istool/basic/type.h"

class SumType: public Type {
public:
    TypeList sub_types;
    SumType(const TypeList& sub_types);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~SumType() = default;
};

class ProductType: public Type {
public:
    TypeList sub_types;
    ProductType(const TypeList& sub_types);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~ProductType() = default;
};

class ArrowType: public Type {
public:
    TypeList inp_types;
    PType oup_type;
    ArrowType(const TypeList& _inp_types, const PType& _oup_type);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~ArrowType() = default;
};

class ListType: public Type {
public:
    PType content;
    ListType(const PType& _content);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~ListType() = default;
};

class BTreeType: public Type {
public:
    PType content, leaf;
    BTreeType(const PType& _content, const PType& _leaf);
    virtual std::string getName();
    virtual bool equal(Type* type);
    ~BTreeType() = default;
};

namespace ext {
    namespace ho {
        PType getTIntList();
        PType getTVarAList();
    }
}

#endif //ISTOOL_DATA_TYPE_H
