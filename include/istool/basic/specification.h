//
// Created by pro on 2021/12/5.
//

#ifndef ISTOOL_SPECIFICATION_H
#define ISTOOL_SPECIFICATION_H

#include "grammar.h"
#include "example_space.h"
#include "env.h"

struct SynthInfo {
public:
    std::string name;
    TypeList inp_type_list;
    PType oup_type;
    Grammar* grammar;
    SynthInfo(const std::string& _name, const TypeList& _inp, const PType& _oup, Grammar* _g);
    ~SynthInfo();
};
typedef std::shared_ptr<SynthInfo> PSynthInfo;

class Specification {
public:
    std::vector<PSynthInfo> info_list;
    Env* env;
    ExampleSpace* example_space;
    Specification(const std::vector<PSynthInfo>& _info_list, Env* _env, ExampleSpace* _example_space);
    virtual ~Specification();
};


#endif //ISTOOL_SPECIFICATION_H
