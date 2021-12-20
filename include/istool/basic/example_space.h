//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_EXAMPLE_SPACE_H
#define ISTOOL_EXAMPLE_SPACE_H

#include "program.h"
#include "env.h"

typedef DataList Example;
typedef std::vector<DataList> ExampleList;
typedef std::pair<Example, Data> IOExample;
typedef std::vector<IOExample> IOExampleList;

class ExampleSpace {
public:
    int oup_id;
    PProgram cons_program;
    ExampleSpace(const PProgram& _cons_program, int _oup_id = -1);
    bool isIO() const;
    bool satisfy(const Example& example, const FunctionContext& ctx);
    virtual ~ExampleSpace() = default;
};

class FiniteExampleSpace: public ExampleSpace {
public:
    ExampleList example_space;
    FiniteExampleSpace(const PProgram& _cons_program, const ExampleList& _example_space, int _oup_id = -1);
};

namespace example {
    ExampleSpace* buildIOExampleSpace(const std::vector<IOExample>& example_space, Env* env);
    FunctionContext buildDummyFuncContext(const PProgram& program);
    IOExample example2IOExample(const Example& example, ExampleSpace* space);
    bool isSatisfyIO(const IOExample& example, const PProgram& program);
}

#endif //ISTOOL_EXAMPLE_SPACE_H
