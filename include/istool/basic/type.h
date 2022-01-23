//
// Created by pro on 2021/12/3.
//

#ifndef ISTOOL_TYPE_H
#define ISTOOL_TYPE_H

#include <memory>
#include <vector>
#include "z3++.h"

class Type;
typedef std::shared_ptr<Type> PType;
typedef std::vector<PType> TypeList;

class Type {
public:
    virtual std::string getName() = 0;
    virtual bool equal(Type* type) = 0;
    virtual ~Type() = default;
};

class TBot: public Type {
public:
    virtual std::string getName();
    virtual bool equal(Type* type);
    virtual ~TBot() = default;
};

class TVar: public Type {
public:
    std::string name;
    TVar(const std::string& _name);
    virtual std::string getName();
    virtual bool equal(Type* type);
    virtual ~TVar() = default;
};

class TBool: public Type {
public:
    virtual std::string getName();
    virtual bool equal(Type* type);
};

typedef std::pair<TypeList, PType> Signature;

namespace type {
    std::string typeList2String(const TypeList& type_list);
    bool equal(const PType& t1, const PType& t2);
    PType getTBool();
    PType getTVarA();
}

#endif //ISTOOL_TYPE_H
