//
// Created by pro on 2021/12/18.
//

#include "env.h"

Data * Env::getConstRef(const std::string &name) {
    if (const_pool.find(name) == const_pool.end()) {
        const_pool[name] = new Data();
    }
    return const_pool[name];
}

void Env::setConst(const std::string &name, const Data &value) {
    *(const_pool[name]) = value;
}

Env::~Env() {
    for (auto& const_info: const_pool) {
        delete const_info.second;
    }
    for (auto& ext_info: extension_pool) {
        delete ext_info.second;
    }
}

Extension * Env::getExtension(const std::string &name) const {
    if (extension_pool.find(name) == extension_pool.end()) {
        return nullptr;
    }
    return extension_pool.find(name)->second;
}
void Env::registerExtension(const std::string &name, Extension *ext) {
    extension_pool[name] = ext;
}