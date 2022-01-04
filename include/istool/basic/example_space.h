//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_EXAMPLE_SPACE_H
#define ISTOOL_EXAMPLE_SPACE_H

#include "program.h"
#include "env.h"

// TODO: Split example space and oracle

typedef DataList Example;
typedef std::vector<DataList> ExampleList;
typedef std::pair<Example, Data> IOExample;
typedef std::vector<IOExample> IOExampleList;

class ExampleSpace {
public:
    PProgram cons_program;
    ExampleSpace(const PProgram& _cons_program);
    virtual bool satisfyExample(const FunctionContext& info, const Example& example);
    virtual ~ExampleSpace() = default;
};

class IOExampleSpace {
public:
    std::string func_name;
    IOExampleSpace(const std::string& _func_name);
    virtual IOExample getIOExample(const Example& example) const = 0;
    virtual ~IOExampleSpace() = default;
};

class FiniteExampleSpace: public ExampleSpace {
public:
    ExampleList example_space;
    FiniteExampleSpace(const PProgram& _cons_program, const ExampleList& _example_space);
    virtual ~FiniteExampleSpace() = default;
};

class FiniteIOExampleSpace: public FiniteExampleSpace, public IOExampleSpace {
public:
    ProgramList inp_list;
    PProgram oup;
    FiniteIOExampleSpace(const PProgram& _cons_program, const ExampleList& _example_space, const std::string& _name,
            const ProgramList& _inp_list, const PProgram& _oup);
    virtual bool satisfyExample(const FunctionContext& info, const Example& example);
    virtual IOExample getIOExample(const Example& example) const;
    virtual ~FiniteIOExampleSpace() = default;
};

namespace example {
    FiniteIOExampleSpace* buildFiniteIOExampleSpace(const IOExampleList& examples, const std::string& name, Env* env);
    bool satisfyIOExample(Program* program, const IOExample& example);
    std::string ioExample2String(const IOExample& example);
}


#endif //ISTOOL_EXAMPLE_SPACE_H
