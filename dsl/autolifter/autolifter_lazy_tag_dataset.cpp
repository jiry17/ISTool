//
// Created by pro on 2022/3/7.
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
#define TINT theory::clia::getTInt()
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

    LiftingModConfigInfo _getModConfigInfoTag(Env* env, const PType& tag_type, const PSemantics& mod_sem) {
        auto F = std::make_shared<TProduct>((TypeList){tag_type, TVARA});
        auto x = program::buildParam(0, std::make_shared<TProduct>((TypeList){tag_type, TLISTINT}));
        auto m = std::make_shared<Program>(mod_sem, (ProgramList){_buildAccess(x, 0), _buildAccess(x, 1)});
        return{m, F};
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
        return {_buildListProgram(ps, str_name), inp_type, env, {lazy_tag_config, dad_mod_config}}; \
    }

    PProgram _buildListProgram(const FullSemanticsFunction& ps, const std::string& name) {
        auto sem = std::make_shared<TypedAnonymousSemantics>(ps, (TypeList){TLISTINT}, theory::clia::getTInt(), name);
        auto p = std::make_shared<Program>(sem, (ProgramList){program::buildParam(0, TLISTINT)});
        return p;
    }

    InfoStart(SumPlus, "sum@+")
        auto plus_f = [](DataList&& inp_list, ExecuteInfo* info) -> Data{
            int x = getIntValue(inp_list[0]);
            auto* y = _getListValue(inp_list[1]);
            DataList res;
            for (auto& d: y->value) {
                res.push_back(BuildData(Int, getIntValue(d) + x));
            }
            return BuildData(List, res);
        };
        auto lazy_tag_config = _getModConfigInfoTag(env.get(), _getLimitedInt(type_config), std::make_shared<AnonymousSemantics>(plus_f, "plus"));
        lazy_tag_config.extra_semantics.push_back(env->getSemantics("*"));
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) -> Data {
            auto* x = _getListValue(inp_list[0]);
            int sum = 0;
            for (auto& d: x->value) sum += getIntValue(d);
            return BuildData(Int, sum);
        };
    InfoEnd(SumPlus, "sum@+")

    InfoStart(SumNeg, "sum@neg")
        auto neg_f = [](DataList&& inp_list, ExecuteInfo* info) ->Data {
            if (!inp_list[0].isTrue()) return inp_list[1];
            auto* x = _getListValue(inp_list[1]);
            DataList res;
            for (auto& d: x->value) res.push_back(BuildData(Int, -getIntValue(d)));
            return BuildData(List, res);
        };
        auto lazy_tag_config = _getModConfigInfoTag(env.get(), type::getTBool(), std::make_shared<AnonymousSemantics>(neg_f, "neg-all"));
        auto ps = [](DataList&& inp_list, ExecuteInfo* info) -> Data {
            auto* x = _getListValue(inp_list[0]);
            int sum = 0;
            for (auto& d: x->value) sum += getIntValue(d);
            return BuildData(Int, sum);
        };
    InfoEnd(SumNeg, "sum@neg")

#define RegisterName(task_name, func_name) if (name == task_name) return _getConfigInfo ## func_name()
    dsl::autolifter::LiftingConfigInfo _getConfigInfo(const std::string& name) {
        RegisterName("sum@+", SumPlus);
        RegisterName("sum@neg", SumNeg);
        LOG(FATAL) << "AutoLifter: Unknown task " << name;
    }
}

LiftingTask * dsl::autolifter::track::getLazyTagLiftingTask(const std::string &name) {
    auto info = _getConfigInfo(name);
    return buildLiftingTask(info);
}