//
// Created by pro on 2022/9/18.
//

#include "istool/incre/language/incre_program.h"
#include "glog/logging.h"
#include <iostream>

using namespace incre;

CommandData::CommandData(CommandType _type, const DecorateSet& _decorate_set):
    type(_type), decorate_set(_decorate_set) {}
CommandType CommandData::getType() const {
    return type;
}

bool CommandData::isDecoratedWith(CommandDecorate deco) const {
    return decorate_set.find(deco) != decorate_set.end();
}

std::string CommandData::toString() {
    std::string res;
    for (auto deco: decorate_set) {
        res += "@" + decorate2String(deco) + " ";
    }
    return res + contentToString();
}

std::string CommandImport::contentToString() const {
    return "import " + name + ";";
}
CommandImport::CommandImport(const std::string &_name, const CommandList &_commands):
    CommandData(CommandType::IMPORT, {}), name(_name), commands(_commands) {
}

CommandBind::CommandBind(const std::string &_name, const Binding &_binding, const DecorateSet& _decorate_set):
    CommandData(CommandType::BIND, _decorate_set), name(_name), binding(_binding) {
}
std::string CommandBind::contentToString() const {
    return name + " = " + binding->toString() + ";";
}

CommandDefInductive::CommandDefInductive(const Ty &__type):
    CommandData(CommandType::DEF_IND, {}), _type(__type) {
    type = dynamic_cast<TyInductive*>(_type.get());
    if (!type) {
        LOG(FATAL) << "Expected TyInductive but get " << _type->toString();
    }
}
std::string CommandDefInductive::contentToString() const {
    auto res = "Inductive " + type->name + " = ";
    for (int i = 0; i < type->constructors.size(); ++i) {
        auto& [name, ty] = type->constructors[i];
        if (i) res += " | ";
        res += name + " " + ty->toString();
    }
    return res + ";";
}

ProgramData::ProgramData(const CommandList &_commands): commands(_commands) {}
void ProgramData::print() const {
    for (auto& command: commands) {
        std::cout << command->toString() << std::endl;
    }
}

namespace {
    const std::unordered_map<CommandDecorate, std::string> KDecorateNameMap = {
            {CommandDecorate::INPUT, "Input"}, {CommandDecorate::START, "Start"},
            {CommandDecorate::SYN_ALIGN, "Align"}, {CommandDecorate::SYN_COMBINE, "Combine"},
            {CommandDecorate::SYN_COMPRESS, "Compress"}
    };
}

CommandDecorate incre::string2Decorate(const std::string &s) {
    for (auto& [deco, name]: KDecorateNameMap) {
        if (name == s) return deco;
    }
    LOG(FATAL) << "Unknown Decorate " << s;
}

std::string incre::decorate2String(CommandDecorate deco) {
    auto it = KDecorateNameMap.find(deco);
    assert(it != KDecorateNameMap.end());
    return it->second;
}