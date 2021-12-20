//
// Created by pro on 2021/12/4.
//

#include "istool/basic/example_space.h"
#include "glog/logging.h"

bool ExampleSpace::isIO() const {
    return oup_id != -1;
}
ExampleSpace::ExampleSpace(const PProgram &_cons_program, int _oup_id):
    cons_program(_cons_program), oup_id(_oup_id) {
}
bool ExampleSpace::satisfy(const Example &example, const FunctionContext &ctx) {
    auto res = program::runWithFunc(cons_program, example, ctx);
    auto* bv = dynamic_cast<BoolValue*>(res.get());
    if (bv) {
        LOG(FATAL) << "Cons program must return a boolean value";
    }
    return bv->w;
}

FiniteExampleSpace::FiniteExampleSpace(const PProgram &_cons_program, const ExampleList &_example_space, int _oup_id):
    ExampleSpace(_cons_program, _oup_id), example_space(_example_space) {
}

namespace {
    const std::string KDummyName = "f";
}

FunctionContext example::buildDummyFuncContext(const PProgram &program) {
    return {{KDummyName, program}};
}

ExampleSpace * example::buildIOExampleSpace(const std::vector<IOExample> &io_example_space, Env* env) {
    if (io_example_space.empty()) {
        LOG(FATAL) << "Example space should not be empty";
    }
    auto example = io_example_space[0];
    TypeList inp_type_list;
    for (const auto& data: example.first) inp_type_list.push_back(data.getPType());
    PType oup_type = example.second.getPType();
    int oup_id = int(inp_type_list.size());

    ProgramList sub_list;
    for (int i = 0; i < inp_type_list.size(); ++i) {
        sub_list.push_back(program::buildParam(i, inp_type_list[i]));
    }

    PProgram r = program::buildParam(oup_id, oup_type);
    PProgram l = std::make_shared<Program>(
            std::make_shared<InvokeSemantics>(KDummyName, std::move(oup_type), std::move(inp_type_list)),
            std::move(sub_list));
    sub_list = {l, r};
    PProgram cons_program = std::make_shared<Program>(env->getSemantics("="), std::move(sub_list));

    ExampleList example_space;
    for (auto& io_example: io_example_space) {
        auto new_example = io_example.first;
        new_example.push_back(io_example.second);
        example_space.push_back(new_example);
    }

    return new FiniteExampleSpace(cons_program, example_space, oup_id);
}

IOExample example::example2IOExample(const Example &example, ExampleSpace *space) {
    if (!space->isIO()) {
        LOG(FATAL) << "Expected IO Example Space";
    }
    
}