//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_Z3_SPLITOR_H
#define ISTOOL_Z3_SPLITOR_H

#include "istool/ext/z3/z3_verifier.h"
#include "splitor.h"

class Z3Splitor: public Splitor {
protected:
    virtual bool getSplitExample(Program* cons_program, const FunctionContext& info,
                                 const ProgramList& seed_list, Example* counter_example, TimeGuard* guard);
public:
    Z3IOExampleSpace* io_space;
    Z3Extension* ext;
    Env* env;
    TypeList inp_type_list;
    PType oup_type;
    Z3Splitor(ExampleSpace* _example_space, const PType& _oup_type, const TypeList& _inp_type_list);
    virtual ~Z3Splitor() = default;
};

#endif //ISTOOL_Z3_SPLITOR_H
