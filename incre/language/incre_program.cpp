//
// Created by pro on 2022/9/18.
//

#include "istool/incre/language/incre_program.h"
#include "glog/logging.h"

using namespace incre;

CommandData::CommandData(CommandType _type): type(_type) {}
CommandType CommandData::getType() const {
    return type;
}

std::string CommandImport::toString() const {
    return "import " + name + ";";
}
CommandImport::CommandImport(const std::string &_name, const CommandList &_commands):
    CommandData(CommandType::IMPORT), name(_name), commands(_commands) {
}

CommandBind::CommandBind(const std::string &_name, const Binding &_binding):
    CommandData(CommandType::BIND), name(_name), binding(_binding) {
}
std::string CommandBind::toString() const {
    return name + " = " + binding->toString() + ";";
}

CommandDefInductive::CommandDefInductive(const Ty &__type):
    CommandData(CommandType::DEF_IND), _type(__type) {
    type = dynamic_cast<TyInductive*>(_type.get());
    if (!type) {
        LOG(FATAL) << "Expected TyInductive but get " << _type->toString();
    }
}
std::string CommandDefInductive::toString() const {
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