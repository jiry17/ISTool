//
// Created by pro on 2022/2/23.
//

#include "istool/dsl/autolifter/autolifter_dataset.h"
#include "istool/dsl/autolifter/autolifter_dsl.h"
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

namespace {
    struct _TypeConfig {
        int int_min = -3, int_max = 3;
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

    LiftingModConfigInfo _getModConfigInfoDaD(Env* env, const _TypeConfig& config) {
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

#define InfoStart(name) \
    LiftingConfigInfo _getConfigInfo ## name() { \
        auto env = std::make_shared<Env>(); \
        dsl::autolifter::prepareEnv(env.get()); \
        _TypeConfig type_config;
#define InfoEnd(name) }

    InfoStart(Sum)
        auto dad_mod_config = _getModConfigInfoDaD(env.get(), type_config);
        auto inp_type = _getLimitedListInt(type_config);

        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            int res = 0;
            for (int i = 0; i < x->value.size(); ++i) {
                res += theory::clia::getIntValue(x->value[i]);
            }
            return BuildData(Int, res);
        };
        auto sem = std::make_shared<TypedAnonymousSemantics>(ps, (TypeList){TLISTINT}, theory::clia::getTInt(), "sum");
        auto p = std::make_shared<Program>(sem, (ProgramList){program::buildParam(0, TLISTINT)});
        return {p, inp_type, env, {dad_mod_config}};
    InfoEnd(Sum)

    InfoStart(MaxPrefixSum)
        auto dad_mod_config = _getModConfigInfoDaD(env.get(), type_config);
        auto inp_type = _getLimitedListInt(type_config);

        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            int res = 0, sum = 0;
            for (int i = 0; i < x->value.size(); ++i) {
                sum += theory::clia::getIntValue(x->value[i]);
                res = std::max(res, sum);
            }
            return BuildData(Int, res);
        };
        auto sem = std::make_shared<TypedAnonymousSemantics>(ps, (TypeList){TLISTINT}, theory::clia::getTInt(), "mps");
        auto p = std::make_shared<Program>(sem, (ProgramList){program::buildParam(0, TLISTINT)});
        return {p, inp_type, env, {dad_mod_config}};
    InfoEnd(MaxPrefixSum)

    InfoStart(MaxSegmentSum)
        auto dad_mod_config = _getModConfigInfoDaD(env.get(), type_config);
        auto inp_type = _getLimitedListInt(type_config);

        auto ps = [](DataList&& inp_list, ExecuteInfo* info) {
            auto* x = _getListValue(inp_list[0]);
            int res = 0, mts = 0;
            for (int i = 0; i < x->value.size(); ++i) {
                mts = std::max(0, mts + theory::clia::getIntValue(x->value[i]));
                res = std::max(res, mts);
            }
            return BuildData(Int, res);
        };
        auto sem = std::make_shared<TypedAnonymousSemantics>(ps, (TypeList){TLISTINT}, theory::clia::getTInt(), "mss");
        auto p = std::make_shared<Program>(sem, (ProgramList){program::buildParam(0, TLISTINT)});
        return {p, inp_type, env, {dad_mod_config}};
    InfoEnd(MaxSegmentSum)

#define RegisterName(task_name, func_name) if (name == task_name) return _getConfigInfo ## func_name()
    dsl::autolifter::LiftingConfigInfo _getConfigInfo(const std::string& name) {
        RegisterName("dad@sum", Sum);
        RegisterName("dad@mps", MaxPrefixSum);
        RegisterName("dad@mss", MaxSegmentSum);
        LOG(FATAL) << "AutoLifter: Unknown task " << name;
    }
}

LiftingTask * dsl::autolifter::getLiftingTask(const std::string &name) {
    dsl::autolifter::LiftingConfigInfo info = _getConfigInfo(name);
    return buildLiftingTask(info);
}