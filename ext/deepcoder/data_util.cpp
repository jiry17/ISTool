//
// Created by pro on 2022/1/21.
//

#include "istool/ext/deepcoder/data_util.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/ext/deepcoder/data_value.h"
#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include <unordered_set>
#include "glog/logging.h"

#define MismatchInfo LOG(FATAL) << "Data " << data.toString() << " does not match functor " << type->getName()

Data ext::ho::polyFMap(Program *p, Type *type, const Data &data, Env *env) {
    auto* st = dynamic_cast<TSum*>(type);
    if (st) {
        auto* sv = dynamic_cast<SumValue*>(data.get());
        if (!sv) MismatchInfo;
        int id = sv->id; Data res = polyFMap(p, st->sub_types[id].get(), sv->value, env);
        return Data(std::make_shared<SumValue>(id, res));
    }
    auto* pt = dynamic_cast<TProduct*>(type);
    if (pt) {
        auto* pv = dynamic_cast<ProductValue*>(data.get());
        if (!pv || pv->elements.size() != pt->sub_types.size()) MismatchInfo;
        DataList elements;
        for (int i = 0; i < pv->elements.size(); ++i) {
            elements.push_back(polyFMap(p, pt->sub_types[i].get(), pv->elements[i], env));
        }
        return Data(std::make_shared<ProductValue>(elements));
    }
    auto* vt = dynamic_cast<TVar*>(type);
    if (vt) return env->run(p, {data});
    return data;
}

namespace {
    void _splitTriangle(const PProgram& p, ProgramList& res) {
        if (p->semantics->name == "tri") {
            for (const auto& sub: p->sub_list) {
                _splitTriangle(sub, res);
            }
        } else {
            res.push_back(p);
        }
    }
}

ProgramList ext::ho::splitTriangle(const PProgram& p) {
    ProgramList res;
    _splitTriangle(p, res);
    return res;
}

namespace {
    std::pair<bool, PProgram> _removeTriAccess(const PProgram &p) {
        auto *as = dynamic_cast<AccessSemantics *>(p->semantics.get());
        if (as) {
            int id = as->id;
            auto content = p->sub_list[0];
            auto *ts = dynamic_cast<TriangleSemantics *>(content->semantics.get());
            if (ts) {
                assert(id >= 0 && id < content->sub_list.size());
                return {true, content->sub_list[id]};
            }
        }
        bool is_changed = false;
        ProgramList sub_list;
        for (const auto& sub: p->sub_list) {
            auto res = _removeTriAccess(sub);
            if (res.first) is_changed = true;
            sub_list.push_back(res.second);
        }
        if (!is_changed) return {false, p};
        return {true, std::make_shared<Program>(p->semantics, sub_list)};
    }
}

PProgram ext::ho::removeTriAccess(const PProgram &p) {
    return _removeTriAccess(p).second;
}

PProgram ext::ho::buildAccess(const PProgram &p, const std::vector<int> &trace) {
    PProgram res = p;
    for (int id: trace) {
        ProgramList sub_list({res});
        res = std::make_shared<Program>(std::make_shared<AccessSemantics>(id), sub_list);
    }
    return res;
}

PProgram ext::ho::buildTriangle(const ProgramList &sub_list) {
    return std::make_shared<Program>(std::make_shared<TriangleSemantics>(), sub_list);
}

PProgram ext::ho::mergeTriangle(const ProgramList &p_list) {
    ProgramList sub_list;
    std::unordered_set<std::string> p_set;
    for (const auto& p: p_list) {
        auto comp_list = splitTriangle(p);
        for (const auto& comp: comp_list) {
            auto feature = comp->toString();
            if (p_set.find(feature) != p_set.end()) continue;
            p_set.insert(feature); sub_list.push_back(p);
        }
    }
    return buildTriangle(sub_list);
}