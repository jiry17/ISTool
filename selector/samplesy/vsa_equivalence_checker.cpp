//
// Created by pro on 2022/1/28.
//

#include "istool/selector/samplesy/vsa_equivalence_checker.h"
#include <unordered_set>

using namespace selector::samplesy;

CollectRes::CollectRes(const PProgram &_p, const Data &_oup): p(_p), oup(_oup) {}

VSAEquivalenceChecker::VSAEquivalenceChecker(const PVSABuilder& _builder, FiniteIOExampleSpace* _example_space):
    builder(_builder), example_space(_example_space) {
    ext = builder->ext;
}
void VSAEquivalenceChecker::addExample(const IOExample &example) {
    auto next_root = builder->buildVSA(example.second, example.first, nullptr);
    root = builder->mergeVSA(root, next_root, nullptr);
}

namespace {
    void _collectAllNode(VSANode* node, std::vector<VSANode*>& node_list) {
        if (node_list[node->id]) return;
        node_list[node->id] = node;
        for (const auto& edge: node->edge_list) {
            for (auto* sub_node: edge.node_list) {
                _collectAllNode(sub_node, node_list);
            }
        }
    }

    void _mergeAllResult(int pos, const std::vector<std::vector<int>>& A, std::vector<int>& tmp, std::vector<std::vector<int>>& res) {
        if (pos == A.size()) {
            res.push_back(tmp); return;
        }
        for (int w: A[pos]) {
            tmp[pos] = w;
            _mergeAllResult(pos + 1, A, tmp, res);
        }
    }

    std::vector<std::vector<int>> _mergeAllResult(const std::vector<std::vector<int>>& A) {
        std::vector<std::vector<int>> res;
        std::vector<int> tmp(A.size());
        _mergeAllResult(0, A, tmp, res);
        return res;
    }

    std::vector<int> _buildRange(int l, int r) {
        std::vector<int> res;
        for (int i = l; i < r; ++i) res.push_back(i);
        return res;
    }
}

selector::samplesy::CollectRes VSAEquivalenceChecker::getRes(const VSAEdge &edge, const std::vector<int>& id_list,
        ExecuteInfo* info) {
    auto* fs = dynamic_cast<FullExecutedSemantics*>(edge.semantics.get());
    ProgramList sub_programs;
    for (int i = 0; i < id_list.size(); ++i) {
        sub_programs.push_back(res_pool[edge.node_list[i]->id][id_list[i]].p);
    }
    PProgram res = std::make_shared<Program>(edge.semantics, sub_programs);
    if (fs) {
        DataList inp;
        for (int i = 0; i < id_list.size(); ++i) {
            inp.push_back(res_pool[edge.node_list[i]->id][id_list[i]].oup);
        }
        return {res, fs->run(std::move(inp), info)};
    } else return {res, edge.semantics->run(sub_programs, info)};
}

bool VSAEquivalenceChecker::isValidWitness(const VSAEdge &edge, const Data &oup, const std::vector<int> &id_list, const DataList &params) {
    auto oup_wit = std::make_shared<DirectWitnessValue>(oup);
    auto wit_list = ext->getWitness(edge.semantics.get(), oup_wit, params);
    DataList inp_list;
    for (int i = 0; i < id_list.size(); ++i) {
        inp_list.push_back(res_pool[edge.node_list[i]->id][id_list[i]].oup);
    }
    for (const auto& wit_term: wit_list) {
        bool is_valid = true;
        for (int i = 0; i < wit_term.size(); ++i) {
            if (!wit_term[i]->isInclude(inp_list[i])) is_valid = false;
        }
        if (is_valid) return true;
    }
    return false;
}

bool VSAEquivalenceChecker::extendResPool(int limit, ExecuteInfo* info) {
    int n = node_list.size();
    std::vector<int> pre_size_list(n);
    bool is_finished = true;
    for (int i = 0; i < n; ++i) pre_size_list[i] = res_pool[i].size();
    for (int id = n - 1; id >= 0; --id) {
        auto* node = node_list[id];
        std::unordered_set<std::string> existing_result;
        for (const auto& d: res_pool[id]) existing_result.insert(d.oup.toString());
        for (const auto& edge: node->edge_list) {
            if (!is_finished && res_pool[id].size() == limit) break;
            int m = edge.node_list.size();
            std::vector<std::vector<int>> A(m);
            std::vector<int> id_list(m);
            for (int i = 0; i < m; ++i) id_list[i] = edge.node_list[i]->id;
            for (int i = 0; i < m; ++i) A.push_back(_buildRange(0, res_pool[id_list[i]].size()));
            A = _mergeAllResult(A);

            for (const auto& plan: A) {
                bool is_new = false;
                for (int i = 0; i < plan.size(); ++i) {
                    if (plan[i] >= pre_size_list[id_list[i]]) {
                        is_new = true; break;
                    }
                }
                if (!is_new) continue;
                auto res = getRes(edge, plan, info);

            }
            for (const auto& plan: A) {
                if (!is_finished && res_pool[id].size() == limit) break;
                bool is_new = false;
                for (int i = 0; i < plan.size(); ++i) {
                    if (plan[i] >= pre_size_list[id_list[i]]) {
                        is_new = true; break;
                    }
                }
                if (!is_new) continue;

                auto res = getRes(edge, plan, info);
                auto feature = res.oup.toString();
                if (existing_result.find(feature) != existing_result.end()) continue;
                if (!isValidWitness(edge, res.oup, plan, info->param_value)) continue;
                existing_result.insert(feature);

                if (res_pool[id].size() == limit) {
                    is_finished = false; break;
                }
                res_pool[id].push_back(res);
            }
        }
    }
    return is_finished;
}

std::pair<PProgram, PProgram> VSAEquivalenceChecker::isAllEquivalent(const IOExample &example) {
    int limit = 2;
    auto* info = new ExecuteInfo(example.first, {});
    builder->setter(builder->g, builder->env, example);
    res_pool.clear();
    for (int i = 0; i < node_list.size(); ++i) res_pool.emplace_back();

    std::pair<PProgram, PProgram> res = {nullptr, nullptr};

    while (1) {
        bool is_finished = extendResPool(limit, info);
        if (res_pool[0].size() >= 2) {
            res = {res_pool[0][0].p, res_pool[0][1].p}; break;
        }
        if (is_finished) break;
        limit *= 2;
    }
    delete info;
    return res;
}
std::pair<PProgram, PProgram> VSAEquivalenceChecker::isAllEquivalent() {
    int n = ext::vsa::indexVSANode(root);
    node_list.resize(n);
    for (int i = 0; i < n; ++i) node_list[i] = nullptr;
    _collectAllNode(root, node_list);
    for (auto& example: example_space->example_space) {
        IOExample io_example = example_space->getIOExample(example);
        auto res = isAllEquivalent(io_example);
        if (res.first && res.second) return res;
    }
    return {nullptr, nullptr};
}