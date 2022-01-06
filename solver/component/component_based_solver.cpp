//
// Created by pro on 2021/12/9.
//

#include "istool/solver/component/component_based_solver.h"
#include "istool/ext/z3/z3_extension.h"
#include "istool/ext/z3/z3_example_space.h"
#include "glog/logging.h"

ComponentBasedSynthesizer::ComponentBasedSynthesizer(Specification *_spec, const std::unordered_map<std::string, Z3GrammarEncoder *> &_map):
    encoder_map(_map), PBESolver(_spec), ext(ext::z3::getExtension(_spec->env.get())) {
    for (const auto& info_list: spec->info_list) {
        if (encoder_map.find(info_list->name) == encoder_map.end()) {
            LOG(FATAL) << "Cannot find encoder for " << info_list->name;
        }
    }
}

ComponentBasedSynthesizer::~ComponentBasedSynthesizer() noexcept {
    for (auto& info: encoder_map) {
        delete info.second;
    }
}

Z3EncodeRes ComponentBasedSynthesizer::encodeExample(Program *program, const Example &example, const std::string& prefix,
        std::unordered_map<std::string, Z3EncodeRes>& cache) const {
    std::string feature = program->toString();
    if (cache.find(feature) != cache.end()) return cache.find(feature)->second;
    auto* iv = dynamic_cast<InvokeSemantics*>(program->semantics.get());
    std::vector<Z3EncodeRes> res_list;
    for (const auto& sub: program->sub_list) {
        res_list.push_back(encodeExample(sub.get(), example, prefix, cache));
    }
    int feature_id = cache.size();
    if (iv) {
        if (encoder_map.find(iv->name) == encoder_map.end()) {
            LOG(FATAL) << "Cannot find program " << iv->name;
        }
        auto* encoder = encoder_map.find(iv->name)->second;
        z3::expr_vector sub_values(ext->ctx);
        for (auto& sub_res: res_list) {
            sub_values.push_back(sub_res.res);
        }
        auto res = encoder->encodeExample(sub_values, prefix + std::to_string(feature_id) + "@");
        for (auto& sub_res: res_list) {
            for (const auto& cons: sub_res.cons_list) {
                res.cons_list.push_back(cons);
            }
        }
        cache.insert({feature, res});
        return res;
    }
    z3::expr_vector param_list(ext->ctx);
    for (const auto& d: example) param_list.push_back(ext->buildConst(d));
    auto res = ext->encodeZ3ExprForSemantics(program->semantics.get(), res_list, param_list);
    cache.insert({feature, res});
    return res;
}

Z3EncodeRes ComponentBasedSynthesizer::encodeExample(Program *program, const Example &example, const std::string &prefix) {
    std::unordered_map<std::string, Z3EncodeRes> cache;
    return encodeExample(program, example, prefix, cache);
}

FunctionContext ComponentBasedSynthesizer::synthesis(const ExampleList& example_list, TimeGuard* guard) {
    auto* space = dynamic_cast<Z3ExampleSpace*>(spec->example_space.get());
    while (1) {
        TimeCheck(guard);
        z3::solver solver(ext->ctx);
        if (guard) {
            double remain_time = std::max(guard->getRemainTime() * 1e3, 1.0);
            z3::params p(ext->ctx);
            p.set(":timeout", int(remain_time) + 1u);
            solver.set(p);
        }
        for (const auto& info: spec->info_list) {
            auto* encoder = encoder_map[info->name];
            solver.add(z3::mk_and(encoder->encodeStructure(info->name + "@structure")));
        }
        for (int i = 0; i < example_list.size(); ++i) {
            auto res = encodeExample(space->cons_program.get(), example_list[i], "val@" + std::to_string(i) + "@");
            solver.add(res.res);
            solver.add(z3::mk_and(res.cons_list));
        }
        auto res = solver.check();
        if (res == z3::unsat) {
            for (auto& encoder_info: encoder_map) {
                encoder_info.second->enlarge();
            }
        } else if (res == z3::sat) {
            auto model = solver.get_model();
            FunctionContext result;
            for (auto& encoder_info: encoder_map) {
                result[encoder_info.first] = encoder_info.second->programBuilder(model);
            }
            return result;
        } else if (res == z3::unknown) {
            throw TimeOutError();
        }
    }
}