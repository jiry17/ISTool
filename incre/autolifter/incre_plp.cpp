//
// Created by pro on 2022/9/26.
//

#include "istool/incre/autolifter/incre_plp_solver.h"
#include "istool/ext/deepcoder/data_type.h"

using namespace incre::autolifter;

PLPInputGenerator::PLPInputGenerator(Env* _env, IncreExampleList *_examples, const std::vector<std::string> &_names, int _compress_id):
    examples(_examples), names(_names), env(_env), compress_id(_compress_id) {
}
PLPInputGenerator::~PLPInputGenerator() {
    for (auto* data: cache_list) delete data;
}

DataList PLPInputGenerator::getPartial(int id) const {
    assert(id < examples->size());
    DataList res(names.size());
    auto example = examples->at(id);
    for (int i = 0; i < names.size(); ++i) res[i] = example->inputs[names[i]];
    return res;
}
void PLPInputGenerator::extendPartial(int id) {
    for (int i = partial_examples.size(); i <= id; ++i) {
        partial_examples.push_back(getPartial(i));
    }
}

Data PLPInputGenerator::getInput(Program *program, int id) {
    extendPartial(id);
    auto& example = partial_examples[id];
    DataList res(example.size());
    for (int i = 0; i < example.size(); ++i) {
        res[i] = env->run(program, {example[i]});
    }
    return BuildData(Product, res);
}
DataList * PLPInputGenerator::getCache(Program *program, int len) {
    auto name = program->toString();
    if (program_map.find(name) == program_map.end()) return nullptr;
    auto* cache = cache_list[program_map[name]];
    for (int i = cache->size(); i < len; ++i) {
        cache->push_back(getInput(program, i));
    }
    return cache;
}
void PLPInputGenerator::registerProgram(Program* program, DataList *cache) {
    auto name = program->toString();
    assert(program_map.find(name) == program_map.end());
    program_map[name] = cache_list.size();
    cache_list.push_back(cache);
}

namespace {
    Data _extract(const Data& d, const std::vector<int>& trace) {
        auto res = d;
        for (int pos: trace) {
            auto* v = dynamic_cast<ProductValue*>(res.get());
            assert(v && v->elements.size() > pos && pos >= 0);
            res = v->elements[pos];
        }
        return res;
    }
}

PLPOutputGenerator::PLPOutputGenerator(Env *_env, IncreExampleList *_examples, const std::vector<int> &_trace, int _compress_id):
    env(_env), examples(_examples), trace(_trace), compress_id(_compress_id) {
}
Data PLPOutputGenerator::getPartial(int id) const {
    assert(examples->size() > id);
    return _extract(examples->at(id)->oup, trace);
}
void PLPOutputGenerator::extendPartial(int id) {
    for (int i = partial_outputs.size(); i <= id; ++i) {
        partial_outputs.push_back(getPartial(id));
    }
}
DataList * PLPOutputGenerator::getCache(Program *program, int len) {
    auto name = program->toString();
    if (program_map.find(name) == program_map.end()) {
        program_map[name] = cache_list.size();
        cache_list.push_back(new DataList());
    }
    auto* cache = cache_list[program_map[name]];
    extendPartial(len - 1);
    for (int i = cache->size(); i < len; ++i) {
        auto w = partial_outputs[i];
        cache->push_back(env->run(program, {w}));
    }
    return cache;
}
PLPOutputGenerator::~PLPOutputGenerator() {
    for (auto* cache: cache_list) delete cache;
}

PLPConstGenerator::PLPConstGenerator(IncreExampleList *_examples, const std::vector<std::string> &_const_names):
    examples(_examples), const_names(_const_names) {
}
DataList * PLPConstGenerator::getCache(int len) {
    assert(examples->size() >= len);
    for (int i = cache.size(); i < len; ++i) {
        auto example = examples->at(i);
        DataList consts(const_names.size());
        for (int j = 0; j < const_names.size(); ++j) {
            consts[j] = example->inputs[const_names[j]];
        }
        cache.push_back(BuildData(Product, consts));
    }
    return &cache;
}

PLPInfo::PLPInfo(const std::vector<InputGenerator> &_inps, const std::vector<Grammar *> &_grammars, const OutputGenerator &_o, const ConstGenerator &_c, IncreExamplePool *_pool):
    inps(_inps), o(_o), f_grammars(_grammars), c(_c), pool(_pool) {
}