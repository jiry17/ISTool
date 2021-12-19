//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_TYPE_SYSTEM_H
#define ISTOOL_TYPE_SYSTEM_H

#include "type.h"
#include "program.h"
#include "grammar.h"
#include <functional>

class TypeSystem {
public:
    virtual PType typeCheck(const PProgram& program) const = 0;
    virtual PType getType(const PProgram& program) const = 0;
    virtual ~TypeSystem() = default;
};

class DummyTypeSystem: public TypeSystem {
public:
    virtual PType typeCheck(const PProgram& program) const;
    virtual PType getType(const PProgram& program) const;
    ~DummyTypeSystem() = default;
};

// TODO: extend to trivial polymorphic
class BasicTypeSystem: public TypeSystem {
public:
    virtual PType typeCheck(const PProgram& program) const;
    virtual PType getType(const PProgram& program) const;
    ~BasicTypeSystem() = default;
};
#endif //ISTOOL_TYPE_SYSTEM_H
