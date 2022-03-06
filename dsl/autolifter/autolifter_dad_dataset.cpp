//
// Created by pro on 2022/2/23.
//

#include "istool/dsl/autolifter/autolifter_dataset.h"
#include "istool/dsl/autolifter/autolifter_dsl.h"
#include "istool/solver/autolifter/composed_sf_solver.h"
#include "istool/ext/deepcoder/data_type.h"
#include "istool/ext/deepcoder/data_value.h"
#include "istool/ext/deepcoder/deepcoder_semantics.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/ext/limited_type/limited_int.h"
#include "istool/ext/limited_type/limited_ds.h"
#include "istool/ext/deepcoder/anonymous_function.h"
#include "glog/logging.h"

using namespace dsl::autolifter;
#define TVARA type::getTVarA()
#define TLISTINT ext::ho::getTIntList()
using theory::clia::getIntValue;

namespace {
    struct _TypeConfig {
        int int_min = -5, int_max = 5;
        int length_max = 5;
    };

    PType _getLimitedInt(const _TypeConfig& config) {
        return std::make_shared<LimitedTInt>(config.int_min, config.int_max);
    }
    PType _getLimitedListInt(const _TypeConfig& config) {
        auto content = _getLimitedInt(config);
        return std::make_shared<LimitedTList>(config.length_max, content);
    }

    PProgram _buildAccess(const PProgram& base, int id) {
        auto sem = std::make_shared<AccessSemantics>(id);
        return std::make_shared<Program>(sem, (ProgramList){base});
    }

    LiftingModConfigInfo _getModConfigInfoDaD(Env* env) {
        auto F = std::make_shared<TProduct>((TypeList){TVARA, TVARA});
        auto x = program::buildParam(0, std::make_shared<TProduct>((TypeList){TLISTINT, TLISTINT}));
        auto m = std::make_shared<Program>(env->getSemantics("++"), (ProgramList){_buildAccess(x, 0), _buildAccess(x, 1)});
        return {m, F};
    }

    ListValue* _getListValue(const Data& data) {
        auto* res = dynamic_cast<ListValue*>(data.get());
        assert(res);
        return res;
    }

#define InfoStart(name, str_name) \
    LiftingConfigInfo _getConfigInfo ## name() { \
        auto env = std::make_shared<Env>(); \
        dsl::autolifter::prepareEnv(env.get()); \
        _TypeConfig type_config; \
        auto dad_mod_config = _getModConfigInfoDaD(env.get()); \
        auto inp_type = _getLimitedListInt(type_config);

#define InfoEnd(name, str_name) \
        return {_buildListProgram(ps, str_name), inp_type, env, {dad_mod_config}}; \
    }

    PProgram _buildListProgram(const FullSemanticsFunction& ps, const std::string& name) {
        auto sem = std::make_shared<TypedAnonymousSemantics>(ps, (TypeList){TLISTINT}, theory::clia::getTInt(), name);
        auto p = std::make_shared<Program>(sem, (ProgramList){program::buildParam(0, TLISTINT)});
        return p;
    }

    InfoStart(Sum, "sum")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            int res = 0;
            for (int i = 0; i < x->value.size(); ++i) {
                res += getIntValue(x->value[i]);
            }
            return BuildData(Int, res);
        };
    InfoEnd(Sum, "sum")

    InfoStart(Min, "min")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.empty()) throw SemanticsError();
            int res = getIntValue(x->value[0]);
            for (int i = 0; i < x->value.size(); ++i) {
                res = std::min(res, getIntValue(x->value[i]));
            }
            return BuildData(Int, res);
        };
    InfoEnd(Min, "min")

    InfoStart(Max, "max")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.empty()) throw SemanticsError();
            int res = getIntValue(x->value[0]);
            for (int i = 0; i < x->value.size(); ++i) {
                res = std::max(res, getIntValue(x->value[i]));
            }
            return BuildData(Int, res);
        };
    InfoEnd(Max, "max")

    InfoStart(Length, "length")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            return BuildData(Int, x->value.size());
        };
    InfoEnd(Length, "length")

    InfoStart(SecondMin, "2nd-min")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.size() < 2) throw SemanticsError();
            int min = getIntValue(x->value[0]);
            int second_min = getIntValue(x->value[1]);
            if (min > second_min) std::swap(min, second_min);
            for (int i = 2; i < x->value.size(); ++i) {
                int now = getIntValue(x->value[i]);
                if (now < min) second_min = min, min = now;
                else second_min = std::min(second_min, now);
            }
            return BuildData(Int, second_min);
        };
    InfoEnd(SecondMin, "2nd-min")

    InfoStart(ThirdMin, "3rd-min")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.size() < 3) throw SemanticsError();
            std::vector<int> value_list;
            for (auto& d: x->value) {
                value_list.push_back(getIntValue(d));
            }
            std::sort(value_list.begin(), value_list.end());
            return BuildData(Int, value_list[2]);
        };
    InfoEnd(ThirdMin, "3rd-min")

    InfoStart(MaxPrefixSum, "mps")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.empty()) throw SemanticsError();
            int res = getIntValue(x->value[0]), sum = res;
            for (int i = 1; i < x->value.size(); ++i) {
                sum += getIntValue(x->value[i]);
                res = std::max(res, sum);
            }
            return BuildData(Int, res);
        };
    InfoEnd(MaxPrefixSum, "mps")

    InfoStart(MaxSuffixSum, "mts")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[1]);
            if (x->value.empty()) throw SemanticsError();
            int last = int(x->value.size()) - 1;
            int res = getIntValue(x->value[last]), sum = res;
            for (int i = last - 1; i >= 0; --i) {
                sum += getIntValue(x->value[i]);
                res = std::max(res, sum);
            }
            return BuildData(Int, res);
        };
    InfoEnd(MaxSuffixSum, "mts")

    InfoStart(MaxSegmentSum, "mss")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.empty()) throw SemanticsError();
            int res = getIntValue(x->value[0]), mts = res;
            for (int i = 1; i < x->value.size(); ++i) {
                mts = std::max(0, mts) + getIntValue(x->value[i]);
                res = std::max(res, mts);
            }
            return BuildData(Int, res);
        };
    InfoEnd(MaxSegmentSum, "mss")

    InfoStart(MaxPrefixSumPos, "mps-pos")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.empty()) throw SemanticsError();
            int res = getIntValue(x->value[0]), sum = res, pos = 0;
            for (int i = 1; i < x->value.size(); ++i) {
                sum += getIntValue(x->value[i]);
                if (sum > res) res = sum, pos = i;
            }
            return BuildData(Int, pos);
        };
        env->setConst(solver::autolifter::KComposedNumName, BuildData(Int, 3));
    InfoEnd(MaxPrefixSumPos, "mps-pos")

    InfoStart(MaxSuffixSumPos, "mts-pos")
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            if (x->value.empty()) throw SemanticsError();
            int last = int(x->value.size()) - 1;
            int sum = getIntValue(x->value[last]), res = sum, pos = last;
            for (int i = last - 1; i >= 0; --i) {
                sum += getIntValue(x->value[i]);
                if (sum > res) res = sum, pos = i;
            }
            return BuildData(Int, pos);
        };
        env->setConst(solver::autolifter::KComposedNumName, BuildData(Int, 3));
    InfoEnd(MaxSuffixSumPos, "mts-pos")

#define RegisterName(task_name, func_name) if (name == task_name) return _getConfigInfo ## func_name()
    dsl::autolifter::LiftingConfigInfo _getConfigInfo(const std::string& name) {
        RegisterName("sum", Sum);
        RegisterName("min", Min);
        RegisterName("max", Max);
        RegisterName("length", Length);
        RegisterName("2nd-min", SecondMin);
        RegisterName("3rd-min", ThirdMin);
        RegisterName("mps", MaxPrefixSum);
        RegisterName("mts", MaxSuffixSum);
        RegisterName("mss", MaxSegmentSum);
        RegisterName("mps-pos", MaxPrefixSumPos);
        RegisterName("mts-pos", MaxSuffixSumPos);
        LOG(FATAL) << "AutoLifter: Unknown task " << name;
    }
}

LiftingTask * dsl::autolifter::track::getDaDLiftingTask(const std::string &name) {
    dsl::autolifter::LiftingConfigInfo info = _getConfigInfo(name);
    return buildLiftingTask(info);
}