//
// Created by pro on 2022/9/18.
//

#ifndef ISTOOL_INCRE_PROGRAM_H
#define ISTOOL_INCRE_PROGRAM_H

#include "incre_term.h"
#include "incre_type.h"
#include "incre_context.h"
#include <unordered_set>

namespace incre {
    enum class CommandType {
        IMPORT, BIND, DEF_IND
    };

    enum class CommandDecorate {
        INPUT, START
    };

    typedef std::unordered_set<CommandDecorate> DecorateSet;

    class CommandData {
        CommandType type;
    public:
        DecorateSet decorate_set;
        CommandData(CommandType _type, const DecorateSet& _set);
        CommandType getType() const;
        bool isDecoratedWith(CommandDecorate deco) const;
        std::string toString();
        virtual std::string contentToString() const = 0;
        virtual ~CommandData() = default;
    };

    typedef std::shared_ptr<CommandData> Command;
    typedef std::vector<Command> CommandList;

    class CommandImport: public CommandData {
    public:
        std::string name;
        CommandList commands;
        CommandImport(const std::string& _name, const CommandList& _commands);
        virtual std::string contentToString() const;
        virtual ~CommandImport() = default;
    };

    class CommandBind: public CommandData {
    public:
        std::string name;
        Binding binding;
        CommandBind(const std::string& _name, const Binding& _binding, const DecorateSet& decorate_set);
        virtual std::string contentToString() const;
        virtual ~CommandBind() = default;
    };

    class CommandDefInductive: public CommandData {
    public:
        TyInductive* type;
        Ty _type;
        CommandDefInductive(const Ty& __type);
        virtual std::string contentToString() const;
        virtual ~CommandDefInductive() = default;
    };

    class ProgramData {
    public:
        CommandList commands;
        ProgramData(const CommandList& _commands);
        void print() const;
        virtual ~ProgramData() = default;
    };
    typedef std::shared_ptr<ProgramData> IncreProgram;

    CommandDecorate string2Decorate(const std::string& s);
    std::string decorate2String(CommandDecorate deco);
}

#endif //ISTOOL_INCRE_PROGRAM_H
