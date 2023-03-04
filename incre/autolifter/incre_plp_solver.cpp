//
// Created by pro on 2022/9/26.
//

#include "istool/incre/autolifter/incre_plp_solver.h"
#include "glog/logging.h"
#include "istool/incre/trans/incre_trans.h"
#include <iostream>

using namespace incre::autolifter;
using solver::autolifter::MaximalInfoList;
using solver::autolifter::EnumerateInfo;


UnitInfo::UnitInfo(const AuxProgram &_program, const Bitset &_info): program(_program), info(_info) {
}


namespace {
    bool _evaluate(std::pair<int, int> example, FExampleSpace* example_space, const AuxProgram& program) {
        auto [l_id, r_id] = example;
        return !(example_space->runAux(l_id, program) == example_space->runAux(r_id, program));
    }
}

namespace {
    int KDefaultComposedNum = 4;
    int KDefaultExtraTurnNum = 0;
    int KDefaultVerifyBaseNum = 300;
    int KDefaultExampleTimeOut = 10;
    int KDefaultEnlargeFactor = 2;
}

IncrePLPSolver::IncrePLPSolver(Env *_env, PLPTask *_task): env(_env), task(_task) {
    auto* d = env->getConstRef(solver::autolifter::KComposedNumName, BuildData(Int, KDefaultComposedNum));
    KComposedNum = theory::clia::getIntValue(*d);
    d = env->getConstRef(solver::autolifter::KExtraTurnNumName, BuildData(Int, KDefaultExtraTurnNum));
    KExtraTurnNum = theory::clia::getIntValue(*d);
    d = env->getConstRef(solver::autolifter::KOccamExampleNumName, BuildData(Int, KDefaultVerifyBaseNum));
    KVerifyBaseNum = theory::clia::getIntValue(*d);
    KExampleTimeOut = KDefaultExampleTimeOut;
    KExampleEnlargeFactor = KDefaultEnlargeFactor;
}

namespace {
    std::vector<UnitInfo> _randomMerge(const std::vector<std::vector<UnitInfo>>& info_storage, Env* env) {
        std::vector<UnitInfo> res;
        std::vector<int> pos_list(info_storage.size(), 0);
        std::vector<int> source_list;
        for (int i = 0; i < info_storage.size(); ++i) {
            for (int j = 0; j < info_storage[i].size(); ++j) source_list.push_back(i);
        }
        std::shuffle(source_list.begin(), source_list.end(), env->random_engine);
        for (auto i: source_list) {
            res.push_back(info_storage[i][pos_list[i]++]);
        }
        return res;
    }
}

UnitInfo IncrePLPSolver::init(const AuxProgram& program) {
    Bitset info(example_list.size(), false);
    for (int i = 0; i < example_list.size(); ++i) {
        if (_evaluate(example_list[i], task->example_space, program)) info.set(i, true);
    }
    return {program, info};
}
std::string IncrePLPSolver::example2String(const std::pair<int, int> &example) {
    auto l_string = task->example_space->example2String(example.first);
    auto r_string = task->example_space->example2String(example.second);
    return "(" + l_string + ", " + r_string + ")";
}
void IncrePLPSolver::addExample(const std::pair<int, int> &example) {
    for (auto& unit: component_info_list) {
        unit.info.append(_evaluate(example, task->example_space, unit.program));
    }
    example_list.push_back(example);

    // clear tmp data structures in the previous turn
    for (auto& info_list: info_storage) {
        for (auto* info: info_list) delete info;
    }
    info_storage.clear(); next_component_id = 0; maximal_list.clear();
    for (auto& q: working_list) {
        while (!q.empty()) {
            delete q.front(); q.pop();
        }
    }
    uncovered_info_set.clear(); global_maximal.clear();
}

std::vector<UnitInfo> IncrePLPSolver::mergeUnits(int compress_size, int aux_size) {
    auto* compress_list = task->compress_grammar->acquirePrograms(compress_size);
    if (!compress_list) return {};

    std::vector<TypedProgramList> known_aux_list(task->aux_grammar_list.size());
    std::vector<TypedProgramList*> aux_pointer_list(task->aux_grammar_list.size());
    if (aux_size == 0) {
        for (int i = 0; i < known_aux_list.size(); ++i) {
            for (auto& f_res: task->pre_res_list[i]) {
                known_aux_list[i].push_back(f_res);
            }
            aux_pointer_list[i] = &known_aux_list[i];
        }
    } else {
        for (int i = 0; i < known_aux_list.size(); ++i) {
            aux_pointer_list[i] = task->aux_grammar_list[i]->acquirePrograms(aux_size);
        }
    }

    /*for (auto* aux: aux_pointer_list) {
        if (aux) std::cout << aux->size(); else std::cout << "null";
        std::cout << " ";
    }
    std::cout << std::endl;*/

    std::vector<UnitInfo> res_list;
    for (auto compress_it = compress_list->begin(); compress_it < compress_list->end(); ++compress_it) {
        auto program = *compress_it;
        auto* ltc = dynamic_cast<TLabeledCompress*>(program.first.get());
        if (!ltc) {
            if (aux_size == 0) {
                res_list.push_back(init({program, {nullptr, nullptr}}));
            }
        } else {
            int compress_id = ltc->id; auto* aux_list = aux_pointer_list[compress_id];
            if (!aux_list) continue;
            for (auto aux_it = aux_list->begin(); aux_it < aux_list->end(); ++aux_it) {
                res_list.push_back(init({program, *aux_it}));
            }
        }
    }

    return res_list;
}

void IncrePLPSolver::getMoreComponent() {

    ++current_size;
    LOG(INFO) << "get more component";

    std::vector<std::vector<UnitInfo>> unit_storage;
    for (int compress_size = 0; compress_size <= current_size; ++compress_size) {
        auto merge_result = mergeUnits(compress_size, current_size - compress_size);
        /*LOG(INFO) << "merge " << compress_size << " " << current_size - compress_size << std::endl;
        for (auto& program: merge_result) {
            LOG(INFO) << "  " << aux2String(program.program);
        }*/
        unit_storage.push_back(merge_result);
    }

    for (auto& unit: _randomMerge(unit_storage, env)) {
        component_info_list.push_back(unit);
    }
}

namespace {
    std::vector<std::pair<int, PProgram>> _merge(const std::vector<std::pair<int, TypedProgram>>& x_list,
            std::vector<std::pair<int, TypedProgram>>& y_list) {
        std::vector<std::pair<int, PProgram>> res;
        for (auto& [id, program]: x_list) res.emplace_back(id, program.second);
        for (auto& [id, program]: y_list) res.emplace_back(id, program.second);
        return res;
    }

    std::string _unitList2String(const std::vector<AuxProgram>& unit_list) {
        std::string res = "[";
        for (int i = 0; i < unit_list.size(); ++i) {
            if (i) res += ",";
            res += aux2String(unit_list[i]);
        }
        return res + "]";
    }
}

std::pair<int, int> IncrePLPSolver::verify(const std::vector<AuxProgram> &aux_list) {
    int total_size = 1;
    for (auto& [p_compress, p_aux]: aux_list) {
        total_size += p_compress.second->size();
        if (p_aux.second) total_size += p_aux.second->size();
    }
    verify_num = std::max(verify_num, total_size * KVerifyBaseNum);
    verify_num = task->acquireExample(verify_num, KExampleTimeOut);

    std::unordered_map<std::string, std::pair<Data, int>> verify_cache;

    auto deal_example = [&](int example_id) {
        auto io_example = task->getIO(example_id, aux_list);
        auto feature = data::dataList2String(io_example.first);
        if (verify_cache.find(feature) == verify_cache.end()) {
            verify_cache[feature] = {io_example.second, example_id};
            return -1;
        } else {
            auto& [pre_oup, pre_id] = verify_cache[feature];
            if (pre_oup == io_example.second) return -1;
            return pre_id;
        }
    };

    for (int _ = 0; _ < verify_num; ++_) {
        verify_pos = (verify_pos + 1) % verify_num;
        auto pre_id = deal_example(verify_pos);
        if (pre_id >= 0) return {pre_id, verify_pos};
    }
    int pre_verify_num = verify_num;
    verify_num = verify_num * KExampleEnlargeFactor;
    verify_num = task->acquireExample(verify_num, KExampleTimeOut);
    for (verify_pos = pre_verify_num; verify_pos < verify_num; ++verify_pos) {
        auto pre_id = deal_example(verify_pos);
        if (pre_id >= 0) return {pre_id, verify_pos};
    }
    return {-1, -1};
}

std::vector<AuxProgram> IncrePLPSolver::extractResultFromInfo(solver::autolifter::EnumerateInfo *info) {
    if (!info) return {};
    std::vector<AuxProgram> res;

    for (auto id: info->ind_list) {
        auto& unit = component_info_list[id];
        res.push_back(unit.program);
    }
    return res;
}

namespace {
    std::string _indList2String(const std::vector<int>& ind_list) {
        std::string res = "[";
        for (int i = 0; i < ind_list.size(); ++i) {
            if (i) res += ","; res += std::to_string(ind_list[i]);
        }
        return res + "]";
    }
}

bool IncrePLPSolver::addUncoveredInfo(solver::autolifter::EnumerateInfo *info) {
    int pos = int(info->ind_list.size()) - 1;
    if (info->ind_list.size() > 1) {
        EnumerateInfo* last = nullptr;
        for (int i = 0; i < info->ind_list.size(); ++i) {
            std::vector<int> sub_ind_list;
            for (int j = 0; j < info->ind_list.size(); ++j) {
                if (i != j) sub_ind_list.push_back(info->ind_list[j]);
            }
            auto feature = _indList2String(sub_ind_list);
            auto it = uncovered_info_set.find(feature);
            if (it == uncovered_info_set.end()) return false;
            last = it->second;
        }
        int last_component = info->ind_list[pos];
        info->info = last->info | component_info_list[last_component].info;
    } else {
        info->info = component_info_list[info->ind_list[0]].info;
    }

    if (!maximal_list[pos].add(info)) return false;
    uncovered_info_set[_indList2String(info->ind_list)] = info;
    info_storage[pos].push_back(info);
    global_maximal.add(info);
    return true;
}

void IncrePLPSolver::constructInfo(solver::autolifter::EnumerateInfo *info) {
    int pos = info->ind_list.size();
    while (working_list.size() <= pos) working_list.emplace_back();
    for (auto* component_info: info_storage[0]) {
        if (component_info->ind_list[0] >= info->ind_list[0]) continue;
        std::vector<int> ind_list = component_info->ind_list;
        for (auto id: info->ind_list) ind_list.push_back(id);
        working_list[pos].push(new EnumerateInfo(ind_list));
    }
}

solver::autolifter::EnumerateInfo * IncrePLPSolver::getNextComponent(int k, TimeGuard *guard) {
    while (working_list.size() <= k) working_list.emplace_back();
    while (info_storage.size() <= k) info_storage.emplace_back();
    while (maximal_list.size() <= k) maximal_list.emplace_back();
    while (k && working_list[k].empty()) --k;
    if (k == 0) {
        while (next_component_id == component_info_list.size()) getMoreComponent();

        return new EnumerateInfo({next_component_id++});
    }
    auto* res = working_list[k].front(); working_list[k].pop();
    return res;
}

std::pair<solver::autolifter::EnumerateInfo *, solver::autolifter::EnumerateInfo *> IncrePLPSolver::recoverResult(int pos, solver::autolifter::EnumerateInfo *info) {
    for (auto* x: info_storage[pos]) {
        if ((x->info | info->info).count() == x->info.size()) return {x, info};
    }
    assert(false);
}
std::pair<solver::autolifter::EnumerateInfo *, solver::autolifter::EnumerateInfo *> IncrePLPSolver::constructResult(solver::autolifter::EnumerateInfo *info, int limit) {
    int rem = limit - int(info->ind_list.size());
    if (info->info.count() == example_list.size()) return {info, nullptr};
    if (!global_maximal.isExistResult(info)) return {nullptr, nullptr};
    for (int i = 0; i < rem && i < maximal_list.size(); ++i) {
        if (maximal_list[i].isExistResult(info)) {
            return recoverResult(i, info);
        }
    }
    return {nullptr, nullptr};
}

std::vector<AuxProgram> IncrePLPSolver::synthesisFromExample(TimeGuard* guard) {
    if (example_list.empty()) return {};
    std::vector<AuxProgram> best_result;
    int extra_turn_num = 0, current_limit = KComposedNum;

    for (int turn_id = 1;; ++turn_id) {
        TimeCheck(guard);
        if (!best_result.empty()) {
            ++extra_turn_num;
            if (extra_turn_num >= KExtraTurnNum || current_limit == 0) return best_result;
        }
        int k = (turn_id - 1) % ((current_limit + 1) / 2);
        auto* info = getNextComponent(k, guard);
        if (!addUncoveredInfo(info)) {
            delete info; continue;
        }
        auto res = constructResult(info, current_limit);
        if (res.first) {
            auto aux_list = extractResultFromInfo(res.first);
            for (const auto& aux: extractResultFromInfo(res.second)) {
                aux_list.push_back(aux);
            }
            best_result = aux_list;
            extra_turn_num = 0; current_limit = int(best_result.size()) - 1;
        }
        if (info->ind_list.size() < (current_limit + 1) / 2) constructInfo(info);
    }
}

PLPRes IncrePLPSolver::synthesis(TimeGuard *guard) {
    std::cout << std::endl << std::endl << std::endl;
    LOG(INFO) << "solve " << task->example_space->tau_id;
    for (auto* grammar: task->aux_grammar_list) {
        std::cout << "aux grammar" << std::endl;
        grammar->grammar->print();
        std::cout << std::endl;
    }
    std::cout << "compress grammar" << std::endl;
    task->compress_grammar->grammar->print();
    std::cout << std::endl;
    auto counter_example = verify({});
    if (counter_example.first == -1) return {};
    LOG(INFO) << "Counter example " << example2String(counter_example);
    addExample(counter_example);

    while (true) {
        TimeCheck(guard);
        auto candidate_result = synthesisFromExample(guard);
        LOG(INFO) << "Candidate result " << _unitList2String(candidate_result);

        counter_example = verify(candidate_result);
        if (counter_example.first == -1) return candidate_result;
        addExample(counter_example);
        LOG(INFO) << "Counter example " << example2String(counter_example);
        std::cout << task->runOup(counter_example.first).toString() << " " << task->runOup(counter_example.second).toString() << std::endl;
        for (auto unit: candidate_result) {
            std::cout << "  " << task->runInp(counter_example.first, unit).toString() << " " <<
              task->runInp(counter_example.second,unit).toString() << std::endl;
        }
    }
}