//
// Created by pro on 2022/1/7.
//

#include "istool/solver/polygen/dnf_learner.h"
#include "istool/solver/enum/enum_util.h"
#include "istool/sygus/theory/basic/clia/clia_value.h"
#include "glog/logging.h"
#include <algorithm>

using namespace polygen;

CmpInfo::CmpInfo(const PProgram& _cmp, const Bitset &_P, const Bitset &_N): cmp(_cmp), P(_P), N(_N) {
}
std::string CmpInfo::toString() const {
    return cmp->toString() + " P:" + P.toString() + " N:" + N.toString();
}
CmpPlan::CmpPlan(const std::vector<PCmpInfo>& _cmp_list, const Bitset &_rem): cmp_list(_cmp_list), rem_example(_rem) {
}
void CmpPlan::print() const {
    std::cout << "plan with example:" << rem_example.toString() << std::endl;
    for (const auto& info: cmp_list) {
        std::cout << "  " << info->toString() << std::endl;
    }
}

namespace {
    const std::set<std::string> ignored_set = {
            "||", "&&", "!", "and", "not", "or"
    };

    PSynthInfo _buildPredInfo(const PSynthInfo& info) {
        auto* g = grammar::copyGrammar(info->grammar);
        int now = 0;
        for (auto* rule: g->start->rule_list) {
            auto name = rule->semantics->getName();
            if (ignored_set.find(name) != ignored_set.end()) {
                delete rule;
            } else g->start->rule_list[now++] = rule;
        }
        g->start->rule_list.resize(now);
        g->removeUseless();
        return std::make_shared<SynthInfo>(info->name, info->inp_type_list, info->oup_type, g);
    }

    int KDefaultMaxClauseNum = 5;

    int _getDefaultClauseNum(const PSynthInfo& info) {
        if (info->inp_type_list.size() >= 10) return 2;
        return KDefaultMaxClauseNum;
    }
}

DNFLearner::DNFLearner(Specification *spec): PBESolver(spec) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "DNFLearner can only synthesize a single program";
    }
    auto dnf_info = spec->info_list[0];
    auto bool_type = type::getTBool();
    if (!bool_type->equal(dnf_info->oup_type.get())) {
        LOG(FATAL) << "DNFLearner: the output type must be Boolean";
    }
    io_space = dynamic_cast<IOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "DNFLeaner supports only IOExamples";
    }
    pred_info = _buildPredInfo(spec->info_list[0]);

    auto* val = spec->env->getConstRef(solver::polygen::KMaxClauseNumName);
    if (val->isNull()) KMaxClauseNum = _getDefaultClauseNum(spec->info_list[0]);
    else KMaxClauseNum = theory::clia::getIntValue(*val);
    KRelaxTimeLimit = 0.1;
}

PCmpInfo DNFLearner::buildInfo(const PProgram& program) {
    Bitset P, N;
    for (auto& example: positive_list) P.append(program::run(program.get(), example).isTrue());
    for (auto& example: negative_list) N.append(program::run(program.get(), example).isTrue());
    return std::make_shared<CmpInfo>(program, P, N);
}

void DNFLearner::relax() {
    size_limit += 1;
    std::vector<FunctionContext> result;
    double time_limit = KRelaxTimeLimit;
    if (guard) time_limit = std::min(time_limit, guard->getRemainTime());
    auto* tmp_guard = new TimeGuard(time_limit);
    auto is_timeout = solver::collectAccordingSize({pred_info}, size_limit, result, EnumConfig(nullptr, nullptr, tmp_guard));
    if (is_timeout) KRelaxTimeLimit *= 2;
    delete tmp_guard;

    ProgramList pred_list;
    for (const auto& res: result) pred_list.push_back(res.begin()->second);
    pred_pool.push_back(pred_list);
}

namespace {
    bool _isSolvable(const std::vector<PCmpInfo>& cmp_list) {
        if (cmp_list.empty()) return false;
        int p_n = cmp_list[0]->P.size(), n_n = cmp_list[0]->N.size();
        for (int i = 0; i < p_n; ++i) {
            Bitset N(n_n, true);
            for (const auto& cmp_info: cmp_list) {
                if (cmp_info->P[i]) N = N & cmp_info->N;
            }
            if (N.count()) return false;
        }
        return true;
    }
}

std::vector<PCmpInfo> DNFLearner::getNextInfoList() {
    ++pred_pos;
    if (pred_pos == pred_pool.size()) relax();
    std::vector<PCmpInfo> pred_list;
    std::vector<bool> is_duplicated;
    int n = pred_pool[pred_pos].size();
    for (const auto& res: pred_pool[pred_pos]) {
        auto info = buildInfo(res);
        pred_list.push_back(info);
        is_duplicated.push_back(false);
    }
    /*LOG(INFO) << "Get next info";
    for (int i = 0; i < 10 && i < pred_list.size(); ++i) {
        std::cout << "  " << pred_list[i]->toString() << std::endl;
    }*/
    for (int i = n - 1; i >= 0; --i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && !is_duplicated[j] && pred_list[j]->P.checkCover(pred_list[i]->P) &&
                pred_list[i]->N.checkCover(pred_list[j]->N)) {
                is_duplicated[i] = true; break;
            }
        }
    }
    int now = 0;
    for (int i = 0; i < n; ++i) {
        if (!is_duplicated[i]) pred_list[now++] = pred_list[i];
    }
    pred_list.resize(now);

    if (!_isSolvable(pred_list)) return {};
    std::string feature;
    for (const auto& info: pred_list) {
        feature += info->cmp->toString();
    }
    if (pred_list_set.find(feature) != pred_list_set.end()) return {};
    pred_list_set.insert(feature);
    return pred_list;
}

void DNFLearner::clear() {
    negative_list.clear(); positive_list.clear();
    pred_pos = -1; pred_list_set.clear();
}

DNFLearner::~DNFLearner() {
    clear();
}

namespace {
    PProgram _buildClause(const ProgramList& program_list, Env* env) {
        ProgramList sub_list;
        if (program_list.empty()) {
            return std::make_shared<Program>(semantics::buildConstSemantics(BuildData(Bool, true)), sub_list);
        }
        auto res = program_list[0]; auto or_sem = env->getSemantics("and");
        for (int i = 1; i < program_list.size(); ++i) {
            sub_list = {res, program_list[i]};
            res = std::make_shared<Program>(or_sem, sub_list);
        }
        return res;
    }

    PProgram _buildDNF(const ProgramList& program_list, Env* env) {
        ProgramList sub_list;
        if (program_list.empty()) {
            return std::make_shared<Program>(semantics::buildConstSemantics(BuildData(Bool, false)), sub_list);
        }
        auto res = program_list[0]; auto or_sem = env->getSemantics("or");
        for (int i = 1; i < program_list.size(); ++i) {
            sub_list = {res, program_list[i]};
            res = std::make_shared<Program>(or_sem, sub_list);
        }
        return res;
    }
}

int CmpPlan::operator<(const CmpPlan &plan) const {
    return cmp_list.size() < plan.cmp_list.size() || (cmp_list.size() == plan.cmp_list.size() && rem_example < plan.rem_example);
}
ClausePlan::ClausePlan(const std::vector<int> &_list, const Bitset &_P, const Bitset &_N): cmp_id_list(_list), P(_P), N(_N) {
}
bool ClausePlan::cover(const ClausePlan &plan) const {
    return P.checkCover(plan.P) && plan.N.checkCover(N);
}

namespace {
    std::vector<ClausePlan> _reduceClause(const std::vector<ClausePlan>& plan_list) {
        std::vector<std::vector<int>> feature_list;
        for (int i = 0; i < plan_list.size(); ++i) {
            auto& plan = plan_list[i];
            feature_list.push_back({-plan.P.count(), plan.N.count(), -int(plan.cmp_id_list.size()), i});
        }
        std::sort(feature_list.begin(), feature_list.end());
        std::vector<ClausePlan> res;
        for (auto& feature: feature_list) {
            auto& plan = plan_list[feature[3]];
            bool is_duplicated = false;
            for (auto& pre: res) {
                if (pre.cover(plan)) {
                    is_duplicated = true; break;
                }
            }
            if (!is_duplicated) res.push_back(plan);
        }
        return res;
    }

    ClausePlan _mergePlan(const ClausePlan& x, const ClausePlan& y, const Bitset& P, const Bitset& N) {
        std::vector<int> id_list = x.cmp_id_list;
        for (auto id: y.cmp_id_list) id_list.push_back(id);
        return {id_list, P, N};
    }
}

// TODO: replace this part by foldl
std::vector<polygen::ClausePlan> DNFLearner::getAllClause(int l, int r, const std::vector<polygen::PCmpInfo> &info_list,
        const Bitset &rem_example, int lim) {
    std::vector<polygen::ClausePlan> result;
    TimeCheck(guard);
    if (l == r) {
        const auto& info = info_list[l];
        result.push_back(ClausePlan({}, rem_example, Bitset(info->N.size(), true)));
        auto rem_P = info->P & rem_example;
        if (rem_P.count() >= lim) {
            result.push_back(ClausePlan({l}, rem_P, info->N));
        }
        return _reduceClause(result);
    }
    int mid = l + r >> 1;
    auto l_result = getAllClause(l, mid, info_list, rem_example, lim);
    auto r_result = getAllClause(mid + 1, r, info_list, rem_example, lim);
    for (const auto& l_plan: l_result) {
        for (const auto& r_plan: r_result) {
            TimeCheck(guard);
            auto rem_P = l_plan.P & r_plan.P;
            if (rem_P.count() >= lim) {
                auto rem_N = l_plan.N & r_plan.N;
                if (l == 0 && r + 1 == info_list.size() && rem_N.count()) continue;
                result.push_back(_mergePlan(l_plan, r_plan, rem_P, rem_N));
            }
        }
    }
    return _reduceClause(result);
}

PCmpInfo DNFLearner::buildSimplifiedInfo(const polygen::ClausePlan &plan, const std::vector<polygen::PCmpInfo> &info_list) {
    std::vector<PCmpInfo> used_info_list, result_info_list;
    for (int id: plan.cmp_id_list) {
        used_info_list.push_back(info_list[id]);
    }
    int n_n = plan.N.size(), p_n = plan.P.size();
    Bitset rem_n(n_n, true);
    while (rem_n.count()) {
        PCmpInfo best; int best_cover_num = -1;
        for (const auto& info: used_info_list) {
            int num = (rem_n.count() - (info->N & rem_n).count());
            if (num > best_cover_num) {
                best_cover_num = num; best = info;
            }
        }
        result_info_list.push_back(best);
        rem_n = rem_n & best->N;
    }
    Bitset now_p = Bitset(p_n, true);
    ProgramList program_list;
    for (const auto& info: result_info_list) {
        program_list.push_back(info->cmp);
        now_p = now_p & info->P;
    }
    return std::make_shared<CmpInfo>(_buildClause(program_list, spec->env.get()), now_p, rem_n);
}

namespace {
    bool _compareCmp(const PCmpInfo& x, const PCmpInfo& y) {
        return x->cmp->toString() < y->cmp->toString();
    }

    std::vector<PCmpInfo> _insertNewCmp(const CmpPlan& plan, const PCmpInfo& info) {
        std::vector<PCmpInfo> cmp_list = plan.cmp_list;
        cmp_list.emplace_back(); int pos = 0;
        while (pos + 1 < cmp_list.size() && _compareCmp(cmp_list[pos], info)) ++pos;
        for (int i = int(cmp_list.size()) - 1; i > pos; --i) cmp_list[i] = cmp_list[i - 1];
        cmp_list[pos] = info;
        return cmp_list;
    }
}

PProgram DNFLearner::searchForCondition(const polygen::CmpPlan &plan, const std::vector<PCmpInfo> &info_list, int rem_num) {
    if (visited_plan.find(plan) != visited_plan.end()) return nullptr;
    TimeCheck(guard); visited_plan.insert(plan);
    if (rem_num == 0 || plan.rem_example.count() == 0) {
        ProgramList clause_list;
        for (const auto& info: plan.cmp_list) clause_list.push_back(info->cmp);
        return _buildDNF(clause_list, spec->env.get());
    }
    int rem_example_num = plan.rem_example.count();
    int limit = (rem_example_num - 1) / rem_num + 1;
    std::vector<PCmpInfo> clause_list;
    // plan.print();
    for (const ClausePlan& info: getAllClause(0, int(info_list.size()) - 1, info_list, plan.rem_example, limit)) {
        if (info.N.count()) continue;
        auto result = buildSimplifiedInfo(info, info_list);
        clause_list.push_back(result);
    }
    for (const auto& cmp: clause_list) {
        auto cmp_list = _insertNewCmp(plan, cmp);
        auto res = searchForCondition({cmp_list, plan.rem_example & (~cmp->P)}, info_list, rem_num - 1);
        if (res) return res;
    }
    return nullptr;
}

PProgram DNFLearner::searchForCondition(const std::vector<PCmpInfo> &cmp_info, int rem_num) {
    int p_n = cmp_info[0]->P.size(); Bitset full_positive(p_n, true);
    CmpPlan empty_plan({}, full_positive);
    visited_plan.clear();
    return searchForCondition(empty_plan, cmp_info, rem_num);
}

FunctionContext DNFLearner::synthesis(const std::vector<Example> &example_list, TimeGuard *_guard) {
    clear(); guard = _guard;
    for (auto& example: example_list) {
        auto io_example = io_space->getIOExample(example);
        if (io_example.second.isTrue()) positive_list.push_back(io_example.first);
        else negative_list.push_back(io_example.first);
    }

    std::set<std::pair<int, int>> visited_set;
    int or_limit = 1;
    while (1) {
        auto info_list = getNextInfoList();
        if (!info_list.empty()) {
            info_storage.push_back(info_list);
        }
        for (int clause_num = 1; clause_num <= or_limit; ++clause_num) {
            for (int si = 0; si < info_storage.size(); ++si) {
                if (visited_set.find({clause_num, si}) != visited_set.end()) continue;
                TimeCheck(guard);
                visited_set.insert({clause_num, si});
                auto condition = searchForCondition(info_storage[si], clause_num);
                if (condition) {
                    return semantics::buildSingleContext(io_space->func_name, condition);
                }
            }
        }
        or_limit = std::min(or_limit + 1, KMaxClauseNum);
    }
}
