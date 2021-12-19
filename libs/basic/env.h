//
// Created by pro on 2021/12/18.
//

#ifndef ISTOOL_ENV_H
#define ISTOOL_ENV_H

#include "semantics.h"
#include <unordered_map>

class Extension {
public:
    virtual ~Extension() = default;
};

class Env {
    std::unordered_map<std::string, Data*> const_pool;
    std::unordered_map<std::string, Extension*> extension_pool;
    std::unordered_map<std::string, PSemantics> semantics_pool;
public:
    Data* getConstRef(const std::string& name);
    void setConst(const std::string& name, const Data& value);

    void registerExtension(const std::string& name, Extension* ext);
    Extension* getExtension(const std::string& name) const;
    ~Env();
};


#endif //ISTOOL_ENV_H
