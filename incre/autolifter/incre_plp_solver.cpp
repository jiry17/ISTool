//
// Created by pro on 2022/9/26.
//

#include "istool/incre/autolifter/incre_plp_solver.h"
#include "istool/solver/enum/enum_util.h"
#include "glog/logging.h"

using namespace incre::autolifter;
using solver::autolifter::MaximalInfoList;
using solver::autolifter::EnumerateInfo;

UnitInfo::UnitInfo(int _pos, const TypedProgram &_program, const Bitset &_info):
    pos(_pos), program(_program), info(_info) {
}

namespace {
    bool _evaluate(std::pair<int, int> example, FExampleSpace* example_space, int id, Program* program) {
        auto [l_id, r_id] = example;
        if (id < 0) {
            return !(example_space->runConst(l_id, program) == example_space->runConst(r_id, program));
        }
        auto l_res = example_space->runAux(l_id, id, program);
        auto r_res = example_space->runAux(r_id, id, program);
        return !(l_res == r_res);
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
    for (auto* f_grammar: task->f_grammar_list) {
        grammar_size_map[f_grammar] = grammar::getMaxSize(f_grammar);
    }
    if (task->target.second) {
        for (int i = 0; i < task->example_space->compress_infos.size(); ++i) {
            auto compress_id = task->example_space->compress_infos[i].first;
            if (compress_id == task->target_compress_id) {
                default_list.emplace_back(i, task->target);
            }
        }
    }
    grammar_size_map[task->const_grammar] = grammar::getMaxSize(task->const_grammar);

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

UnitInfo IncrePLPSolver::init(int pos, const TypedProgram &program) {
    Bitset info(example_list.size(), false);
    for (int i = 0; i < example_list.size(); ++i) {
        if (_evaluate(example_list[i], task->example_space, pos, program.second.get())) info.set(i, true);
    }
    return {pos, program, info};
}
std::string IncrePLPSolver::example2String(const std::pair<int, int> &example) {
    //LOG(INFO) << "example " << example.first << " " << example.second << " " << task->example_space->example_list.size();
    //LOG(INFO) << task->example_space->example_list[0].toString();
    return "(" + task->example_space->example_list[example.first].toString() + ", " + task->example_space->example_list[example.second].toString() + ")";
}
void IncrePLPSolver::addExample(const std::pair<int, int> &example) {
    for (auto& unit: component_info_list) {
        unit.info.append(_evaluate(example, task->example_space, unit.pos, unit.program.second.get()));
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

namespace {
    TypedProgram _extractTypedProgram(const PProgram& program) {
        auto* ts = dynamic_cast<TypeLabeledDirectSemantics*>(program->semantics.get());
        assert(ts && ts->type);
        return {ts->type, program->sub_list[0]};
    }
}
void IncrePLPSolver::getMoreComponent() {
    LOG(INFO) << "get more component";
    //task->const_grammar->print();
    //LOG(INFO) << grammar_size_map[task->const_grammar];
    ++current_size; bool is_extended = false;
    //LOG(INFO) << "Start get more component " << current_size;
    std::vector<std::vector<UnitInfo>> unit_storage;
    auto get_components = [&](int id, Grammar* grammar) {
        is_extended = true;
        auto info = std::make_shared<SynthInfo>("", TypeList(), PType(), grammar);
        std::vector<FunctionContext> res_list; EnumConfig c(nullptr, nullptr, nullptr);
        solver::collectAccordingSize({info}, current_size, res_list, c);
        std::vector<UnitInfo> info_list;
        for (auto& res: res_list) {
            auto p = res[""];
            if (p->size() == current_size) info_list.push_back(init(id, _extractTypedProgram(p)));
        }
        unit_storage.push_back(info_list);
    };
    for (int i = 0; i < task->f_grammar_list.size(); ++i) {
        auto* grammar = task->f_grammar_list[i];
        if (grammar_size_map[grammar] < current_size && grammar_size_map[grammar] >= 0) continue;
        get_components(i, grammar);
    }
    if (grammar_size_map[task->const_grammar] >= current_size || grammar_size_map[task->const_grammar] == -1) {
        get_components(-1, task->const_grammar);
    }
    if (!is_extended) {
        LOG(FATAL) << "All auxiliary values have been exhausted.";
    }
    for (auto& unit: _randomMerge(unit_storage, env)) {
        component_info_list.push_back(unit);
        // LOG(INFO) << "new component " << unit.pos << " " << unit.program->toString();
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

    std::string _unitList2String(const std::vector<std::pair<int, TypedProgram>>& unit_list) {
        std::string res = "[";
        for (int i = 0; i < unit_list.size(); ++i) {
            if (i) res += ",";
            res += std::to_string(unit_list[i].first) + "@" + unit_list[i].second.second->toString();
        }
        return res + "]";
    }
}

std::pair<int, int> IncrePLPSolver::verify(const std::vector<std::pair<int, TypedProgram> > &_aux_list) {
    std::vector<std::pair<int, PProgram>> aux_list = _merge(_aux_list, default_list);
    int total_size = 1;
    for (auto& [_, p]: aux_list) total_size += p->size();
    verify_num = std::max(verify_num, total_size * KVerifyBaseNum);
    verify_num = task->acquireExample(verify_num, KExampleTimeOut);
    // LOG(INFO) << "verify " << verify_num << " " << total_size;

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

std::vector<std::pair<int, TypedProgram> > IncrePLPSolver::extractResultFromInfo(solver::autolifter::EnumerateInfo *info) {
    if (!info) return {};
    std::vector<std::pair<int, TypedProgram>> res;
    for (auto id: info->ind_list) {
        auto& unit = component_info_list[id];
        res.emplace_back(unit.pos, unit.program);
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

std::vector<std::pair<int, TypedProgram> > IncrePLPSolver::synthesisFromExample(TimeGuard* guard) {
    if (example_list.empty()) return {};
    std::vector<std::pair<int, TypedProgram>> best_result;
    int extra_turn_num = 0, current_limit = KComposedNum;

    for (int turn_id = 1;; ++turn_id) {
        TimeCheck(guard);
        if (!best_result.empty()) {
            ++extra_turn_num;
            if (extra_turn_num >= KExtraTurnNum || current_limit == 0) return best_result;
        }
        int k = (turn_id - 1) % ((current_limit + 1) / 2);
        auto* info = getNextComponent(k, guard);
        /*if (info->ind_list.size() == 2) {
            std::vector<std::pair<int, PProgram>> unit_list;
            for (auto ind: info->ind_list) unit_list.emplace_back(component_info_list[ind].pos, component_info_list[ind].program);
            LOG(INFO) << "current enum " << _unitList2String(unit_list);
            int kk; std::cin >> kk;
        }*/
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

namespace {
    PLPRes _buildPLPRes(const std::vector<std::pair<int, TypedProgram>>& unit_list) {
        std::vector<TypedProgram> const_list;
        std::vector<std::pair<int, TypedProgram>> aux_list;
        for (auto& [id, program]: unit_list) {
            if (id < 0) const_list.emplace_back(program);
            else aux_list.emplace_back(id, program);
        }
        return {aux_list, const_list};
    }
}

PLPRes IncrePLPSolver::synthesis(TimeGuard *guard) {
    LOG(INFO) << "solve " << task->example_space->tau_id;
    for (auto* grammar: task->f_grammar_list) {
        LOG(INFO) << "aux grammar";
        grammar->print();
    }
    LOG(INFO) << "const grammar"; task->const_grammar->print();
    auto counter_example = verify({});
    if (counter_example.first == -1) return {{}, {}};
    LOG(INFO) << "Counter example " << example2String(counter_example);
    addExample(counter_example);

    while (true) {
        TimeCheck(guard);
        auto candidate_result = synthesisFromExample(guard);
        LOG(INFO) << "Candidate result " << _unitList2String(candidate_result);

        counter_example = verify(candidate_result);
        if (counter_example.first == -1) return _buildPLPRes(candidate_result);
        addExample(counter_example);
        LOG(INFO) << "Counter example " << example2String(counter_example);
        std::cout << task->runOup(counter_example.first).toString() << " " << task->runOup(counter_example.second).toString() << std::endl;
        for (auto [id, info]: candidate_result) {
            auto [_, prog] = info;
            std::cout << "  " << data::dataList2String(task->runInp(counter_example.first, id, prog.get())) << " " <<
              data::dataList2String(task->runInp(counter_example.second, id, prog.get())) << std::endl;
        }
    }
}