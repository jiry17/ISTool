//
// Created by pro on 2021/12/7.
//

#include "istool/ext/z3/z3_verifier.h"
#include "istool/ext/z3/z3_extension.h"
#include <map>
#include "glog/logging.h"

Z3Verifier::Z3Verifier(Z3ExampleSpace *_example_space): example_space(_example_space), ext(_example_space->ext) {
}

Z3EncodeRes Z3Verifier::encodeConsProgram(const PProgram &program, const FunctionContext &info,
        const z3::expr_vector& param_list, std::unordered_map<Program*, Z3EncodeRes>& cache) const {
    if (cache.find(program.get()) != cache.end()) {
        return cache.find(program.get())->second;
    }
    std::vector<Z3EncodeRes> sub_list;
    for (auto& p: program->sub_list) {
        sub_list.push_back(encodeConsProgram(p, info, param_list, cache));
    }

    auto* iv = dynamic_cast<InvokeSemantics*>(program->semantics.get());
    if (iv) {
        std::string name = iv->name;
        if (info.find(name) == info.end()) {
            LOG(FATAL) << "Cannot find program " << name;
        }
        z3::expr_vector sub_param_list(param_list.ctx());
        for (auto& sub: sub_list) sub_param_list.push_back(sub.res);
        auto encode_res = ext->encodeZ3ExprForProgram(info.find(name)->second.get(), sub_param_list);
        for (auto& sub: sub_list) {
            for (const auto& cons: sub.cons_list) encode_res.cons_list.push_back(cons);
        }
        cache.insert(std::make_pair(program.get(), encode_res));
        return encode_res;
    }
    auto encode_res = ext->encodeZ3ExprForSemantics(program->semantics.get(), sub_list, param_list);
    cache.insert(std::make_pair(program.get(), encode_res));
    return encode_res;
}

Z3EncodeRes Z3Verifier::encodeConsProgram(const PProgram &program, const FunctionContext &info, const z3::expr_vector &param_list) const {
    std::unordered_map<Program*, Z3EncodeRes> cache;
    return encodeConsProgram(program, info, param_list, cache);
}

bool Z3Verifier::verify(const FunctionContext &info, Example *counter_example) {
    z3::solver s(ext->ctx);
    z3::expr_vector param_list(ext->ctx);
    for (int i = 0; i < example_space->type_list.size(); ++i) {
        param_list.push_back(ext->buildVar(example_space->type_list[i].get(), "Param" + std::to_string(i)));
    }
    auto encode_res = encodeConsProgram(example_space->cons_program, info, param_list);
    s.add(!encode_res.res || !z3::mk_and(encode_res.cons_list));
    auto res = s.check();
    if (res == z3::unsat) return true;
    if (res != z3::sat) {
        LOG(FATAL) << "Z3 failed with " << res;
    }
    auto model = s.get_model();
    if (s.check() == z3::sat) {
        model = s.get_model();
    }
    if (counter_example) {
        counter_example->clear();
        for (int i = 0; i < param_list.size(); ++i) {
            counter_example->push_back(ext->getValueFromModel(model, param_list[i], example_space->type_list[i].get()));
        }
    }
    return false;
}