//
// Created by pro on 2022/1/24.
//

#include "istool/solver/autolifter/composed_sf_solver.h"
#include "istool/solver/enum/enum_util.h"
#include "istool/sygus/theory/basic/clia/clia_value.h"
#include "istool/ext/deepcoder/data_util.h"
#include "glog/logging.h"
#include <cassert>

namespace {
    int KDefaultComposedNum = 1;
    bool KDefaultFullH = false;
    int KDefaultExtraTurnNum = 100;

    std::string _indList2String(const std::vector<int>& ind_list) {
        std::string res = "{";
        for (int i = 0; i < ind_list.size(); ++i) {
            if (i) res += ","; res += std::to_string(ind_list[i]);
        }
        return res + "}";
    }
}

using namespace solver::autolifter;
EnumerateInfo::EnumerateInfo(const std::vector<int> &_ind_list):
    ind_list(_ind_list) {
}

MaximalInfoList::MaximalInfoList(): size(0), info_list(100) {}
void MaximalInfoList::clear() {size = 0;}
bool MaximalInfoList::add(EnumerateInfo *info) {
    size = 0;
    bool is_cover = false;
    for (auto* x: info_list) {
        if (!is_cover && x->info.checkCover(info->info)) return false;
        if (info->info.checkCover(x->info)) {
            is_cover = true;
        } else info_list[size++] = x;
    }
    if (size == info_list.size()) info_list.push_back(info);
    else info_list[size] = info;
    ++size;
    return true;
}
bool MaximalInfoList::isExistResult(EnumerateInfo *info) {
    for (auto* x: info_list) {
        if ((x->info | info->info).count() == info->info.size()) return true;
    }
    return false;
}

ComposedSFSolver::ComposedSFSolver(PartialLiftingTask *task): SFSolver(task) {
    KComposedNum = theory::clia::getIntValue(
            *(task->env->getConstRef(solver::autolifter::KComposedNumName, BuildData(Int, KDefaultComposedNum))));
    KIsFullH = task->env->getConstRef(solver::autolifter::KIsFullHName, BuildData(Bool, KDefaultFullH))->isTrue();
    KExtraTurnNum = theory::clia::getIntValue(
            *(task->env->getConstRef(solver::autolifter::KExtraTurnNumName, BuildData(Int, KDefaultExtraTurnNum))));
    KEnumerateTimeOut = 1;
    v = new SFVerifier(task, KIsFullH);
    env = task->env.get();
}
ComposedSFSolver::~ComposedSFSolver() {
    delete v;
}

bool ComposedSFSolver::isSatisfyExample(Program *p, const std::pair<int, int> &example) {
    return !(task->info->getFMapResult(p, example.first) == task->info->getFMapResult(p, example.second));
}

PProgram ComposedSFSolver::synthesisFromH() {
    ProgramList h_list = ext::ho::splitTriangle(task->h);
    ProgramList useful_list;
    while (1) {
        auto current = ext::ho::buildTriangle(useful_list);
        auto example = v->verify(current);
        if (example.first == -1) return current;
        bool is_solved = false;
        for (const auto& h_comp: h_list) {
            if (isSatisfyExample(h_comp.get(), example)) {
                is_solved = true;
                useful_list.push_back(h_comp);
                break;
            }
        }
        if (!is_solved) return nullptr;
    }
}

ProgramList ComposedSFSolver::getProgramListFromInfo(EnumerateInfo *info) {
    if (!info) return {};
    ProgramList res;
    for (auto ind: info->ind_list) {
        res.push_back(program_space[ind]);
    }
    return res;
}

void ComposedSFSolver::getMoreComponent(TimeGuard *guard) {
    int num = program_space.size();
    if (num == 0) num = 10000; else num *= 2;
    std::vector<FunctionContext> result;

    while (result.size() <= program_space.size()) {
        auto *tmp_o = new TrivialOptimizer();
        auto *tmp_v = new TrivialVerifier();
        double timeout = KEnumerateTimeOut;
        if (guard) timeout = std::min(timeout, guard->getRemainTime());
        auto *tmp_guard = new TimeGuard(timeout);
        EnumConfig c(tmp_v, tmp_o, tmp_guard);
        if (!solver::collectAccordingNum({task->f_info}, num, result, c)) {
            KEnumerateTimeOut *= 2;
        }
        delete tmp_o; delete tmp_v; delete tmp_guard;
    }

    for (int i = program_space.size(); i < result.size(); ++i) {
        auto p = result[i].begin()->second;
        program_space.push_back(p);
    }
}


void ComposedSFSolver::initNewProgram(TimeGuard *guard) {
    if (program_info_list.size() == program_space.size()) {
        getMoreComponent(guard);
    }
    int n = program_info_list.size();
    auto program = program_space[n].get();
    program_info_list.emplace_back();
    for (auto& example: example_list) {
        program_info_list[n].append(isSatisfyExample(program, example));
    }
}

EnumerateInfo* ComposedSFSolver::getNextComposition(int k, TimeGuard* guard) {
    while (working_list.size() <= k) working_list.emplace_back();
    while (info_storage.size() <= k) info_storage.emplace_back();
    while (maximal_list.size() <= k) maximal_list.emplace_back();
    while (k && working_list[k].empty()) --k;

    if (k == 0) {
        if (next_component_id == program_info_list.size()) {
            initNewProgram(guard);
        }
        return new EnumerateInfo({next_component_id});
    }
    auto* res = working_list[k].front(); working_list[k].pop();
    return res;
}

std::pair<solver::autolifter::EnumerateInfo *, solver::autolifter::EnumerateInfo *> ComposedSFSolver::recoverResult(
        int pos, solver::autolifter::EnumerateInfo *info) {
    for (auto* x: info_storage[pos]) {
        if ((x->info | info->info).count() == x->info.size()) return {x, info};
    }
    assert(false);
}

std::pair<EnumerateInfo *, EnumerateInfo*> ComposedSFSolver::constructResult(EnumerateInfo *info, int limit) {
    int rem = limit - int(info->ind_list.size());
    if (info->info.count() == example_list.size()) return {info, nullptr};
    if (!global_maximal.isExistResult(info)) return {nullptr, nullptr};
    for (int i = 0; i < rem; ++i) {
        if (maximal_list[i].isExistResult(info)) {
            return recoverResult(i, info);
        }
    }
    return {nullptr, nullptr};
}

bool ComposedSFSolver::addUncoveredInfo(solver::autolifter::EnumerateInfo *info) {
    if (info->ind_list.size() > 1) {
        EnumerateInfo* last_sub_info = nullptr;
        for (int i = 0; i < info->ind_list.size(); ++i) {
            std::vector<int> sub_ind_list;
            for (int j = 0; j < info->ind_list.size(); ++j) {
                if (i != j) sub_ind_list.push_back(info->ind_list[j]);
            }
            std::string feature = _indList2String(sub_ind_list);
            auto it = uncovered_info_set.find(feature);
            if (it == uncovered_info_set.end()) return false;
            last_sub_info = it->second;
        }
        info->info = last_sub_info->info | program_info_list[info->ind_list[0]];
    } else {
        info->info = program_info_list[info->ind_list[0]];
    }

    int ind = int(info->ind_list.size()) - 1;
    if (!maximal_list[ind].add(info)) return false;

    global_maximal.add(info);
    return true;
}

ProgramList ComposedSFSolver::synthesisFromExample(TimeGuard* guard) {
    if (example_list.empty()) return {};
    ProgramList best_result;
    int extra_turn_num = 0;
    int current_limit = KComposedNum;

    for (int turn_id = 1;; ++turn_id) {
        TimeCheck(guard);
        int k = (turn_id - 1) % ((current_limit + 1) / 2);
        if (!best_result.empty()) ++extra_turn_num;
        auto* info = getNextComposition(k, guard);

        if (!addUncoveredInfo(info)) continue;

        auto res = constructResult(info, current_limit);

        if (res.first) {
            ProgramList result = getProgramListFromInfo(res.first);
            for (const auto& p: getProgramListFromInfo(res.second)) {
                result.push_back(p);
            }
            best_result = result;
            extra_turn_num = 0;
            current_limit = int(best_result.size()) - 1;
        }
        if (extra_turn_num == KExtraTurnNum || current_limit == 0) {
            return best_result;
        }
    }
}

std::pair<PProgram, PProgram> ComposedSFSolver::synthesis(TimeGuard *guard) {
    auto empty = ext::ho::buildTriangle({});
    if (!KIsFullH) {
        PProgram useful_h = synthesisFromH();
        if (useful_h) return {useful_h, empty};
    }

    int turn_num = 0;
    while (true) {
        turn_num += 1;
        auto candidate_result = ext::ho::buildTriangle(synthesisFromExample(guard));
        auto example = v->verify(candidate_result);

        if (example.first != -1) addCounterExample(example);
        else if (KIsFullH) return {task->h, candidate_result};
        else return {empty, candidate_result};
    }
}


