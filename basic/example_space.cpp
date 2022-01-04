//
// Created by pro on 2021/12/4.
//

#include "istool/basic/example_space.h"
#include "glog/logging.h"
#include <unordered_map>

namespace {
    bool getBool(const Data& data) {
        auto* bv = dynamic_cast<BoolValue*>(data.get());
        return bv->w;
    }
}

ExampleSpace::ExampleSpace(const PProgram &_cons_program): cons_program(_cons_program) {
}
bool ExampleSpace::satisfyExample(const FunctionContext &info, const Example &example) {
    try {
        auto res = program::runWithFunc(cons_program.get(), example, info);
        return getBool(res);
    } catch (SemanticsError& e) {
        return false;
    }
}

IOExampleSpace::IOExampleSpace(const std::string &_func_name): func_name(_func_name) {
}

FiniteExampleSpace::FiniteExampleSpace(const PProgram &_cons_program, const ExampleList &_example_space):
    ExampleSpace(_cons_program), example_space(_example_space) {
}
FiniteIOExampleSpace::FiniteIOExampleSpace(const PProgram &_cons_program, const ExampleList &_example_space,
        const std::string &_name, const ProgramList &_inp_list, const PProgram &_oup):
        FiniteExampleSpace(_cons_program, _example_space), IOExampleSpace(_name), inp_list(_inp_list), oup(_oup) {
}
IOExample FiniteIOExampleSpace::getIOExample(const Example &example) const {
    DataList inp_vals;
    for (const auto& p: inp_list) {
        inp_vals.push_back(program::run(p.get(), example));
    }
    auto oup_val = program::run(oup.get(), example);
    return {inp_vals, oup_val};
}
bool FiniteIOExampleSpace::satisfyExample(const FunctionContext &ctx, const Example &example) {
    auto io_example = getIOExample(example);
    if (ctx.find(func_name) == ctx.end()) {
        LOG(FATAL) << "Cannot find program " << func_name;
    }
    return example::satisfyIOExample(ctx.find(func_name)->second.get(), io_example);
}

bool example::satisfyIOExample(Program *program, const IOExample &example) {
    return program::run(program, example.first) == example.second;
}

FiniteIOExampleSpace * example::buildFiniteIOExampleSpace(const IOExampleList &examples, const std::string& name, Env *env) {
    if (examples.empty()) {
        LOG(FATAL) << "Example space should not be empty";
    }
    int n = examples[0].first.size();
    ProgramList l_subs;
    TypeList inp_types;
    for (int i = 0; i < n; ++i) {
        auto type = examples[0].first[i].getPType();
        l_subs.push_back(program::buildParam(i, type));
        inp_types.push_back(type);
    }
    auto oup_type = examples[0].second.getPType();
    auto r = program::buildParam(n, oup_type);
    auto l = std::make_shared<Program>(
            std::make_shared<InvokeSemantics>(name, oup_type, inp_types),
            l_subs);
    ProgramList sub_list = {l, r};
    auto cons_program = std::make_shared<Program>(env->getSemantics("="), sub_list);
    ExampleList example_list;
    for (auto& io_example: examples) {
        Example example = io_example.first;
        example.push_back(io_example.second);
        example_list.push_back(example);
    }

    return new FiniteIOExampleSpace(cons_program, example_list, name, l_subs, r);
}

std::string example::ioExample2String(const IOExample &example) {
    return data::dataList2String(example.first) + "=>" + example.second.toString();
}