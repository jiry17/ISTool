//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_EXAMPLE_SPACE_H
#define ISTOOL_EXAMPLE_SPACE_H

#include "program.h"

class Example {
public:
    virtual bool isSatisfy(const PProgram& program) = 0;
    virtual std::string toString() const = 0;
    virtual ~Example() = default;
};

typedef std::shared_ptr<Example> PExample;

class IOExample: public Example {
public:
    DataList inp_list;
    Data oup;
    IOExample(DataList&& _inp_list, Data&& _oup);
    virtual std::string toString() const;
    virtual bool isSatisfy(const PProgram& program);
    virtual ~IOExample() = default;
};

class ExampleSpace {
public:
    std::vector<PExample> example_space;
    ExampleSpace(const std::vector<PExample>& _example_space = {});
    virtual PExample get(int i) = 0;
    virtual ~ExampleSpace() = default;
};

class FiniteExampleSpace: public ExampleSpace {
public:
    int size() const;
    virtual PExample get(int i);
    FiniteExampleSpace(const std::vector<PExample>& _example_list);
    ~FiniteExampleSpace() = default;
};

class StreamedExampleSpace: public ExampleSpace {
public:
    StreamedExampleSpace() = default;
    virtual PExample get(int i);
    virtual void extendExampleSpace() = 0;
};

#endif //ISTOOL_EXAMPLE_SPACE_H
