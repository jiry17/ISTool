//
// Created by pro on 2022/1/20.
//

#include "istool/ext/deepcoder/data_util.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/solver/autolifter/basic/lifting_problem.h"
#include "glog/logging.h"

DataList* LiftingCache::registerProgram(Program *program, const DataList &res) {
    auto feature = program->toString();
    if (program_map.find(feature) != program_map.end()) {
        LOG(FATAL) << "Program " << program->toString() << " has been cached";
    }
    auto item = new DataList(res); int id = cache.size();
    program_map[feature] = id;
    cache.push_back(item);
    return item;
}
DataList * LiftingCache::getCache(Program *program) {
    auto feature = program->toString();
    auto it = program_map.find(feature);
    if (it == program_map.end()) return nullptr;
    return cache[it->second];
}
LiftingCache::~LiftingCache() {
    for (auto* cache_item: cache) delete cache_item;
}

LiftingModInfo::LiftingModInfo(const PProgram &_m, const PType &_F, const PStreamedExampleSpace& _example_space, CombinatorGrammarBuilder* _c_builder):
    m(_m), F(_F), example_space(_example_space), env(_example_space->env), c_builder(_c_builder) {
}
LiftingModInfo::~LiftingModInfo() {
    delete c_builder;
}

Data LiftingModInfo::getFMapResult(Program *p, int id) {
    auto inp = example_space->getExample(id);
    return ext::ho::polyFMap(p, F.get(), inp[0], env);
}
DataList * LiftingModInfo::getFMapCache(Program *p) {
    return fmap_cache.getCache(p);
}
DataList* LiftingModInfo::registerFMapCache(Program *p, const DataList &res) {
    return fmap_cache.registerProgram(p, res);
}
int LiftingModInfo::extendFMapCache(Program *p, DataList *cache, int target) {
    int now = cache->size();
    target = example_space->extendExampleList(target, nullptr);
    if (now < target) {
        cache->resize(target);
        for (int i = now; i < target; ++i) {
            cache->at(i) = getFMapResult(p, i);
        }
    }
}

Data LiftingModInfo::getModResult(Program *p, int id) {
    auto inp = example_space->getExample(id);
    auto m_res = env->run(m.get(), inp);
    return env->run(p, inp);
}
DataList * LiftingModInfo::getModCache(Program *p) {
    return mod_cache.getCache(p);
}
DataList* LiftingModInfo::registerModCache(Program *p, const DataList &res) {
    return mod_cache.registerProgram(p, res);
}
int LiftingModInfo::extendModCache(Program *p, DataList *cache, int target) {
    int now = cache->size();
    target = example_space->extendExampleList(target, nullptr);
    if (now < target) {
        cache->resize(target);
        for (int i = now; i < target; ++i) {
            cache->at(i) = getModResult(p, i);
        }
    }
}

LiftingTask::LiftingTask(const std::vector<PLiftingModInfo> &_info_list, const PProgram &_p, const PProgram& _h,
        const PSynthInfo& _f_info, const PEnv& _env): p(_p), info_list(_info_list), f_info(_f_info), h(_h), env(_env) {
}
PartialLiftingTask::PartialLiftingTask(const PLiftingModInfo &_info, const PProgram &_p, const PProgram &_h,
        const PSynthInfo& _f_info, const PEnv& _env): info(_info), p(_p), h(_h), f_info(_f_info), env(_env) {
}

namespace {
    PProgram _constructFMapInput(Type* type, const PProgram& inp, const std::function<PProgram(const PProgram&)>& rewrite) {
        auto* st = dynamic_cast<TSum*>(type);
        if (st) {
            LOG(FATAL) << "AutoLifter: current implementation does not support sum type in the functor but finds" << type->getName();
        }
        auto* pt = dynamic_cast<TProduct*>(type);
        if (pt) {
            ProgramList sub_list;
            for (int i = 0; i < pt->sub_types.size(); ++i) {
                auto sub_inp = ext::ho::buildAccess(inp, {i});
                sub_list.push_back(_constructFMapInput(pt->sub_types[i].get(), inp, rewrite));
            }
            return ext::ho::buildTriangle(sub_list);
        }
        auto* vt = dynamic_cast<TVar*>(type);
        if (vt) return rewrite(inp);
        return inp;
    }
}

PProgram solver::autolifter::rewriteCombinator(Type* F, const PProgram &pre_h, const PProgram &pre_f,
        const PProgram &pre_c, const PProgram &new_h, const PProgram &new_f) {
    auto inp = program::buildParam(0);
    std::unordered_map<std::string, std::vector<int>> trace_map;

    auto new_h_list = ext::ho::splitTriangle(new_h);
    for (int i = 0; i < new_h_list.size(); ++i) {
        trace_map[new_h_list[i]->toString()] = {0, i};
    }
    auto new_f_list = ext::ho::splitTriangle(new_f);
    for (int i = 0; i < new_f_list.size(); ++i) {
        trace_map[new_f_list[i]->toString()] = {1, i};
    }

    auto get_trace = [&](const PProgram& p) {
        auto it = trace_map.find(p->toString());
        if (it == trace_map.end()) {
            LOG(FATAL) << new_h->toString() + " tri " << new_f->toString() << " does not cover " << pre_h->toString() << " tri " << pre_f->toString();
        }
        return it->second;
    };

    std::vector<std::vector<int>> h_trace_list, f_trace_list;
    for (const auto& pre_h_comp: ext::ho::splitTriangle(pre_h)) {
        h_trace_list.push_back(get_trace(pre_h_comp));
    }
    for (const auto& pre_f_comp: ext::ho::splitTriangle(pre_f)) {
        f_trace_list.push_back(get_trace(pre_f_comp));
    }

    auto rewrite = [=](const PProgram& program) {
        ProgramList sub_list;
        for (const auto& trace: h_trace_list) sub_list.push_back(ext::ho::buildAccess(program, trace));
        auto l = ext::ho::buildTriangle(sub_list);
        sub_list.clear();
        for (const auto& trace: f_trace_list) sub_list.push_back(ext::ho::buildAccess(program, trace));
        auto r = ext::ho::buildTriangle(sub_list);
        return ext::ho::buildTriangle({l, r});
    };

    auto new_inp = _constructFMapInput(F, inp, rewrite);
    auto res = program::rewriteParam(pre_c, {new_inp});
    return ext::ho::removeTriAccess(res);
}