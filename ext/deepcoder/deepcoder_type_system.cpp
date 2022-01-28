//
// Created by pro on 2022/1/25.
//

#include "istool/ext/deepcoder/deepcoder_type_system.h"
#include "istool/ext/deepcoder/data_value.h"
#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/ext/deepcoder/anonymous_function.h"
#include "istool/basic/type.h"

typedef std::pair<PType, PType> Equation;
typedef std::vector<Equation> EquationList;

namespace {
    std::pair<bool, PType> _applySub(const PType& x, const std::string& name, const PType& y) {
        auto* tv = dynamic_cast<TVar*>(x.get());
        if (tv) {
            if (tv->name != name) return {false, x};
            return {true, y};
        }
        TypeList sub_types;
        bool is_changed = false;
        for (const auto& sub: x->getParams()) {
            auto sub_res = _applySub(sub, name, y);
            if (sub_res.first) is_changed = true;
            sub_types.push_back(sub_res.second);
        }
        if (is_changed) return {true, x->clone(sub_types)};
        return {false, x};
    }

    bool _isOccur(const PType& x, const std::string& name) {
        auto* tv = dynamic_cast<TVar*>(x.get());
        if (tv) return tv->name == name;
        for (const auto& sub: x->getParams()) {
            if (_isOccur(sub, name)) return true;
        }
        return false;
    }

    bool _getMGU(const EquationList& equation_list, std::unordered_map<std::string, PType>& res) {
        auto equations = equation_list; res.clear();
        for (int i = 0; i < equations.size(); ++i) {
            auto l = equations[i].first, r = equations[i].second;
            auto *lv = dynamic_cast<TVar *>(l.get()), *rv = dynamic_cast<TVar *>(r.get());
            if (!lv) {
                std::swap(l, r);
                std::swap(lv, rv);
            }
            if (lv) {
                if (_isOccur(r, lv->name)) return false;
                for (int j = i + 1; j < equations.size(); ++j) {
                    equations[j].first = _applySub(equations[j].first, lv->name, r).second;
                    equations[j].second = _applySub(equations[j].second, lv->name, r).second;
                }
                for (auto &info: res) {
                    info.second = _applySub(info.second, lv->name, r).second;
                }
                res[lv->name] = r;
            }
            if (l->getBaseName() != r->getBaseName()) return false;
            auto l_params = l->getParams(), r_params = r->getParams();
            if (l_params.size() != r_params.size()) return false;
            for (int j = 0; j < l_params.size(); ++j) {
                equations.emplace_back(l_params[j], r_params[j]);
            }
        }
        return true;
    }
}

PType DeepCoderTypeSystem::intersect(const PType &x, const PType &y) {
    std::unordered_map<std::string, PType> res;
    if (!_getMGU({{x, y}}, res)) return nullptr;
    return type::substituteVar(x, res);
}

PType DeepCoderTypeSystem::getType(Program *p) {
    auto* ts = dynamic_cast<TypedSemantics*>(p->semantics.get());
    if (ts) {
        TypeList sub_list;
        for (const auto& sub_program: p->sub_list) {
            auto sub_type = getType(sub_program.get());
            if (!sub_type) return nullptr;
            sub_list.push_back(sub_type);
        }
        if (sub_list.size() != ts->inp_type_list.size()) return nullptr;
        EquationList equation_list;
        for (int i = 0; i < sub_list.size(); ++i) {
            equation_list.emplace_back(sub_list[i], ts->inp_type_list[i]);
        }
        std::unordered_map<std::string, PType> sigma;
        if (!_getMGU(equation_list, sigma)) return nullptr;
        return type::substituteVar(ts->oup_type, sigma);
    }

    // TODO: finish this part
}