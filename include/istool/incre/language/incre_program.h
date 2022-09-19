//
// Created by pro on 2022/9/18.
//

#ifndef ISTOOL_INCRE_PROGRAM_H
#define ISTOOL_INCRE_PROGRAM_H

#include "incre_term.h"
#include "incre_type.h"
#include "incre_context.h"

namespace incre {
    enum class CommandType {
        IMPORT, BIND, DEF_IND
    };

    class CommandData {
        CommandType type;
    public:
        CommandData(CommandType _type);
        CommandType getType() const;
        virtual std::string toString() const = 0;
        virtual ~CommandData() = default;
    };

    typedef std::shared_ptr<CommandData> Command;
    typedef std::vector<Command> CommandList;

    class CommandImport: public CommandData {
    public:
        std::string name;
        CommandList commands;
        CommandImport(const std::string& _name, const CommandList& _commands);
        virtual std::string toString() const;
        virtual ~CommandImport() = default;
    };

    class CommandBind: public CommandData {
    public:
        std::string name;
        Binding binding;
        CommandBind(const std::string& _name, const Binding& _binding);
        virtual std::string toString() const;
        virtual ~CommandBind() = default;
    };

    class CommandDefInductive: public CommandData {
    public:
        TyInductive* type;
        Ty _type;
        CommandDefInductive(const Ty& __type);
        virtual std::string toString() const;
        virtual ~CommandDefInductive() = default;
    };

    class ProgramData {
    public:
        CommandList commands;
        ProgramData(const CommandList& _commands);
        void print() const;
        virtual ~ProgramData() = default;
    };
    typedef std::shared_ptr<ProgramData> Program;
}

#endif //ISTOOL_INCRE_PROGRAM_H
